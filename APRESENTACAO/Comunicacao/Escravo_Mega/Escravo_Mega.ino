/*
 * CODIGO PARA O ARDUINO MEGA 2560 (RECEPTOR)
 * Conectar o HC-05 nos pinos 18 (RX1) e 19 (TX1)
 */

void setup() {
  // Inicia o Monitor Serial (comunicação com o PC)
  Serial.begin(115200);
  Serial.println("Mega pronto para receber via Bluetooth...");
  Serial.println("Aguardando conexão do ESP32...");

  // Inicia a Serial1 (comunicação com o HC-05)
  // Certifique-se que o baud rate (9600) é o mesmo do seu HC-05
  Serial1.begin(115200);
}

void loop() {
  // Se houver dados disponíveis no Bluetooth (Serial1)...
  if (Serial1.available()) {
    // Lê a string inteira até encontrar uma nova linha '\n'
    String mensagem = Serial1.readStringUntil('\n');
    
    // Imprime a mensagem recebida no Monitor Serial do PC
    Serial.print("Mensagem recebida do ESP32: ");
    Serial.println(mensagem);
  }
}