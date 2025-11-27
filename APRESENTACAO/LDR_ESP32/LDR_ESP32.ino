#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// --- Configuração do MPU6050 ---
Adafruit_MPU6050 mpu;

// Variáveis para guardar o "erro" de calibração (offsets)
float offset_ax = 0.0, offset_ay = 0.0, offset_az = 0.0;
float offset_gx = 0.0, offset_gy = 0.0, offset_gz = 0.0;

// --- DEFINIÇÕES DE LIMITE (Thresholds) ---
// Ajuste estes valores com testes!
// Quão forte o giroscópio precisa girar para contar como "movimento"
const float LIMITE_MOVIMENTO_GYRO = 80.0; 
// Quão forte o acelerômetro precisa mexer para contar como "movimento"
const float LIMITE_MOVIMENTO_ACEL = 5.0;  
// Limite para detectar a gravidade (para orientação)
const float LIMITE_GRAVIDADE = 7.0;       

// --- Seu Código de LDR ---
//struct chave-valor para a leitura do LDR
struct ChaveValor{
  byte chave[5];
  char valor;
};

// Ordem -> {mindinho, anelar, médio, indicador, polegar}
//         {dedo 1,  dedo2,  dedo3,  dedo4,  dedo 5}
// (Copiado da sua lista, com as duplicatas comentadas removidas)
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
// Calcula o número de letras que temos no array
const int NUM_LETRAS = sizeof(Alfabeto) / sizeof(ChaveValor);

// Pinos LDR
const int LDR1 = 33; // Mindinho
const int LDR2 = 32; // Anelar
const int LDR3 = 35; // Médio
const int LDR4 = 34; // Indicador
const int LDR5 = 36; // Polegar (SVP) - **CORREÇÃO: Descomentei isso**

byte Saida[5];
const int LIMITE_LDR = 4000;

// --- FUNÇÃO DE CALIBRAÇÃO MPU ---
void calibrarMPU() {
  Serial.println("Calibrando MPU... Mantenha o sensor parado na posição de repouso!");
  delay(1000);
  
  long num_leituras = 1000;
  
  for(int i = 0; i < num_leituras; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    offset_ax += a.acceleration.x;
    offset_ay += a.acceleration.y;
    offset_az += a.acceleration.z; // Este vai pegar a gravidade (ex: 10.6)
    
    offset_gx += g.gyro.x;
    offset_gy += g.gyro.y;
    offset_gz += g.gyro.z;
    delay(1);
  }

  offset_ax /= num_leituras;
  offset_ay /= num_leituras;
  // Subtraímos a gravidade (9.81) do offset Z
  // (Baseado nos seus dados de repouso ~10.6)
  offset_az = (offset_az / num_leituras) - 9.81; 
  
  offset_gx /= num_leituras;
  offset_gy /= num_leituras;
  offset_gz /= num_leituras;

  Serial.println("Calibração concluída.");
  Serial.print("Offset Acel: X="); Serial.print(offset_ax);
  Serial.print(", Y="); Serial.print(offset_ay);
  Serial.print(", Z="); Serial.println(offset_az);
}


// --- FUNÇÃO DE SETUP ---
void setup() {
  Serial.begin(115200);
  
  // Inicia o I2C nos pinos corretos (D21, D22)
  Wire.begin(21, 22); 
  
  if (!mpu.begin()) {
    Serial.println("Falha ao encontrar MPU6050!");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Encontrado.");

  // Configura faixas do MPU
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  // Calibra o sensor
  calibrarMPU(); 

  // (Não é necessário definir os pinos LDR como INPUT para analogRead)
  
  Serial.println("Sistema pronto. Começando a leitura...");
}

// --- FUNÇÕES AUXILIARES DE LEITURA ---

// Preenche o array 'Saida' com o estado dos dedos
void lerLDRs() {
  (analogRead(LDR1) > LIMITE_LDR) ? Saida[0] = 0 : Saida[0] = 1;
  (analogRead(LDR2) > LIMITE_LDR) ? Saida[1] = 0 : Saida[1] = 1;
  (analogRead(LDR3) > LIMITE_LDR) ? Saida[2] = 0 : Saida[2] = 1;
  (analogRead(LDR4) > LIMITE_LDR) ? Saida[3] = 0 : Saida[3] = 1;
  (analogRead(LDR5) > LIMITE_LDR) ? Saida[4] = 0 : Saida[4] = 1; // **CORREÇÃO: LDR5 adicionado**
}

// Compara o array de Saida com o Alfabeto e retorna a letra base
char identificarLetraLDR() {
  for(int i = 0; i < NUM_LETRAS; i++) {
    bool match = true;
    for(int j = 0; j < 5; j++) {
      if(Saida[j] != Alfabeto[i].chave[j]) {
        match = false;
        break; // Para de checar esta letra
      }
    }
    // Se todos os 5 dedos bateram, retorna a letra
    if(match) {
      return Alfabeto[i].valor;
    }
  }
  // Se nenhuma letra foi encontrada
  return '?'; 
}


// --- LOOP PRINCIPAL ---
void loop() {
  // 1. LER OS LDRs
  lerLDRs();
  char letraBase = identificarLetraLDR();
  char letraFinal = letraBase; // Por padrão, é a letra base

  // Se a forma dos dedos não foi reconhecida, não faz nada
  if(letraBase == '?') {
    Serial.println("Forma de mão desconhecida.");
    delay(100);
    return;
  }
  
  // 2. LER O MPU6050
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Aplica a calibração (subtrai o erro de repouso)
  float ax = a.acceleration.x - offset_ax;
  float ay = a.acceleration.y - offset_ay;
  float az = a.acceleration.z - offset_az;
  float gx = g.gyro.x - offset_gx;
  float gy = g.gyro.y - offset_gy;
  float gz = g.gyro.z - offset_gz;

  // Calcula a "magnitude" total do movimento do giroscópio
  float gyroMag = sqrt(gx*gx + gy*gy + gz*gz);


  // 3. MÁQUINA DE ESTADOS (DECISÃO)
  // Com base na letra dos LDRs, decidimos se é um movimento ou orientação
  switch (letraBase) {

    case 'I': // Dedo mindinho para cima
      // Ambíguo: 'I' (estático) vs 'J' (movimento de desenhar J)
      if (gyroMag > LIMITE_MOVIMENTO_GYRO) {
        letraFinal = 'J';
      } else {
        letraFinal = 'I';
      }
      break;

    case 'D': // Dedo indicador para cima
      // Ambíguo: 'D' (estático) vs 'Z' (movimento de desenhar Z)
      // 'Z' é um movimento lateral rápido (Aceleração em X)
      if (abs(ax) > LIMITE_MOVIMENTO_ACEL) { 
        letraFinal = 'Z';
      } else {
        letraFinal = 'D';
      }
      break;

    case 'C': // Mão fechada/curvada
      // Ambíguo: 'Ç' (movimento de tremer) vs 'C', 'O', 'S', 'E' (estáticos)
      // 'Ç' é um tremor (Giroscópio alto)
      if (gyroMag > LIMITE_MOVIMENTO_GYRO) {
        letraFinal = 'Ç';
      } else {
        // NOTA: Diferenciar C, O, S e E é MUITO difícil só com MPU.
        // Todos são estáticos e têm orientação similar.
        // Vamos assumir 'C' por enquanto.
        letraFinal = 'C'; 
      }
      break;

    case 'H': // Indicador e médio para cima, juntos
      // Ambíguo: 'H', 'U', 'V', 'N', 'R'
      // 'H' (horizontal) vs 'U'/'V' (vertical)
      // Esta lógica assume que o MPU está no antebraço "deitado" (Z para cima)
      // Mão HORIZONTAL (como 'H'): gravidade age em Z.
      // Mão VERTICAL (como 'U'/'V'): gravidade age em Y.
      // (Baseado no seu setup: Z "para fora" do braço, Y "ao longo" do braço)
      
      if (abs(ay) > LIMITE_GRAVIDADE) { // Mão vertical
        letraFinal = 'U'; // Ou 'V'. Não dá para diferenciar V de U com LDR.
      } 
      else if (abs(az) > LIMITE_GRAVIDADE) { // Mão horizontal
        letraFinal = 'H';
      } 
      else {
        letraFinal = 'H'; // Default
      }
      break;

    // Adicione mais 'case' aqui para outras letras ambíguas (F/T, K/P, M/W)

    default:
      // Se a letra não é ambígua (como 'A', 'B', 'L'), 
      // letraFinal já é a letraBase.
      letraFinal = letraBase;
      break;
  }

  // 4. MOSTRAR O RESULTADO
  Serial.print("Dedos: ");
  Serial.print(letraBase);
  Serial.print(" | MPU diz: ");
  Serial.println(letraFinal);

  delay(100); // Um delay curto para não sobrecarregar o Serial
}