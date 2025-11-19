/*
  ==================================================================================
  Código Final para ESP32 - LDR + MPU6050 e Servidor Web
  ==================================================================================
  Descrição:
  Este código combina a lógica de leitura dos LDRs com o acelerômetro/giroscópio
  MPU6050 para diferenciar gestos ambíguos (forma vs. movimento/orientação).

  Ele também implementa um servidor web que envia a "letraFinal" reconhecida
  para a aplicação web.

  - Conecta-se à rede Wi-Fi.
  - Calibra o MPU6050 na inicialização.
  - Lê 5 LDRs e o MPU6050 continuamente.
  - Usa uma máquina de estados para identificar a letra correta.
  - Disponibiliza a letra no endpoint "/gesto" para a aplicação web.
  ==================================================================================
*/

// --- Bibliotecas ---
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h> // Adicionado para a versão JSON do ESP32

// --- CONFIGURAÇÕES DA REDE WI-FI ---
const char* ssid = "Henrique204_RCT_2G";
const char* password = "Colorado2002";

// Cria uma instância do servidor na porta 80
WebServer server(80);

// Variáveis globais para o servidor web (agora uma char e um array de bytes)
char letraReconhecida = '?';
byte estadosLDR[5] = {0, 0, 0, 0, 0}; // Para armazenar o estado atual dos LDRs

// --- Configuração do MPU6050 ---
Adafruit_MPU6050 mpu;
float offset_ax = 0.0, offset_ay = 0.0, offset_az = 0.0;
float offset_gx = 0.0, offset_gy = 0.0, offset_gz = 0.0;

// --- DEFINIÇÕES DE LIMITE (Thresholds) ---
// Ajuste estes valores com testes!
const float LIMITE_MOVIMENTO_GYRO = 80.0;
const float LIMITE_MOVIMENTO_ACEL = 5.0;
const float LIMITE_GRAVIDADE = 7.0;

unsigned long tempoInicioMovimento = 0;
const unsigned long TEMPO_MAX_MOVIMENTO_MS = 1000; // Tempo máximo para completar o gesto (1 segundo)
const float LIMITE_MOVIMENTO_INICIAL_GYRO = 0.4; // Limiar para detectar o *início* do movimento
const float LIMITE_REMOVER_REPOSO_ACEL = 0.5;

// --- Lógica dos LDRs ---
struct ChaveValor {
  byte chave[5];
  char valor;
};

// Ordem -> {mindinho, anelar, médio, indicador, polegar}
ChaveValor Alfabeto[] = {
  {{0, 0, 0, 0, 1}, 'A'},
  {{1, 1, 1, 1, 0}, 'B'},
  {{1, 1, 1, 1, 1}, 'C'}, // Ambíguo: C, O
  {{0, 0, 0, 1, 0}, 'D'}, // Ambíguo: D, Q, X, Z, P
  {{1, 1, 1, 0, 1}, 'F'}, // Ambíguo: F, T (O MAIS DIFICIL DE RESOLVER)
  {{0, 0, 1, 1, 0}, 'N'}, // Ambíguo: N, U, V, R
  {{0, 0, 1, 0, 0}, 'H'}, // Dedao sobrepoe o LDR do indicador
  {{1, 0, 0, 0, 0}, 'I'}, // Ambíguo: I, J
  {{0, 0, 1, 0, 1}, 'K'}, // Ambíguo: K, // Dedao sobrepoe o LDR do indicador
  {{0, 0, 0, 1, 1}, 'L'}, // Ambíguo: G, L
  {{0, 1, 1, 1, 0}, 'M'}, // Ambíguo: M, W
  {{0, 0, 0, 0, 0}, 'S'}, // Ambíguo: S, E
  {{1, 0, 0, 0, 1}, 'Y'},
};
const int NUM_LETRAS = sizeof(Alfabeto) / sizeof(ChaveValor);

// Pinos LDR
const int LDR1 = 33; // Mindinho
const int LDR2 = 32; // Anelar
const int LDR3 = 35; // Médio
const int LDR4 = 34; // Indicador
const int LDR5 = 36; // Polegar (SVP)

const int LDR_PINS[] = {33, 32, 35, 34, 36};

// byte Saida[5]; // Substituído por estadosLDR
const int LIMITE_LDR = 4000; // Ajuste conforme seus testes
const int NUM_LDRS = 5;
int ldrThresholds[NUM_LDRS]; // Limiares calculados para cada LDR
bool ldrCalibrated = false;  // Flag para saber se a calibração foi feita

void calibrarLDRs() {
  Serial.println("\n--- INICIANDO CALIBRACAO DOS LDRs ---");
  Serial.println("Certifique-se de que a luva esta no ambiente de uso.");
  delay(5000);

  int ldrMinValues[NUM_LDRS]; // Valores quando o dedo está flexionado (máximo cobertura, mínima luz)
  int ldrMaxValues[NUM_LDRS]; // Valores quando o dedo está esticado (mínima cobertura, máxima luz)

  // 1. CALIBRAR DEDOS ESTICADOS (Max Luz)
  Serial.println("\n1. ESTIQUE TODOS OS DEDOS e mantenha por 3 segundos...");
  delay(3000); // Dá tempo para o usuário esticar os dedos

  for (int i = 0; i < NUM_LDRS; i++) {
    ldrMaxValues[i] = analogRead(LDR_PINS[i]);
    Serial.print("LDR"); Serial.print(i + 1); Serial.print(" (Esticado): "); Serial.println(ldrMaxValues[i]);
  }
  delay(1000);

  // 2. CALIBRAR DEDOS FLEXIONADOS (Min Luz)
  Serial.println("\n2. FLEXIONE TODOS OS DEDOS (feche a mao) e mantenha por 3 segundos...");
  delay(3000); // Dá tempo para o usuário flexionar os dedos

  for (int i = 0; i < NUM_LDRS; i++) {
    ldrMinValues[i] = analogRead(LDR_PINS[i]);
    Serial.print("LDR"); Serial.print(i + 1); Serial.print(" (Flexionado): "); Serial.println(ldrMinValues[i]);
  }
  delay(1000);

  // 3. CALCULAR LIMIARES
  Serial.println("\nCalculando limiares...");
  for (int i = 0; i < NUM_LDRS; i++) {
    // O limiar é o ponto médio entre os valores min e max
    // Ou uma porcentagem, ex: ldrThresholds[i] = ldrMinValues[i] + (ldrMaxValues[i] - ldrMinValues[i]) * 0.7;
    // O 0.7 indica que o dedo é considerado "flexionado" se a leitura estiver 70% mais perto do "flexionado"
    // Ou simplesmente o meio:
    ldrThresholds[i] = (ldrMinValues[i] + ldrMaxValues[i]) / 2;
    Serial.print("LDR"); Serial.print(i + 1); Serial.print(" Limiar: "); Serial.println(ldrThresholds[i]);
  }

  ldrCalibrated = true;
  Serial.println("\n--- CALIBRACAO DOS LDRs CONCLUIDA ---");
}

// --- FUNÇÃO DE CALIBRAÇÃO MPU ---
void calibrarMPU() {
  Serial.println("Calibrando MPU... Mantenha o sensor parado na posição de repouso!");
  delay(1000); // Dá um segundo para a pessoa posicionar o sensor
  long num_leituras = 1000;

  for (int i = 0; i < num_leituras; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    offset_ax += a.acceleration.x;
    offset_ay += a.acceleration.y;
    offset_az += a.acceleration.z;
    offset_gx += g.gyro.x;
    offset_gy += g.gyro.y;
    offset_gz += g.gyro.z;
    delay(1);
  }
  offset_ax /= num_leituras;
  offset_ay /= num_leituras;
  offset_az = (offset_az / num_leituras) - 9.81; // Subtrai a gravidade
  offset_gx /= num_leituras;
  offset_gy /= num_leituras;
  offset_gz /= num_leituras;

  Serial.println("Calibração concluída.");
  Serial.print("Offsets de Aceleração (X,Y,Z): "); Serial.print(offset_ax, 2); Serial.print(", "); Serial.print(offset_ay, 2); Serial.print(", "); Serial.println(offset_az, 2);
  Serial.print("Offsets de Giroscópio (X,Y,Z): "); Serial.print(offset_gx, 2); Serial.print(", "); Serial.print(offset_gy, 2); Serial.print(", "); Serial.println(offset_gz, 2);
}

// --- FUNÇÕES DO SERVIDOR WEB ---
void handleGesto() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");

  DynamicJsonDocument doc(128); // 128 bytes é suficiente para "letra" e array de 5 bytes

  doc["letra"] = String(letraReconhecida); // Converte char para String para o JSON
  JsonArray dedosArray = doc.createNestedArray("dedos");
  for (int i = 0; i < 5; i++) {
    dedosArray.add(estadosLDR[i]);
  }

  String jsonOutput;
  serializeJson(doc, jsonOutput);

  server.send(200, "application/json", jsonOutput); // Envia como application/json
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Nao encontrado");
}

// --- FUNÇÃO DE SETUP ---
void setup() {
  Serial.begin(115200);
  delay(100);

  // --- 1. Inicia MPU ---
  Serial.println("Iniciando MPU6050...");
  Wire.begin(21, 22); // Pinos I2C do ESP32 (verifique se são os corretos para sua placa)
  if (!mpu.begin()) {
    Serial.println("Falha ao encontrar MPU6050!");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); // Ou MPU6050_BAND_184_HZ, MPU6050_BAND_94_HZ, MPU6050_BAND_44_HZ
  calibrarMPU();

  calibrarLDRs();

  // --- 2. Conexão Wi-Fi ---
  Serial.println("\nIniciando Wi-Fi...");
  WiFi.begin(ssid, password);
  Serial.print("Conectando a rede");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado com sucesso!");
  Serial.print("Endereco de IP do ESP32: ");
  Serial.println(WiFi.localIP());

  // --- 3. Configuração do Servidor ---
  server.on("/gesto", HTTP_GET, handleGesto);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
  Serial.println("Sistema pronto. Começando a leitura...");
}


// --- FUNÇÕES AUXILIARES DE LEITURA ---
void lerLDRs() {
  (analogRead(LDR1) > LIMITE_LDR) ? estadosLDR[0] = 0 : estadosLDR[0] = 1;
  (analogRead(LDR2) > LIMITE_LDR) ? estadosLDR[1] = 0 : estadosLDR[1] = 1;
  (analogRead(LDR3) > LIMITE_LDR) ? estadosLDR[2] = 0 : estadosLDR[2] = 1;
  (analogRead(LDR4) > LIMITE_LDR) ? estadosLDR[3] = 0 : estadosLDR[3] = 1;
  (analogRead(LDR5) > LIMITE_LDR) ? estadosLDR[4] = 0 : estadosLDR[4] = 1;
}

char identificarLetraLDR() {
  for (int i = 0; i < NUM_LETRAS; i++) {
    bool match = true;
    for (int j = 0; j < 5; j++) {
      if (estadosLDR[j] != Alfabeto[i].chave[j]) {
        match = false;
        break;
      }
    }
    if (match) return Alfabeto[i].valor;
  }
  return '?';
}
const int BUFFER_SIZE = 20; // Armazena as últimas 20 leituras (20 * 50ms = 1 segundo de dados)
float accelX_buffer[BUFFER_SIZE];
float accelY_buffer[BUFFER_SIZE];
int buffer_index = 0;

// --- LOOP PRINCIPAL ---
void loop() {
  // 1. LER LDRs E OBTER LETRA BASE
  lerLDRs();
  char letraBase = identificarLetraLDR();
  char letraFinal = '?'; // Começa como '?' por padrão

  // 2. PROCESSAR MPU APENAS SE A FORMA DA MÃO FOR RECONHECIDA
  // Se você quiser ver os dados do MPU sempre, remova o 'if (letraBase != '?')'
  // mas para testar o MPU, é bom ter os LDRs funcionando também
  // if (letraBase != '?') { // Descomente esta linha para reativar a condição
    // 2a. Ler MPU
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Aplica calibração
    float ax = a.acceleration.x - offset_ax;
    float ay = a.acceleration.y - offset_ay;
    float az = a.acceleration.z - offset_az;
    float gx = g.gyro.x - offset_gx;
    float gy = g.gyro.y - offset_gy;
    float gz = g.gyro.z - offset_gz;

    float gyroMag = sqrt(gx * gx + gy * gy + gz * gz);
    Serial.print("LDR1: "); Serial.print(estadosLDR[0]);
    Serial.print("LDR2: "); Serial.print(estadosLDR[1]);
    Serial.print("LDR3: "); Serial.print(estadosLDR[2]);
    Serial.print("LDR4: "); Serial.print(estadosLDR[3]);
    Serial.print("LDR5: "); Serial.print(estadosLDR[4]);

    // --- PRINTS DE DEBUG DO MPU6050 ---
    Serial.print("ACC (cal) X:"); Serial.print(ax, 2);
    Serial.print(" Y:"); Serial.print(ay, 2);
    Serial.print(" Z:"); Serial.print(az, 2);
    Serial.print(" | GYRO (cal) X:"); Serial.print(gx, 2);
    Serial.print(" Y:"); Serial.print(gy, 2);
    Serial.print(" Z:"); Serial.print(gz, 2);
    Serial.print(" | GyroMag:"); Serial.print(gyroMag, 2);
    Serial.print(" | G_Lim:"); Serial.print(LIMITE_MOVIMENTO_GYRO);
    Serial.print(" | A_Lim:"); Serial.print(LIMITE_MOVIMENTO_ACEL);
    Serial.print(" | Grav_Lim:"); Serial.print(LIMITE_GRAVIDADE);
    // --- FIM DOS PRINTS DE DEBUG ---

    accelX_buffer[buffer_index] = ax;
    accelY_buffer[buffer_index] = ay;
    buffer_index = (buffer_index + 1) % BUFFER_SIZE; // Buffer circular 

    // 2b. MÁQUINA DE ESTADOS (DECISÃO)
    switch (letraBase) {
      case 'I': // 'I' (estático) vs 'J' (movimento)
        if (gyroMag > LIMITE_MOVIMENTO_GYRO) letraFinal = 'J';
        else letraFinal = 'I';
        break;

      case 'D': // 'D' (estático) vs 'Z' (movimento)
        // 1. Detecta o início do movimento para 'Z'
        if (gyroMag > LIMITE_MOVIMENTO_INICIAL_GYRO && tempoInicioMovimento == 0) {
            tempoInicioMovimento = millis();
            Serial.println("Movimento inicial para Z detectado! Preparando buffer.");
            // Limpa o buffer ou inicializa para as próximas leituras relevantes
            for(int i=0; i<BUFFER_SIZE; i++) {
                accelX_buffer[i] = 0;
                accelY_buffer[i] = 0;
            }
            buffer_index = 0; // Reinicia o index do buffer
        }

        // 2. Se há um movimento em andamento para 'Z' e dentro do tempo limite
        if (tempoInicioMovimento > 0 && (millis() - tempoInicioMovimento < TEMPO_MAX_MOVIMENTO_MS)) {
            // --- ANÁLISE DO PADRÃO DE MOVIMENTO PARA 'Z' COM BUFFER ---
            // Procura por picos/vales que indicam um zigue-zague
            // Esta é ainda uma simplificação, mas mais robusta que um único ponto.

            bool zPatternDetected = false;
            int numXChanges = 0; // Conta mudanças de direção no eixo X
            int numYChanges = 0; // Conta mudanças de direção no eixo Y

            // Itera sobre o buffer para encontrar mudanças de direção
            for (int i = 1; i < BUFFER_SIZE; i++) {
                // Verifica mudanças de sinal (pico-vale ou vale-pico)
                // Com um pequeno "deadband" para evitar ruído
                if (accelX_buffer[i] > LIMITE_REMOVER_REPOSO_ACEL && accelX_buffer[i-1] < -LIMITE_REMOVER_REPOSO_ACEL) numXChanges++;
                if (accelX_buffer[i] < -LIMITE_REMOVER_REPOSO_ACEL && accelX_buffer[i-1] > LIMITE_REMOVER_REPOSO_ACEL) numXChanges++;
                
                if (accelY_buffer[i] > LIMITE_REMOVER_REPOSO_ACEL && accelY_buffer[i-1] < -LIMITE_REMOVER_REPOSO_ACEL) numYChanges++;
                if (accelY_buffer[i] < -LIMITE_REMOVER_REPOSO_ACEL && accelY_buffer[i-1] > LIMITE_REMOVER_REPOSO_ACEL) numYChanges++;
            }

            // Um 'Z' teria pelo menos 2-3 mudanças de direção em X (horizontal)
            // e talvez 1-2 em Y (diagonal) ou vice-versa, dependendo da orientação do sensor.
            // Ajuste esses valores `> X` experimentalmente!
            if (numXChanges >= 2 || numYChanges >= 2) { // Ex: 2 ou mais mudanças de direção em X ou Y
                zPatternDetected = true;
            }
            Serial.print("Num X Changes: "); Serial.print(numXChanges); Serial.print(" | Num Y Changes: "); Serial.println(numYChanges);

            if (zPatternDetected) {
                letraFinal = 'Z';
                tempoInicioMovimento = 0; // Gesto reconhecido, resetar para próximo gesto
            } else {
                letraFinal = 'D'; // Ainda não é 'Z'
            }
        } else if (tempoInicioMovimento > 0 && (millis() - tempoInicioMovimento >= TEMPO_MAX_MOVIMENTO_MS)) {
            // Tempo excedido para completar o gesto 'Z', resetar
            Serial.println("Tempo de gesto Z excedido. Resetando.");
            tempoInicioMovimento = 0;
            letraFinal = 'D'; // Volta para 'D'
        } else {
            // Nenhum movimento para 'Z' detectado, assume 'D'
            letraFinal = 'D';
        }
        break;

      case 'C': // 'Ç' (movimento) vs 'C', 'O', 'S', 'E' (estáticos)
        if (gyroMag > LIMITE_MOVIMENTO_GYRO) letraFinal = 'Ç';
        else letraFinal = 'C'; // Default para estáticos
        break;

      case 'H': // 'H' (horizontal) vs 'U'/'V' (vertical)
        // Você precisará ajustar essa lógica de acordo com o que 'U' e 'V' significam
        // para o seu sensor e como eles se diferenciam de 'H'.
        // Exemplo: se 'U' e 'V' são mais "verticais", 'H' é "horizontal".
        // Pode ser necessário verificar o sinal de ay ou az e thresholds diferentes.
        // Para uma diferenciação mais robusta entre U e V, você pode precisar
        // analisar a variação de um eixo específico ou a orientação da mão.
        if (abs(ay) > LIMITE_GRAVIDADE && ay < 0) letraFinal = 'U'; // Ex: mão apontando para cima
        else if (abs(ay) > LIMITE_GRAVIDADE && ay > 0) letraFinal = 'V'; // Ex: mão apontando para baixo
        else letraFinal = 'H'; // Default se não for vertical o suficiente
        break;
      
      // Adicione mais 'case' aqui para F/T, K/P, M/W, etc.
      case 'F': // 'F' vs 'T'
        // Exemplo: se 'T' envolve o polegar mais escondido ou um movimento específico
        // Mantenha como 'F' se não houver movimento distintivo
        letraFinal = 'F'; 
        break;

      case 'K': // 'K' vs 'P'
        // Exemplo: 'P' geralmente tem a mão virada para baixo, 'K' mais reta
        // Verifique a orientação do eixo Z (roll) ou X (pitch)
        letraFinal = 'K'; 
        break;

      case 'M': // 'M' vs 'W'
        // 'M' e 'W' podem ser difíceis apenas com LDRs, talvez o MPU ajude com pequenos movimentos
        letraFinal = 'M';
        break;

      default:
        // Se a letra não é ambígua (A, B, L...), usa a letraBase
        letraFinal = letraBase;
        break;
    }
  // } // Descomente esta linha para reativar a condição
  // Se letraBase era '?', letraFinal permanecerá '?'

  // 3. ATUALIZAR VARIÁVEIS GLOBAIS PARA O SERVIDOR
  letraReconhecida = letraFinal;
  // Os 'estadosLDR' já foram atualizados pela função lerLDRs()

  // 4. PROCESSAR REQUISIÇÕES WEB
  server.handleClient();

  // 5. DEBUG E DELAY (Combinado para MPU e Letra)
  Serial.print(" | Dedos: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(estadosLDR[i]);
    Serial.print(" ");
  }
  Serial.print(" | Letra Base LDR: "); Serial.print(letraBase);
  Serial.print(" | Letra Final: "); Serial.println(letraFinal);
  delay(50); // Delay menor para melhor responsividade
}