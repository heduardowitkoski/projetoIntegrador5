/*
  ==================================================================================
  Código Final para ESP32 - Cliente (Edge) + Servidor Web
  ==================================================================================
  Nova Arquitetura:
  1. Lê LDRs e MPU6050.
  2. Detecta se houve movimento brusco.
  3. Envia os dados crus (LDRs + Buffer de Aceleração) para o Raspberry Pi (Python).
  4. Recebe a letra processada do Raspberry Pi.
  5. Disponibiliza a letra no endpoint "/gesto" para a aplicação web.
  ==================================================================================
*/

// --- Bibliotecas ---
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h> // Biblioteca para enviar requisições ao Raspberry Pi

// --- CONFIGURAÇÕES DA REDE WI-FI ---
const char* ssid = "Henrique204_RCT_2G";
const char* password = "Colorado2002";

// --- CONFIGURAÇÃO DO RASPBERRY PI ---
// IMPORTANTE: Coloque o IP do seu Raspberry Pi aqui. Mantenha a porta 5000.
const char* serverPi = "http://192.168.1.4:5000/processar_sinal"; 

// Cria uma instância do servidor na porta 80 (Para o Web App)
WebServer server(80);

// Variáveis globais
char letraReconhecida = '?';
byte estadosLDR[5] = {0, 0, 0, 0, 0}; 

// --- DEFINIÇÕES DE CONTROLE ---
const int TAMANHO_GESTO = 30; 
const float LIMITE_DISPARO_ACEL = 9.0; // Valor para iniciar gravação de movimento
const int DELAY_AMOSTRAGEM = 50; // Delay entre leituras do MPU

// Configuração do MPU6050
Adafruit_MPU6050 mpu;
float offset_ax = 0.0, offset_ay = 0.0, offset_az = 0.0;
float offset_gx = 0.0, offset_gy = 0.0, offset_gz = 0.0;

// Pinos LDR
const int LDR_PINS[] = {32, 33, 36, 35, 34}; // Mindinho, Anelar, Médio, Indicador, Polegar
const int NUM_LDRS = 5;
const int LIMITE_LDR_PADRAO = 4000; // Valor fallback
int ldrThresholds[NUM_LDRS]; 
bool ldrCalibrated = false;

// Buffer para enviar ao Raspberry
float bufferEnvio[TAMANHO_GESTO];

// --- 1. CALIBRAÇÃO DOS LDRs ---
void calibrarLDRs() {
  Serial.println("\n--- INICIANDO CALIBRACAO DOS LDRs ---");
  Serial.println("Certifique-se de que a luva esta no ambiente de uso.");
  delay(5000);

  int ldrMinValues[NUM_LDRS];
  int ldrMaxValues[NUM_LDRS];

  // 1. DEDOS ESTICADOS
  Serial.println("\n1. ESTIQUE TODOS OS DEDOS e mantenha...");
  delay(3000);
  for (int i = 0; i < NUM_LDRS; i++) {
    ldrMaxValues[i] = analogRead(LDR_PINS[i]);
  }
  delay(1000);

  // 2. DEDOS FLEXIONADOS
  Serial.println("\n2. FLEXIONE TODOS OS DEDOS (feche a mao) e mantenha...");
  delay(3000);
  for (int i = 0; i < NUM_LDRS; i++) {
    ldrMinValues[i] = analogRead(LDR_PINS[i]);
  }
  delay(1000);

  // 3. CALCULAR LIMIARES
  Serial.println("\nCalculando limiares...");
  for (int i = 0; i < NUM_LDRS; i++) {
    ldrThresholds[i] = (ldrMinValues[i] + ldrMaxValues[i]) / 2;
    Serial.print("LDR"); Serial.print(i + 1); Serial.print(" Limiar: "); Serial.println(ldrThresholds[i]);
  }
  ldrCalibrated = true;
  Serial.println("--- CALIBRACAO LDR CONCLUIDA ---");
}

// --- 2. CALIBRAÇÃO DO MPU ---
void calibrarMPU() {
  Serial.println("Calibrando MPU... Mantenha a mão parada!");
  delay(1000);
  long num_leituras = 1000;

  for (int i = 0; i < num_leituras; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    offset_ax += a.acceleration.x;
    offset_ay += a.acceleration.y;
    offset_az += a.acceleration.z;
    delay(1);
  }
  offset_ax /= num_leituras;
  offset_ay /= num_leituras;
  offset_az /= num_leituras;
  
  Serial.println("MPU Calibrado.");
}

// --- 3. SERVIDOR WEB (Para a Aplicação React/JS) ---
void handleGesto() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");

  DynamicJsonDocument doc(256);
  doc["letra"] = String(letraReconhecida); 
  JsonArray dedosArray = doc.createNestedArray("dedos");
  for (int i = 0; i < 5; i++) {
    dedosArray.add(estadosLDR[i]);
  }

  String jsonOutput;
  serializeJson(doc, jsonOutput);
  server.send(200, "application/json", jsonOutput);
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Nao encontrado");
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  delay(100);

  // Inicia MPU
  Serial.println("Iniciando MPU6050...");
  Wire.begin(21, 22); 
  if (!mpu.begin()) {
    Serial.println("Falha ao encontrar MPU6050!");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  calibrarMPU();
  calibrarLDRs();

  // Conexão Wi-Fi
  Serial.println("\nIniciando Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Conectado!");
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.localIP());

  // Inicia Servidor Web
  server.on("/gesto", HTTP_GET, handleGesto);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

// --- FUNÇÃO DE LEITURA LDR ---
void lerLDRs() {
  for(int i=0; i<NUM_LDRS; i++) {
     int leitura = analogRead(LDR_PINS[i]);
     // Se a leitura for maior que o limiar, considera dedo esticado (1), senão flexionado (0)
     // Nota: Verifique se sua lógica é Maior ou Menor dependendo do seu circuito divisor de tensão
     // Assumindo: Muita luz (esticado) = valor alto. Pouca luz (fechado) = valor baixo.
     // No seu código original: analogRead > LIMITE ? 0 : 1. Isso inverte a lógica. Vou manter a original:
     int threshold = ldrCalibrated ? ldrThresholds[i] : LIMITE_LDR_PADRAO;
     
     if (leitura > threshold) {
        estadosLDR[i] = 0; // Código original dizia: Maior que limite = 0 (Flexionado/Coberto? Ou inversão logica?)
        // Ajuste aqui se necessário: Normalmente Luz Alta = Esticado.
     } else {
        estadosLDR[i] = 1;
     }
  }
}

// --- LOOP PRINCIPAL ---
void loop() {
  // 1. Ler Sensores Imediatos
  lerLDRs();
  
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  // Aplica offset simples
  float ax = a.acceleration.x - offset_ax;
  float ay = a.acceleration.y - offset_ay;
  
  // Calcula magnitude para ver se é início de um gesto
  float accelMovimento = sqrt(ax * ax + ay * ay);
  bool movimentoDetectado = (accelMovimento > LIMITE_DISPARO_ACEL);

  // 2. Se houver movimento, capturar o buffer (bloqueante por ~1.5s)
  if (movimentoDetectado) {
     Serial.println(">>> Movimento iniciado! Capturando buffer...");
     for(int k=0; k < TAMANHO_GESTO; k++) {
        mpu.getEvent(&a, &g, &temp);
        bufferEnvio[k] = a.acceleration.x - offset_ax;
        delay(DELAY_AMOSTRAGEM); 
     }
  }

  // 3. Enviar dados para o Raspberry Pi
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverPi);
    http.addHeader("Content-Type", "application/json");

    // Cria JSON com capacidade suficiente
    DynamicJsonDocument docEnvio(2048);
    
    // Adiciona array de LDRs
    JsonArray ldrArray = docEnvio.createNestedArray("ldrs");
    for(int i=0; i<5; i++) ldrArray.add(estadosLDR[i]);

    // Flag de movimento
    docEnvio["movimento"] = movimentoDetectado;

    // Adiciona buffer de aceleração (apenas se houve movimento, senão envia null ou array vazio)
    if (movimentoDetectado) {
      JsonArray accArray = docEnvio.createNestedArray("buffer_accel");
      for(int i=0; i<TAMANHO_GESTO; i++) accArray.add(bufferEnvio[i]);
    }

    String requestBody;
    serializeJson(docEnvio, requestBody);

    // Envia POST
    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      String response = http.getString();
      // O Raspberry responde: {"letra": "A"} ou {"letra": "Z"}
      
      DynamicJsonDocument docResp(256);
      DeserializationError error = deserializeJson(docResp, response);

      if (!error) {
        const char* l = docResp["letra"];
        if (l && strlen(l) > 0) {
          letraReconhecida = l[0];
          Serial.print("Pi Reconheceu: "); Serial.println(letraReconhecida);
        }
      } else {
        Serial.print("Erro JSON Pi: "); Serial.println(error.c_str());
      }
    } else {
      Serial.print("Erro HTTP POST: "); Serial.println(httpResponseCode);
    }
    http.end(); // Libera recursos
  } else {
    Serial.println("WiFi Desconectado!");
  }

  // 4. Atender requisições da Aplicação Web
  server.handleClient();
  
  // Pequeno delay para estabilidade e não saturar a rede
  delay(100); 
}