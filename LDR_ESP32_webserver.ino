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

// --- CONFIGURAÇÕES DA REDE WI-FI ---
const char* ssid = "Henrique204_RCT_2G";
const char* password = "Colorado2002";

// Cria uma instância do servidor na porta 80
WebServer server(80);

// Variável global para o servidor web
char letraReconhecida = '?';

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

byte Saida[5];
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
  server.send(200, "text/plain", String(letraReconhecida));
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
  (analogRead(LDR1) > LIMITE_LDR) ? Saida[0] = 0 : Saida[0] = 1;
  (analogRead(LDR2) > LIMITE_LDR) ? Saida[1] = 0 : Saida[1] = 1;
  (analogRead(LDR3) > LIMITE_LDR) ? Saida[2] = 0 : Saida[2] = 1;
  (analogRead(LDR4) > LIMITE_LDR) ? Saida[3] = 0 : Saida[3] = 1;
  (analogRead(LDR5) > LIMITE_LDR) ? Saida[4] = 0 : Saida[4] = 1;
}

char identificarLetraLDR() {
  for (int i = 0; i < NUM_LETRAS; i++) {
    bool match = true;
    for (int j = 0; j < 5; j++) {
      if (Saida[j] != Alfabeto[i].chave[j]) {
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
        if (abs(ay) > LIMITE_GRAVIDADE) letraFinal = 'U'; // Ou 'V'
        else if (abs(az) > LIMITE_GRAVIDADE) letraFinal = 'H';
        else letraFinal = 'H'; // Default
        break;
      
      // Adicione mais 'case' aqui para F/T, K/P, M/W

      default:
        // Se a letra não é ambígua (A, B, L...), usa a letraBase
        letraFinal = letraBase;
        break;
    }
  }
  // Se letraBase era '?', letraFinal permanecerá '?'

  // 3. ATUALIZAR VARIÁVEL GLOBAL PARA O SERVIDOR
  letraReconhecida = letraFinal;

  // 4. PROCESSAR REQUISIÇÕES WEB
  server.handleClient();

  // 5. DEBUG E DELAY
  Serial.print("Dedos: "); Serial.print(letraBase);
  Serial.print(" | MPU diz: "); Serial.println(letraFinal);
  delay(50); // Delay menor para melhor responsividade
}

