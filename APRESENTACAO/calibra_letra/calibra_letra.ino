#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

// --- CONFIGURAÇÕES ---
// Aumentado para 40 (captura 2 segundos de movimento)
const int TAMANHO_BUFFER = 30; 
const int DELAY_LOOP = 50; 

// LIMITE DE ACELERAÇÃO (m/s^2)
const float LIMITE_DISPARO_ACEL = 10.0; 

// Variáveis de Offset
float offset_ax = 0, offset_ay = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("Inicializando MPU6050...");
  
  Wire.begin(21, 22); 
  
  if (!mpu.begin()) {
    Serial.println("MPU6050 não encontrado!");
    while (1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("\n=== CALIBRANDO REPOUSO (3s) ===");
  Serial.println("Mantenha a mão parada na posição inicial.");
  delay(1000);
  
  long num_leituras = 500;
  for (int i = 0; i < num_leituras; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    offset_ax += a.acceleration.x;
    offset_ay += a.acceleration.y;
    delay(2);
  }
  
  offset_ax /= num_leituras;
  offset_ay /= num_leituras;

  Serial.println("Calibração OK!");
  Serial.println(">>> FAÇA O GESTO 'Z' (TEMPO: 2 SEGUNDOS) <<<");
  delay(1000);
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float ax = a.acceleration.x - offset_ax;
  float ay = a.acceleration.y - offset_ay;

  // Magnitude do movimento no plano X/Y
  float accelMovimento = sqrt(ax * ax + ay * ay);

  // Debug Visual
  Serial.print("Forca: "); Serial.print(accelMovimento, 1);
  Serial.print(" | Limite: "); Serial.println(LIMITE_DISPARO_ACEL);

  // Gatilho de início
  if (accelMovimento > LIMITE_DISPARO_ACEL) {
    
    Serial.println("\n\n!!! GRAVANDO (2 Segundos)... !!!");
    
    float bufferX[TAMANHO_BUFFER];

    for (int i = 0; i < TAMANHO_BUFFER; i++) {
      mpu.getEvent(&a, &g, &temp);
      
      // Captura eixo X (movimento lateral principal do Z)
      bufferX[i] = a.acceleration.x - offset_ax;

      Serial.print("*"); 
      delay(DELAY_LOOP); 
    }

    Serial.println("\n\n=== COPIE O ARRAY ABAIXO PARA O SEU CÓDIGO ===");
    
    // Imprime a definição completa já com o tamanho 40
    Serial.print("const float MODELO_Z_X[");
    Serial.print(TAMANHO_BUFFER);
    Serial.print("] = {");
    
    for (int i = 0; i < TAMANHO_BUFFER; i++) {
      Serial.print(bufferX[i], 2);
      if (i < TAMANHO_BUFFER - 1) Serial.print(", ");
      
      // Quebra de linha a cada 10 números para facilitar a leitura/cópia
      if ((i + 1) % 10 == 0) Serial.print("\n");
    }
    Serial.println("};");
    
    Serial.println("----------------------------------------------");
    Serial.println("Pausando 4 segundos para respirar...");
    delay(4000); 
  }
  
  delay(50);
}