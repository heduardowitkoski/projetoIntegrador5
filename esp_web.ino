/*
  ==================================================================================
  Código Final para ESP32 - LDR + MPU6050 e Servidor Web
  ==================================================================================
  Descrição:
  Este código combina a lógica de leitura dos LDRs com o acelerômetro/giroscópio
  MPU6050 para diferenciar gestos ambíguos (forma vs. movimento/orientação).

  Ele também implementa um servidor web que envia a "letraFinal" reconhecida
  e o estado dos LDRs em formato JSON para a aplicação web.

  - Conecta-se à rede Wi-Fi.
  - Calibra o MPU6050 na inicialização.
  - Lê 5 LDRs e o MPU6050 continuamente.
  - Usa uma máquina de estados para identificar a letra correta.
  - Disponibiliza a letra e os estados dos LDRs no endpoint "/gesto" para a aplicação web.
  ==================================================================================
*/

// --- Bibliotecas ---
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h> // <<< --- NOVA BIBLIOTECA PARA JSON ---

// --- CONFIGURAÇÕES DA REDE WI-FI ---
const char* ssid = "Cyber rede";
const char* password = "colorado";

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

// --- Lógica dos LDRs ---
struct ChaveValor {
  byte chave[5];
  char valor;
};

// Ordem -> {mindinho, anelar, médio, indicador, polegar}
ChaveValor Alfabeto[] = {
  {{0, 0, 0, 0, 1}, 'A'},
  {{1, 1, 1, 1, 0}, 'B'},
  {{1, 1, 1, 1, 1}, 'C'}, // Ambíguo: C, E, O, S, Ç
  {{0, 0, 0, 1, 0}, 'D'}, // Ambíguo: D, G, Q, X, Z
  {{1, 1, 1, 0, 0}, 'F'}, // Ambíguo: F, T
  {{0, 0, 1, 1, 0}, 'H'}, // Ambíguo: H, N, U, V, R
  {{1, 0, 0, 0, 0}, 'I'}, // Ambíguo: I, J
  {{0, 0, 1, 1, 1}, 'K'}, // Ambíguo: K, P
  {{0, 0, 0, 1, 1}, 'L'},
  {{0, 1, 1, 1, 0}, 'M'}, // Ambíguo: M, W
  {{1, 0, 0, 0, 1}, 'Y'},
};
const int NUM_LETRAS = sizeof(Alfabeto) / sizeof(ChaveValor);

// Pinos LDR
const int LDR1 = 33; // Mindinho
const int LDR2 = 32; // Anelar
const int LDR3 = 35; // Médio
const int LDR4 = 34; // Indicador
const int LDR5 = 36; // Polegar (SVP)

// <<< --- Saida agora é a variável global estadosLDR ---
// byte Saida[5]; // Não precisa mais de 'Saida', use 'estadosLDR' diretamente
const int LIMITE_LDR = 4000; // Ajuste conforme seus testes

// --- FUNÇÃO DE CALIBRAÇÃO MPU ---
void calibrarMPU() {
  Serial.println("Calibrando MPU... Mantenha o sensor parado na posição de repouso!");
  delay(1000);
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
}

// --- FUNÇÕES DO SERVIDOR WEB ---
void handleGesto() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");

  // <<< --- MUDANÇA CRUCIAL AQUI: Gerar JSON ---
  DynamicJsonDocument doc(128); // 128 bytes é suficiente para "letra" e array de 5 bytes

  doc["letra"] = String(letraReconhecida); // Converte char para String para o JSON
  JsonArray dedosArray = doc.createNestedArray("dedos");
  for (int i = 0; i < 5; i++) {
    dedosArray.add(estadosLDR[i]);
  }

  String jsonOutput;
  serializeJson(doc, jsonOutput);

  server.send(200, "application/json", jsonOutput); // <<< --- Envia como application/json ---
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
  Wire.begin(21, 22); // Pinos I2C
  if (!mpu.begin()) {
    Serial.println("Falha ao encontrar MPU6050!");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  calibrarMPU();

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
  // <<< --- Usa 'estadosLDR' diretamente ---
  (analogRead(LDR1) > LIMITE_LDR) ? estadosLDR[0] = 0 : estadosLDR[0] = 1; // Mindinho
  (analogRead(LDR2) > LIMITE_LDR) ? estadosLDR[1] = 0 : estadosLDR[1] = 1; // Anelar
  (analogRead(LDR3) > LIMITE_LDR) ? estadosLDR[2] = 0 : estadosLDR[2] = 1; // Médio
  (analogRead(LDR4) > LIMITE_LDR) ? estadosLDR[3] = 0 : estadosLDR[3] = 1; // Indicador
  (analogRead(LDR5) > LIMITE_LDR) ? estadosLDR[4] = 0 : estadosLDR[4] = 1; // Polegar
}

char identificarLetraLDR() {
  for (int i = 0; i < NUM_LETRAS; i++) {
    bool match = true;
    for (int j = 0; j < 5; j++) {
      // <<< --- Compara com 'estadosLDR' ---
      if (estadosLDR[j] != Alfabeto[i].chave[j]) { 
        match = false;
        break;
      }
    }
    if (match) return Alfabeto[i].valor;
  }
  return '?';
}


// --- LOOP PRINCIPAL ---
void loop() {
  // 1. LER LDRs E OBTER LETRA BASE
  lerLDRs();
  char letraBase = identificarLetraLDR();
  char letraFinal = '?'; // Começa como '?' por padrão

  // 2. PROCESSAR MPU APENAS SE A FORMA DA MÃO FOR RECONHECIDA
  if (letraBase != '?') {
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

    // 2b. MÁQUINA DE ESTADOS (DECISÃO)
    switch (letraBase) {
      case 'I': // 'I' (estático) vs 'J' (movimento)
        if (gyroMag > LIMITE_MOVIMENTO_GYRO) letraFinal = 'J';
        else letraFinal = 'I';
        break;

      case 'D': // 'D' (estático) vs 'Z' (movimento)
        if (abs(ax) > LIMITE_MOVIMENTO_ACEL) letraFinal = 'Z';
        else letraFinal = 'D';
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
  }
  // Se letraBase era '?', letraFinal permanecerá '?'

  // 3. ATUALIZAR VARIÁVEIS GLOBAIS PARA O SERVIDOR
  letraReconhecida = letraFinal;
  // Os 'estadosLDR' já foram atualizados pela função lerLDRs()

  // 4. PROCESSAR REQUISIÇÕES WEB
  server.handleClient();

  // 5. DEBUG E DELAY
  Serial.print("Dedos (Mindinho a Polegar): ");
  for (int i = 0; i < 5; i++) {
    Serial.print(estadosLDR[i]);
    Serial.print(" ");
  }
  Serial.print(" | Letra Base LDR: "); Serial.print(letraBase);
  Serial.print(" | Letra Final: "); Serial.println(letraFinal);
  delay(50); // Delay menor para melhor responsividade
}