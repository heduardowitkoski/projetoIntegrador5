/*
==================================================================================
Código Final para ESP32 - Leitura de Sensores e Servidor Web
==================================================================================
Descrição:
Este código combina a sua lógica de leitura dos sensores LDR com o servidor web.
- Conecta-se à rede Wi-Fi.
- Lê os valores dos 5 sensores LDR continuamente.
- Compara os valores lidos com a tabela de gestos (Alfabeto).
- Quando o App Inventor faz uma requisição, o ESP32 envia a última letra
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

// --- SEU CÓDIGO DE LÓGICA DOS SENSORES ---

// Struct chave-valor para a leitura do LDR
struct ChaveValor {
byte chave[5];
char valor;
};

// Tabela de gestos
ChaveValor Alfabeto[26] = {
{{0, 0, 0, 0, 1}, 'A'},
{{1, 1, 1, 1, 0}, 'B'},
{{0, 0, 0, 0, 0}, 'C'}, // Gesto 'C' e 'O' e 'S' estão iguais, revise se necessário
{{0, 0, 0, 1, 0}, 'D'}, // Gesto 'D' e 'G' e 'X' e 'Z' estão iguais
{{0, 0, 0, 0, 0}, 'E'},
{{1, 1, 1, 0, 1}, 'F'},
{{0, 0, 0, 1, 0}, 'G'},
{{0, 0, 1, 1, 0}, 'H'}, // Gesto 'H' e 'N' e 'R' e 'U' e 'V' estão iguais
{{1, 0, 0, 0, 0}, 'I'},
{{1, 0, 0, 0, 0}, 'J'}, // Gesto 'I' e 'J' estão iguais
{{0, 0, 1, 1, 1}, 'K'}, // Gesto 'K' e 'P' estão iguais
{{0, 0, 0, 1, 1}, 'L'},
{{0, 1, 1, 1, 0}, 'M'}, // Gesto 'M' e 'W' estão iguais
{{0, 0, 1, 1, 0}, 'N'},
{{0, 0, 0, 0, 0}, 'O'},
{{0, 0, 1, 1, 1}, 'P'},
{{0, 0, 0, 1, 1}, 'Q'},
{{0, 0, 1, 1, 0}, 'R'},
{{0, 0, 0, 0, 0}, 'S'},
{{1, 1, 1, 0, 0}, 'T'},
{{0, 0, 1, 1, 0}, 'U'},
{{0, 0, 1, 1, 0}, 'V'},
{{0, 1, 1, 1, 0}, 'W'},
{{0, 0, 0, 1, 0}, 'X'},
{{1, 0, 0, 0, 1}, 'Y'},
{{0, 0, 0, 1, 0}, 'Z'}
};


// Pinos dos sensores LDR
const int LDR1 = 33;
const int LDR2 = 32;
const int LDR3 = 35;
const int LDR4 = 34;
const int LDR5 = 39; // Pino SN/ADC1_CH3, bom para leitura analógica
byte Saida[5];

const int LIMITE = 3000; // Ajuste este valor conforme seus testes

// Variável global para armazenar a letra reconhecida
char letraReconhecida = '?';

// Função de comparação (do seu código)
bool comparaArrays(byte arr1[], byte arr2[], int tamanho_chave) {
for (int i = 0; i < tamanho_chave; i++) {
if (arr1[i] != arr2[i])
return false;
}
return true;
}

// --- FUNÇÕES DO SERVIDOR ---

// Esta função será chamada quando o app acessar o endpoint "/gesto"
void handleGesto() {
// Apenas envia a última letra que foi reconhecida no loop principal
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
Serial.println(WiFi.localIP());

// --- Configuração do Servidor ---
server.on("/gesto", HTTP_GET, handleGesto);
server.onNotFound(handleNotFound);
server.begin();
Serial.println("Servidor HTTP iniciado.");
}

// --- LOOP ---
void loop() {
// 1. LER OS SENSORES E DETERMINAR O GESTO
int valor_LDR1 = analogRead(LDR1);
int valor_LDR2 = analogRead(LDR2);
int valor_LDR3 = analogRead(LDR3);
int valor_LDR4 = analogRead(LDR4);
int valor_LDR5 = analogRead(LDR5);

// Converte leitura analógica em binário (0 ou 1)
(valor_LDR1 > LIMITE) ? Saida[0] = 0 : Saida[0] = 1;
(valor_LDR2 > LIMITE) ? Saida[1] = 0 : Saida[1] = 1;
(valor_LDR3 > LIMITE) ? Saida[2] = 0 : Saida[2] = 1;
(valor_LDR4 > LIMITE) ? Saida[3] = 0 : Saida[3] = 1;
(valor_LDR5 > LIMITE) ? Saida[4] = 0 : Saida[4] = 1;

// 2. PROCURAR O GESTO NA TABELA
bool achouLetra = false;
for (int i = 0; i < 26; i++) {
if (comparaArrays(Saida, Alfabeto[i].chave, 5)) {
letraReconhecida = Alfabeto[i].valor;
achouLetra = true;
break; // Para a busca assim que encontrar a primeira correspondência
}
}

// Se não encontrou nenhuma letra correspondente, define como '?'
if (!achouLetra) {
letraReconhecida = '?';
}
// Imprime no Serial para debug
Serial.print("Saida LDRs: [");
Serial.print(Saida[0]); Serial.print(", ");
Serial.print(Saida[1]); Serial.print(", ");
Serial.print(Saida[2]); Serial.print(", ");
Serial.print(Saida[3]); Serial.print(", ");
Serial.print(Saida[4]);
Serial.print("] -> Letra: ");
Serial.println(letraReconhecida);

// 3. PROCESSAR REQUISIÇÕES DO APP
server.handleClient();
delay(500); // Um pequeno delay para não sobrecarregar
}
