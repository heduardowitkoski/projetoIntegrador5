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

// --- DEFINIÇÕES DE GESTOS (TEMPLATE MATCHING) ---
const int TAMANHO_GESTO = 30; // Ajustado para os seus 30 valores
const float LIMITE_DISPARO_ACEL = 9.0; // Valor solicitado para iniciar leitura
const float TOLERANCIA_GESTO_Z = 3.0; // Se o erro for menor que isso, é Z. 
const float TOLERANCIA_GESTO_H = 1.5; // Se o erro for menor que isso, é H.
const float TOLERANCIA_GESTO_J = 4.25; // Se o erro for menor que isso, é J 
const int DELAY_AMOSTRAGEM = 50; // Mesmo delay usado na calibração

// SEU MODELO CAPTURADO PARA A LETRA Z
const float MODELO_Z_X[30] = {
  -7.15, -6.53, -6.14, -4.18, -1.89, -0.12, 0.90, -1.33, -2.16, -2.61, 
  -3.09, -3.90, -3.20, -3.36, -3.45, -3.99, -4.66, -5.11, -3.72, -3.05, 
  -2.73, -2.97, -3.12, -3.41, -2.65, -2.86, -3.41, -2.29, -2.66, -3.01
};

const float MODELO_H_X[30] = {
  -6.74, -6.01, -6.28, -5.49, -5.86, -5.76, -6.15, -5.94, -6.37, -5.92, 
  -6.85, -5.77, -4.70, -3.29, -1.76, -3.32, -1.16, -3.13, -4.70, -5.13, 
  -4.69, -5.67, -5.63, -5.40, -5.02, -4.48, -4.70, -3.81, -3.53, -3.80
};

const float MODELO_J_X[30] = {
  -5.03, -3.76, -2.76, -2.75, -3.11, -6.17, -0.02, -4.82, -0.28, 1.99, 
  4.89, 5.40, 6.82, 7.43, 8.46, 7.87, 8.08, 6.27, 4.91, 3.97, 
  2.53, 2.06, 0.32, -0.64, -2.34, -1.78, -1.92, -3.61, -3.54, -3.81
};

const float MODELO_P_X[30] = {
  1.08, 2.20, 3.37, 4.36, 4.33, 5.72, 6.05, 7.13, 7.35, 9.09, 
  9.84, 10.17, 10.38, 10.50, 10.95, 11.13, 11.46, 12.58, 13.25, 13.00, 
  13.89, 13.90, 13.34, 13.44, 13.65, 13.49, 12.61, 12.99, 13.30, 13.01
};

const float MODELO_X_X[30] = {
  -6.63, -5.94, -6.07, -6.19, -6.00, -6.28, -3.59, -3.43, -2.91, -3.06, 
  -2.02, -1.26, -0.93, 0.92, 2.45, 3.13, 4.24, 4.32, 4.04, 3.57, 
  3.12, 2.76, 2.92, 2.91, 2.15, 1.09, 0.77, 1.38, 1.34, 0.68
};

const float MODELO_Q_X[30] = {
  -6.82, -6.20, -5.18, -4.87, -5.43, -6.48, -8.38, -5.33, -3.03, -3.94, 
  -2.07, -0.76, -0.27, 0.28, 0.59, 1.40, 2.36, 3.10, 3.76, 4.67, 
  5.16, 5.03, 4.84, 4.93, 4.84, 5.08, 4.90, 4.62, 4.47, 4.72
};

const float MODELO_G_X[30] = {
  -5.03, -3.76, -2.76, -2.75, -3.11, -6.17, -0.02, -4.82, -0.28, 1.99, 
  4.89, 5.40, 6.82, 7.43, 8.46, 7.87, 8.08, 6.27, 4.91, 3.97, 
  2.53, 2.06, 0.32, -0.64, -2.34, -1.78, -1.92, -3.61, -3.54, -3.81
};

const float MODELO_V_X[30] = {
  -6.54, -6.45, -6.58, -6.92, -6.93, -7.46, -5.92, -5.40, -4.66, -4.88, 
  -4.66, -3.73, -3.15, -0.79, -0.13, -0.36, -0.64, -0.06, -0.82, -0.12, 
  -0.02, -3.38, -4.48, -6.00, -5.84, -5.41, -5.77, -6.90, -7.16, -6.50
};

const float MODELO_R_X[30] = {
  -6.31, -6.18, -5.65, -6.24, -5.35, -6.12, -6.46, -5.98, -6.33, -5.86, 
  -4.68, -6.47, -2.01, -5.59, -2.39, -3.07, -0.63, -0.67, 1.13, 1.64, 
  2.50, 2.75, 2.87, 2.97, 3.27, 2.24, 1.38, 0.02, -1.60, -2.94
};

const float MODELO_T_X[30] = {
  -6.42, -5.35, -4.46, -4.82, -4.32, -5.20, -4.91, -3.98, -3.97, -2.16, 
  -1.85, -1.65, -0.01, 1.64, 2.48, 3.40, 4.41, 6.90, 6.00, 5.40, 
  4.76, 4.50, 4.48, 4.19, 3.30, 4.39, 0.20, -1.65, -4.33, -4.20
};
// --- Outros Limites ---
const float LIMITE_MOVIMENTO_GYRO = 80.0; // Mantido para I/J e C/Ç
const float LIMITE_GRAVIDADE = 7.0; // Para H/U/V

// --- Configuração do MPU6050 ---
Adafruit_MPU6050 mpu;
float offset_ax = 0.0, offset_ay = 0.0, offset_az = 0.0;
float offset_gx = 0.0, offset_gy = 0.0, offset_gz = 0.0;

// --- Lógica dos LDRs ---
struct ChaveValor {
  byte chave[5];
  char valor;
};

// Ordem -> {mindinho, anelar, médio, indicador, polegar}
ChaveValor Alfabeto[] = {
  {{0, 0, 0, 0, 1}, 'A'},
  {{1, 1, 1, 1, 0}, 'B'},
  {{1, 1, 1, 1, 1}, 'C'},
  {{0, 0, 0, 1, 0}, 'D'},
  {{1, 1, 1, 0, 1}, 'F'}, // Ambíguo: F, T
  {{0, 0, 1, 1, 0}, 'U'},
  {{0, 0, 1, 0, 1}, 'H'},
  {{1, 0, 0, 0, 0}, 'I'},
  {{0, 0, 1, 0, 1}, 'K'},
  {{0, 0, 0, 1, 1}, 'L'}, // Ambíguo: G, L
  {{0, 1, 1, 1, 0}, 'W'},
  {{0, 0, 0, 0, 0}, 'S'}, // Ambíguo: S, E, N, M, Q
  {{1, 0, 0, 0, 1}, 'Y'},
};
const int NUM_LETRAS = sizeof(Alfabeto) / sizeof(ChaveValor);

// Pinos LDR
const int LDR1 = 32; // Mindinho
const int LDR2 = 33; // Anelar
const int LDR3 = 36; // Médio 
const int LDR4 = 35; // Indicador 
const int LDR5 = 34; // Polegar (SVP) 

const int LDR_PINS[] = {32, 33, 36, 35, 34};

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
  offset_az /= num_leituras;
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

float compararGesto(float* bufferCapturado, const float* modelo) {
  float mediaCaptura = 0.0;
  float mediaModelo = 0.0;

  // 1. Calcula a média dos valores para descobrir o "centro" (offset)
  for (int i = 0; i < TAMANHO_GESTO; i++) {
    mediaCaptura += bufferCapturado[i];
    mediaModelo += modelo[i];
  }
  mediaCaptura /= TAMANHO_GESTO;
  mediaModelo /= TAMANHO_GESTO;

  float erroTotal = 0.0;
  for (int i = 0; i < TAMANHO_GESTO; i++) {
    // 2. Compara removendo o offset (Centraliza as duas ondas no zero)
    // Isso anula o erro causado se a mão estiver inclinada diferente da calibração
    float valorCapturadoNormalizado = bufferCapturado[i] - mediaCaptura;
    float valorModeloNormalizado = modelo[i] - mediaModelo;
    
    erroTotal += abs(valorCapturadoNormalizado - valorModeloNormalizado);
  }
  
  return erroTotal / TAMANHO_GESTO;
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
    float accelMovimento = sqrt(ax * ax + ay * ay);
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
    Serial.print(" | A_Lim:"); Serial.print(LIMITE_DISPARO_ACEL);
    Serial.print(" | Grav_Lim:"); Serial.print(LIMITE_GRAVIDADE);
    // --- FIM DOS PRINTS DE DEBUG ---
    float bufferCaptura[TAMANHO_GESTO];
    accelX_buffer[buffer_index] = ax;
    accelY_buffer[buffer_index] = ay;
    buffer_index = (buffer_index + 1) % BUFFER_SIZE; // Buffer circular 

    // 2b. MÁQUINA DE ESTADOS (DECISÃO)
    switch (letraBase) {
      case 'I': // 'I' (estático) vs 'J' (movimento)
        if (accelMovimento > LIMITE_DISPARO_ACEL) {
           Serial.println(">>> Detectado movimento para J! Gravando...");
           float bufferCaptura[TAMANHO_GESTO];
           
           for(int k=0; k < TAMANHO_GESTO; k++) {
              mpu.getEvent(&a, &g, &temp);
              bufferCaptura[k] = a.acceleration.x - offset_ax; 
              delay(DELAY_AMOSTRAGEM); 
           }
           float erro = compararGesto(bufferCaptura, MODELO_J_X);
           Serial.print("Erro J: "); Serial.println(erro);

           if (erro < TOLERANCIA_GESTO_J) {
              letraFinal = 'J';
              Serial.println("RECONHECIDO: J");
              delay(500); 
           } else {
              letraFinal = 'I'; 
              Serial.println("Falha no J. Mantendo I.");
           }
        } else {
           letraFinal = 'I';
        }
        break;

      case 'D': 
        if (accelMovimento > LIMITE_DISPARO_ACEL) {
           Serial.println(">>> Movimento detectado (D/Z/X/P/Q)! Gravando...");
           
           // 1. Captura o movimento
           for(int k=0; k < TAMANHO_GESTO; k++) {
              mpu.getEvent(&a, &g, &temp);
              bufferCaptura[k] = a.acceleration.x - offset_ax; 
              delay(DELAY_AMOSTRAGEM); 
           }

           // 2. Compara com TODOS os modelos possíveis para essa configuração de mão
           float erroZ = compararGesto(bufferCaptura, MODELO_Z_X);
           float erroX = compararGesto(bufferCaptura, MODELO_X_X);
           float erroP = compararGesto(bufferCaptura, MODELO_P_X);
           float erroQ = compararGesto(bufferCaptura, MODELO_Q_X);

           Serial.print("Erros -> Z: "); Serial.print(erroZ);
           Serial.print(" | X: "); Serial.print(erroX);
           Serial.print(" | P: "); Serial.print(erroP);
           Serial.print(" | Q: "); Serial.println(erroQ);

           // 3. Define tolerâncias (Ajuste conforme necessário)
           float tolZ = TOLERANCIA_GESTO_Z; 
           float tolX = 4.0; // Tolerância para X
           float tolP = 4.0; // Tolerância para P
           float tolQ = 4.0; // Tolerância para Q

           // 4. Lógica do Vencedor (Quem tem o menor erro E está dentro da tolerância?)
           char vencedor = 'D'; // Padrão se ninguém ganhar
           float menorErro = 100.0;

           // Testar Z
           if (erroZ < tolZ && erroZ < menorErro) {
             menorErro = erroZ;
             vencedor = 'Z';
           }
           // Testar X
           if (erroX < tolX && erroX < menorErro) {
             menorErro = erroX;
             vencedor = 'X';
           }
           // Testar P
           if (erroP < tolP && erroP < menorErro) {
             menorErro = erroP;
             vencedor = 'P';
           }
           // Testar Q
           if (erroQ < tolQ && erroQ < menorErro) {
             menorErro = erroQ;
             vencedor = 'Q';
           }

           letraFinal = vencedor;
           
           if (letraFinal != 'D') {
              Serial.print(">>> VENCEDOR RECONHECIDO: "); Serial.println(letraFinal);
              delay(500); // Pausa para não repetir leitura imediata
           } else {
              Serial.println(">>> Movimento não reconhecido. Mantendo D.");
           }

        } else {
           letraFinal = 'D'; // Sem movimento = D
        }
        break;

      case 'C': // 'Ç' (movimento) vs 'C', 'O', 'S', 'E' (estáticos)
        if (accelMovimento > LIMITE_DISPARO_ACEL) letraFinal = 'Ç';
        else letraFinal = 'C'; // ou O
        break;

      case 'H': // 'H' (movimento)
        // 1. Tenta verificar se é o gesto H pelo movimento
        if (accelMovimento > LIMITE_DISPARO_ACEL) {
           Serial.println(">>> Detectado movimento para H! Gravando...");
           float bufferCaptura[TAMANHO_GESTO];
           
           for(int k=0; k < TAMANHO_GESTO; k++) {
              mpu.getEvent(&a, &g, &temp);
              bufferCaptura[k] = a.acceleration.x - offset_ax; 
              delay(DELAY_AMOSTRAGEM); 
           }
           float erro = compararGesto(bufferCaptura, MODELO_H_X);
           Serial.print("Erro H: "); Serial.println(erro);

           if (erro < TOLERANCIA_GESTO_H) {
              letraFinal = 'H';
              Serial.println("RECONHECIDO: H");
              delay(500); 
           } else {
            letraFinal = '?'; // Default se falhar o H e não estiver inclinado
           }
        } else {
          letraFinal = '?'; // Default estático
        }
        break;
      
      case 'U': 
        if (accelMovimento > LIMITE_DISPARO_ACEL) {
           Serial.println(">>> Movimento V/R detectado! Gravando...");
           for(int k=0; k < TAMANHO_GESTO; k++) {
              mpu.getEvent(&a, &g, &temp);
              bufferCaptura[k] = a.acceleration.x - offset_ax; 
              delay(DELAY_AMOSTRAGEM); 
           }
           
           float erroV = compararGesto(bufferCaptura, MODELO_V_X);
           float erroR = compararGesto(bufferCaptura, MODELO_R_X);
           Serial.print(" E_V:"); Serial.print(erroV);
           Serial.print(" E_R:"); Serial.println(erroR);

           float menorErro = 100.0;
           char vencedor = 'U'; // Default se falhar o reconhecimento

           if (erroV < 5 && erroV > 4 && erroV < menorErro) { menorErro = erroV; vencedor = 'V'; }
           if (erroR < 2 && erroR < menorErro) { menorErro = erroR; vencedor = 'R'; }

           letraFinal = vencedor;
           Serial.print("Vencedor: "); Serial.println(letraFinal);
           delay(500);
        } else {
           letraFinal = 'U'; // Estático assume-se U
        }
        break;
      
      case 'F':
        // A letra base é F. Se houver movimento específico, pode ser T.
        if (accelMovimento > LIMITE_DISPARO_ACEL) {
           Serial.println(">>> Movimento T detectado! Gravando...");
           
           // 1. Captura
           for(int k=0; k < TAMANHO_GESTO; k++) {
              mpu.getEvent(&a, &g, &temp);
              bufferCaptura[k] = a.acceleration.x - offset_ax; 
              delay(DELAY_AMOSTRAGEM); 
           }
           
           // 2. Compara com modelo T
           float erroT = compararGesto(bufferCaptura, MODELO_T_X);
           Serial.print("Erro T: "); Serial.println(erroT);

           if (erroT < 4.5) {
              letraFinal = 'T';
              Serial.println("Reconhecido: T");
              delay(500); 
           } else {
              letraFinal = 'F'; // Moveu, mas não parece T, mantém F
              Serial.println("Movimento não é T. Mantendo F.");
           }
        } else {
           letraFinal = 'F'; // Estático
        }
        break;

      case 'K':
        letraFinal = 'K'; 
        break;

      case 'W':
        letraFinal = 'W';
        break;

      case 'L':
        // A letra base é F. Se houver movimento específico, pode ser T.
        if (accelMovimento > LIMITE_DISPARO_ACEL) {
           Serial.println(">>> Movimento T detectado! Gravando...");
           
           // 1. Captura
           for(int k=0; k < TAMANHO_GESTO; k++) {
              mpu.getEvent(&a, &g, &temp);
              bufferCaptura[k] = a.acceleration.x - offset_ax; 
              delay(DELAY_AMOSTRAGEM); 
           }
           
           // 2. Compara com modelo T
           float erroT = compararGesto(bufferCaptura, MODELO_T_X);
           Serial.print("Erro T: "); Serial.println(erroT);

           if (erroT < 4.5) {
              letraFinal = 'G';
              Serial.println("Reconhecido: T");
              delay(500); 
           } else {
              letraFinal = 'L'; // Moveu, mas não parece T, mantém F
              Serial.println("Movimento não é T. Mantendo F.");
           }
        } else {
           letraFinal = 'L'; // Estático
        }
        break;

      case 'S':

        break;
      default:
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