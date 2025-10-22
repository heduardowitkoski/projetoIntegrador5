#include "BluetoothSerial.h"

// Cria um objeto BluetoothSerial
BluetoothSerial SerialBT;

// --- MUDANÇA 1: Formato do MAC ---
// A função connect() precisa de um array uint8_t, não de uma String.
uint8_t mac_do_hc05[] = {0x98, 0xD3, 0x41, 0xFD, 0x3D, 0x79};

// --- ADIÇÃO 1: PIN ---
// O PIN que descobrimos ser necessário
const char *pin = "1234";

// Variáveis para o timer de envio (não-bloqueante)
unsigned long contador = 0;
unsigned long tempoEnvioAnterior = 0;
const long intervaloEnvio = 5000; // 5 segundos

// Variável para o timer de reconexão
unsigned long tempoReconexaoAnterior = 0;
const long intervaloReconexao = 5000; // Tenta reconectar a cada 5s se cair

/**
 * Função auxiliar para tentar a conexão.
 * Colocamos em uma função para não repetir o código.
 */
bool tentarConectar() {
  Serial.println("Tentando conectar ao HC-05 (MAC: 98:D3:41:FD:3D:79)...");
  
  // Limpa pareamentos antigos (boa prática que ajuda na reconexão)
  SerialBT.unpairDevice(mac_do_hc05); 
  delay(500); // Pequeno delay para a limpeza

  // Tenta conectar ao endereço MAC
  bool conectado = SerialBT.connect(mac_do_hc05);
  
  if(conectado) {
    Serial.println("Conectado ao HC-05 com sucesso!");
  } else {
    Serial.println("Falha ao conectar.");
  }
  return conectado;
}


void setup() {
  // Inicia o Serial (para debug no PC)
  Serial.begin(115200); 

  // --- ADIÇÃO 2: Definir o PIN ---
  // Deve ser feito ANTES de iniciar o Bluetooth
  Serial.println("Definindo o PIN como '1234'...");
  SerialBT.setPin(pin, 4); // 4 é o comprimento de "1234"
  
  // --- MUDANÇA 2: Modo Mestre ---
  // O 'true' é obrigatório para o ESP32 ser o Mestre
  if (!SerialBT.begin("ESP32_Mestre", true)) {
    Serial.println("Falha grave ao iniciar o Bluetooth!");
    return; // Não continua se o BT falhar
  }
  
  Serial.println("ESP32 iniciado em modo Mestre.");

  // Tenta a conexão inicial
  tentarConectar();
  
  // Inicia os timers
  tempoEnvioAnterior = millis();
  tempoReconexaoAnterior = millis();
}

void loop() {
  unsigned long tempoAtual = millis();

  // Se estiver conectado...
  if (SerialBT.connected()) {
    
    // --- MUDANÇA 3: Lógica de envio não-bloqueante ---
    // Verifica se já se passaram 5 segundos desde o último envio
    if (tempoAtual - tempoEnvioAnterior >= intervaloEnvio) {
      tempoEnvioAnterior = tempoAtual; // Reseta o timer de envio
      
      String mensagem = "Olá do ESP32! Contagem: " + String(contador);
      
      // Envia a mensagem com uma nova linha
      SerialBT.println(mensagem); 
      
      Serial.println("Enviado: " + mensagem); // Debug local
      contador++;
    }
    
  } 
  else 
  {
    // Se não estiver conectado, entra na lógica de reconexão
    // Verifica se já se passaram 5 segundos desde a última tentativa
    if (tempoAtual - tempoReconexaoAnterior >= intervaloReconexao) {
      tempoReconexaoAnterior = tempoAtual; // Reseta o timer de reconexão
      
      Serial.println("Desconectado. Tentando reconectar...");
      tentarConectar(); // Chama a função que tenta conectar
    }
  }
  
  // Um pequeno delay no loop é bom para a estabilidade
  delay(20);
}