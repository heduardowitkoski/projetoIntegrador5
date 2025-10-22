/*
 * CODIGO FINAL PARA O ESP32 (TRANSMISSOR BLE)
 * Com os UUIDs corretos para o seu módulo.
 */
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// O MAC Address BLE do seu módulo
static BLEAddress *pServerAddress = new BLEAddress("2a:41:cc:b7:34:d7");

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// UUIDS CORRETOS (que você encontrou no nRF Connect)
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
static BLEUUID serviceUUID("594a34fc-31db-11ea-978f-2e728ce88125");
static BLEUUID charUUID("594a3010-31db-11ea-978f-2e728ce88125");

static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static boolean deviceFound = false;

// Classe de callback para quando o dispositivo é encontrado no scan
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getAddress().equals(*pServerAddress)) {
      advertisedDevice.getScan()->stop();
      Serial.println("É o nosso dispositivo! Parando o scan.");
      deviceFound = true;
      doConnect = true; // Seta a flag para conectar no loop principal
    }
  }
};

bool connectToServer() {
    Serial.print("Formando conexão com ");
    Serial.println(pServerAddress->toString().c_str());
    
    BLEClient* pClient  = BLEDevice::createClient();
    Serial.println(" - Cliente BLE criado");

    if (!pClient->connect(*pServerAddress)) {
       Serial.println(" - Conexão falhou");
       return false;
    }
    Serial.println(" - Conectado ao servidor");

    // Tenta obter o serviço (a "pasta")
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Falha ao obter serviço: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Serviço encontrado!"); // <-- ESPERAMOS VER ISSO AGORA

    // Tenta obter a característica (o "arquivo")
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Falha ao obter característica: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Característica encontrada!"); // <-- E ISSO
    connected = true;
    return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Cliente BLE (Versão Final)...");
  BLEDevice::init(""); 

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false); 
  Serial.println("Scan iniciado...");
}

void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("Sucesso! Estamos conectados e prontos para enviar.");
    } else {
      Serial.println("Falha ao conectar. Reiniciando scan...");
      BLEDevice::getScan()->start(5, false); 
    }
    doConnect = false; 
  }

  if (connected) {
    String mensagem = "Ola do ESP32 via BLE!\n"; // Adiciona '\n'
    Serial.print("Enviando: ");
    Serial.print(mensagem.c_str());
    
    // Escreve os dados (sem esperar resposta, é mais rápido)
    pRemoteCharacteristic->writeValue((uint8_t*)mensagem.c_str(), mensagem.length(), false);
    
    delay(3000); // Espera 3 segundos
  } else if (deviceFound == false) {
     Serial.println("Dispositivo não encontrado. Escaneando novamente...");
     BLEDevice::getScan()->start(5, false);
     delay(5000);
  }
}