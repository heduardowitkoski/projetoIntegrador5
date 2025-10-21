/*
  ==================================================================================
  Código Final para ESP32 - Leitura de Sensores e Servidor Web
  ==================================================================================
  Descrição:
  Este código está pronto para funcionar com a aplicação web final.
  - Conecta-se à rede Wi-Fi.
  - Lê os valores dos 5 sensores LDR continuamente.
  - Compara os valores lidos com a tabela de gestos (com duplicatas removidas).
  - Quando a aplicação web faz uma requisição, o ESP32 envia a última letra
    reconhecida. Se nenhum gesto for reconhecido, envia um "?".
  ==================================================================================
*/

// Bibliotecas para Wi-Fi e Servidor Web
#include <WiFi.h>
#include <WebServer.h>

// --- CONFIGURAÇÕES DA REDE WI-FI ---
// Substitua com o nome e a senha da sua rede
const char* ssid = "NOME_DA_SUA_REDE_WIFI";
const char* password = "SENHA_DA_SUA_REDE_WIFI";

// Cria uma instância do servidor na porta 80
WebServer server(80);

// --- LÓGICA DOS SENSORES ---

// Struct para associar a leitura binária dos sensores a uma letra
struct ChaveValor {
  byte chave[5];
  char valor;
};

// Tabela de gestos (com duplicatas comentadas para evitar ambiguidade)
// ATENÇÃO: É crucial calibrar estes valores com a luva real para obter precisão.
ChaveValor Alfabeto[26] = {
  {{0, 0, 0, 0, 1}, 'A'},
  {{1, 1, 1, 1, 0}, 'B'},
  {{0, 0, 0, 0, 0}, 'C'}, 
  {{0, 0, 0, 1, 0}, 'D'}, 
  //{{0, 0, 0, 0, 0}, 'E'}, // Duplicado de 'C'
  {{1, 1, 1, 0, 1}, 'F'},
  //{{0, 0, 0, 1, 0}, 'G'}, // Duplicado de 'D'
  {{0, 0, 1, 1, 0}, 'H'}, 
  {{1, 0, 0, 0, 0}, 'I'},
  //{{1, 0, 0, 0, 0}, 'J'}, // Duplicado de 'I'
  {{0, 0, 1, 1, 1}, 'K'}, 
  {{0, 0, 0, 1, 1}, 'L'},
  {{0, 1, 1, 1, 0}, 'M'}, 
  //{{0, 0, 1, 1, 0}, 'N'}, // Duplicado de 'H'
  //{{0, 0, 0, 0, 0}, 'O'}, // Duplicado de 'C'
  //{{0, 0, 1, 1, 1}, 'P'}, // Duplicado de 'K'
  {{0, 0, 0, 1, 1}, 'Q'},
  //{{0, 0, 1, 1, 0}, 'R'}, // Duplicado de 'H'
  //{{0, 0, 0, 0, 0}, 'S'}, // Duplicado de 'C'
  {{1, 1, 1, 0, 0}, 'T'},
  //{{0, 0, 1, 1, 0}, 'U'}, // Duplicado de 'H'
  //{{0, 0, 1, 1, 0}, 'V'}, // Duplicado de 'H'
  //{{0, 1, 1, 1, 0}, 'W'}, // Duplicado de 'M'
  //{{0, 0, 0, 1, 0}, 'X'}, // Duplicado de 'D'
  {{1, 0, 0, 0, 1}, 'Y'},
  //{{0, 0, 0, 1, 0}, 'Z'}  // Duplicado de 'D'
};


// Pinos dos sensores LDR
const int LDR1 = 33;
const int LDR2 = 32;
const int LDR3 = 35;
const int LDR4 = 34;
const int LDR5 = 39; // Pino SN/ADC1_CH3
byte Saida[5];

// Valor de corte para decidir se o dedo está dobrado ou esticado.
// Ajuste este valor conforme os testes com a luva real.
const int LIMITE = 3000; 

// Variável global para armazenar a letra reconhecida
char letraReconhecida = '?';

// Função para comparar a leitura atual dos sensores com um gesto da tabela
bool comparaArrays(byte arr1[], byte arr2[], int tamanho_chave) {
  for (int i = 0; i < tamanho_chave; i++) {
    if (arr1[i] != arr2[i])
      return false;
  }
  return true;
}

// --- FUNÇÕES DO SERVIDOR ---

// Esta função é chamada quando a aplicação web acessa o endpoint "/gesto"
void handleGesto() {
  // Envia a última letra que foi reconhecida no loop principal como resposta
  server.send(200, "text/plain", String(letraReconhecida));
}

// Função para lidar com requisições a endereços não encontrados
void handleNotFound() {
  server.send(404, "text/plain", "404: Nao encontrado");
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  delay(100);

  // --- Conexão Wi-Fi ---
  Serial.println("\nIniciando...");
  WiFi.begin(ssid, password);
  Serial.print("Conectando a rede Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado com sucesso!");
  Serial.print("Endereco de IP do ESP32: ");
  Serial.println(WiFi.localIP()); // Este é o IP que você deve usar na aplicação web

  // --- Configuração do Servidor ---
  server.on("/gesto", HTTP_GET, handleGesto);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

// --- LOOP PRINCIPAL ---
void loop() {
  // 1. LER OS SENSORES E DETERMINAR O GESTO
  int valor_LDR1 = analogRead(LDR1);
  int valor_LDR2 = analogRead(LDR2);
  int valor_LDR3 = analogRead(LDR3);
  int valor_LDR4 = analogRead(LDR4);
  int valor_LDR5 = analogRead(LDR5);

  // Converte a leitura analógica em binário (0 ou 1)
  // 0 = dedo esticado (baixa resistência do LDR, alta leitura)
  // 1 = dedo dobrado (alta resistência do LDR, baixa leitura)
  (valor_LDR1 > LIMITE) ? Saida[0] = 0 : Saida[0] = 1;
  (valor_LDR2 > LIMITE) ? Saida[1] = 0 : Saida[1] = 1;
  (valor_LDR3 > LIMITE) ? Saida[2] = 0 : Saida[2] = 1;
  (valor_LDR4 > LIMITE) ? Saida[3] = 0 : Saida[3] = 1;
  (valor_LDR5 > LIMITE) ? Saida[4] = 0 : Saida[4] = 1;

  // 2. PROCURAR O GESTO NA TABELA
  bool achouLetra = false;
  for (int i = 0; i < 26; i++) {
    // Ignora as linhas que foram comentadas (o valor será nulo)
    if (Alfabeto[i].valor == 0) continue;

    if (comparaArrays(Saida, Alfabeto[i].chave, 5)) {
      letraReconhecida = Alfabeto[i].valor;
      achouLetra = true;
      break; // Para a busca assim que encontrar a primeira correspondência
    }
  }

  // Se, após varrer toda a tabela, nenhum gesto corresponder, define como '?'
  if (!achouLetra) {
    letraReconhecida = '?';
  }
  
  // Imprime no Serial Monitor para ajudar na depuração e calibração
  Serial.print("Saida LDRs: [");
  Serial.print(Saida[0]); Serial.print(", ");
  Serial.print(Saida[1]); Serial.print(", ");
  Serial.print(Saida[2]); Serial.print(", ");
  Serial.print(Saida[3]); Serial.print(", ");
  Serial.print(Saida[4]);
  Serial.print("] -> Letra: ");
  Serial.println(letraReconhecida);

  // 3. PROCESSAR REQUISIÇÕES DA APLICAÇÃO WEB
  server.handleClient();
  
  delay(200); // Um pequeno delay para estabilizar as leituras
}
