/*
==================================================================================
BLOCO DE DESCRIÇÃO
==================================================================================
Descrição:
Este código combina a lógica de leitura dos sensores LDR com um servidor web no ESP32.
- Conecta-se à rede Wi-Fi.
- Lê os valores dos 5 sensores LDR continuamente.
- Compara os valores lidos com a tabela de gestos (Alfabeto).
- Quando o App Inventor faz uma requisição, o ESP32 envia a última letra
reconhecida. Se nenhum gesto for reconhecido, envia um "?".
==================================================================================
*/

// ==================================================================================
// BLOCO 1: Inclusão de Bibliotecas
// ==================================================================================
// Bibliotecas necessárias para a funcionalidade Wi-Fi no ESP32.
#include <WiFi.h>
// Biblioteca para criar e gerenciar o servidor web.
#include <WebServer.h>

// ==================================================================================
// BLOCO 2: Configurações de Rede e Servidor
// ==================================================================================
// --- CONFIGURAÇÕES DA REDE WI-FI ---
// Substitua com o nome e a senha da sua rede
const char* ssid = "Henrique204_RCT_2G";
const char* password = "Colorado2002";

// Cria uma instância do servidor web. A porta 80 é a porta padrão para HTTP.
WebServer server(80);

// ==================================================================================
// BLOCO 3: Definição da Lógica de Gestos (Tabela Alfabeto)
// ==================================================================================

// Define uma 'struct' (estrutura de dados) para associar um padrão de 5 sensores
// (a 'chave') a um caractere (o 'valor', ou a letra).
struct ChaveValor {
  byte chave[5]; // Array de 5 bytes (0 ou 1) para o estado dos LDRs
  char valor;    // A letra correspondente
};

// Tabela de gestos (Alfabeto)
// Mapeia os padrões binários dos 5 sensores para as letras do alfabeto.
//
// IMPORTANTE: A lógica de binarização no Bloco 9 (loop) define:
// (valor_LDR > LIMITE) ? Saida[X] = 0 : Saida[X] = 1;
// Isso significa que:
// 0 = Muita Luz (Leitura Alta, LDR exposto)
// 1 = Pouca Luz (Leitura Baixa, LDR coberto/sombra)
/
//
// Duplicatas encontradas:
// C, E, O, S   -> (0,0,0,0,0) -> Sempre será 'C'
// D, G, X, Z   -> (0,0,0,1,0) -> Sempre será 'D'
// H, N, R, U, V -> (0,0,1,1,0) -> Sempre será 'H'
// I, J         -> (1,0,0,0,0) -> Sempre será 'I'
// K, P         -> (0,0,1,1,1) -> Sempre será 'K'
// M, W         -> (0,1,1,1,0) -> Sempre será 'M'
//
// É preciso ajustar os padrões (as 'chaves') para serem únicos para cada letra.
ChaveValor Alfabeto[26] = {
  {{0, 0, 0, 0, 1}, 'A'}, // Dedo 5 (polegar?) coberto, outros expostos
  {{1, 1, 1, 1, 0}, 'B'}, // Dedos 1-4 cobertos, dedo 5 exposto
  {{0, 0, 0, 0, 0}, 'C'}, // Mão aberta (todos expostos à luz)
  {{0, 0, 0, 1, 0}, 'D'},
  {{0, 0, 0, 0, 0}, 'E'}, // Duplicata de 'C'
  {{1, 1, 1, 0, 1}, 'F'},
  {{0, 0, 0, 1, 0}, 'G'}, // Duplicata de 'D'
  {{0, 0, 1, 1, 0}, 'H'},
  {{1, 0, 0, 0, 0}, 'I'},
  {{1, 0, 0, 0, 0}, 'J'}, // Duplicata de 'I'
  {{0, 0, 1, 1, 1}, 'K'},
  {{0, 0, 0, 1, 1}, 'L'},
  {{0, 1, 1, 1, 0}, 'M'},
  {{0, 0, 1, 1, 0}, 'N'}, // Duplicata de 'H'
  {{0, 0, 0, 0, 0}, 'O'}, // Duplicata de 'C'
  {{0, 0, 1, 1, 1}, 'P'}, // Duplicata de 'K'
  {{0, 0, 0, 1, 1}, 'Q'},
  {{0, 0, 1, 1, 0}, 'R'}, // Duplicata de 'H'
  {{0, 0, 0, 0, 0}, 'S'}, // Duplicata de 'C'
  {{1, 1, 1, 0, 0}, 'T'},
  {{0, 0, 1, 1, 0}, 'U'}, // Duplicata de 'H'
  {{0, 0, 1, 1, 0}, 'V'}, // Duplicata de 'H'
  {{0, 1, 1, 1, 0}, 'W'}, // Duplicata de 'M'
  {{0, 0, 0, 1, 0}, 'X'}, // Duplicata de 'D'
  {{1, 0, 0, 0, 1}, 'Y'},
  {{0, 0, 0, 1, 0}, 'Z'}  // Duplicata de 'D'
};

// ==================================================================================
// BLOCO 4: Definições de Hardware (Pinos e Limiar)
// ==================================================================================

// Define os pinos de entrada analógica (ADC) onde os LDRs estão conectados.
const int LDR1 = 33;
const int LDR2 = 32;
const int LDR3 = 35;
const int LDR4 = 34;
const int LDR5 = 39; // Pino SN/ADC1_CH3, bom para leitura analógica

// Array para armazenar o estado binário (0 ou 1) dos 5 LDRs após a leitura.
byte Saida[5];

// Limiar (Threshold) de Calibração.
// Leituras analógicas acima deste valor serão '0' (luz), abaixo ou igual '1' (sombra).
// Este valor (3000) é crucial e deve ser ajustado ("calibrado") com base
// na iluminação ambiente e nos resistores usados com os LDRs.
const int LIMITE = 3000;

// ==================================================================================
// BLOCO 5: Variável de Estado Global
// ==================================================================================

// Armazena a última letra reconhecida pelo loop().
// Esta variável é "global" para que a função 'handleGesto()' (do servidor)
// possa acessá-la e enviá-la ao App Inventor quando ele requisitar.
// Inicia com '?' (indefinido/nenhum gesto reconhecido).
char letraReconhecida = '?';

// ==================================================================================
// BLOCO 6: Função Utilitária de Comparação
// ==================================================================================

// Função auxiliar (helper function).
// Compara dois arrays (o array 'Saida' lido dos sensores vs um 'chave' da tabela Alfabeto).
// Retorna 'true' (verdadeiro) se todos os 5 elementos forem idênticos.
// Retorna 'false' (falso) se qualquer elemento for diferente.
bool comparaArrays(byte arr1[], byte arr2[], int tamanho_chave) {
  for (int i = 0; i < tamanho_chave; i++) {
    if (arr1[i] != arr2[i])
      return false; // Encontrou uma diferença, os arrays não são iguais.
  }
  return true; // O loop terminou sem encontrar diferenças, os arrays são iguais.
}

// ==================================================================================
// BLOCO 7: Funções de Callback do Servidor Web (Handlers)
// ==================================================================================
// Handlers são as funções que o servidor executa quando recebe uma requisição.

// --- FUNÇÕES DO SERVIDOR ---

// Esta função é executada toda vez que o servidor recebe uma requisição GET
// no endereço IP do ESP32, no caminho (endpoint) "/gesto".
// Ex: http://192.168.1.10/gesto
void handleGesto() {
  // Imprime no Serial Monitor (para debug) que a requisição foi recebida.
  Serial.println("Requisicao /gesto recebida!");
  Serial.print("Enviando letra: ");
  Serial.println(letraReconhecida); // Imprime o valor que será enviado.

  // --- Cabeçalhos CORS (Cross-Origin Resource Sharing) ---
  // Essencial para que um aplicativo web (como o App Inventor em modo 'companion'
  // ou rodando em um navegador) possa fazer requisições para este servidor (o ESP32),
  // que está em uma "origem" (IP) diferente.
  // Permite requisições de qualquer origem (IP/domínio).
  server.sendHeader("Access-Control-Allow-Origin", "*");
  // Permite os métodos HTTP comuns.
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  // -----------------------------------------------------

  // Envia a resposta HTTP de volta para o App Inventor:
  // Código 200 (OK)
  // Tipo de conteúdo "text/plain" (texto puro)
  // O corpo da resposta é a 'letraReconhecida' (convertida para String)
  server.send(200, "text/plain", String(letraReconhecida));
}

// Esta função é executada se o cliente (App Inventor) tentar acessar
// qualquer outro caminho (endpoint) que não seja "/gesto".
// Ex: http://192.168.1.10/outra-coisa
void handleNotFound() {
  server.send(404, "text/plain", "404: Nao encontrado"); // Envia o erro "404 Not Found"
}

// ==================================================================================
// BLOCO 8: Função de Configuração (Setup)
// ==================================================================================
// Esta função é executada apenas uma vez, quando o ESP32 é ligado ou resetado.
void setup() {
  // Inicia a comunicação serial (para debug no Monitor Serial).
  Serial.begin(115200);
  delay(100);

  // --- Sub-bloco 8.1: Conexão Wi-Fi ---
  Serial.println("\nIniciando...");
  WiFi.begin(ssid, password); // Tenta se conectar à rede Wi-Fi definida no Bloco 2.
  Serial.print("Conectando a rede Wi-Fi");

  // Loop de espera: Fica "preso" aqui (imprimindo ".") até que
  // o status da conexão seja "Conectado".
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Confirmação de conexão e impressão do IP (IMPORTANTE para o App Inventor).
  Serial.println("\nConectado com sucesso!");
  Serial.print("Endereco de IP do ESP32: ");
  Serial.println(WiFi.localIP()); // Este IP deve ser usado no App Inventor.

  // --- Sub-bloco 8.2: Configuração das Rotas do Servidor ---
  // Associa o caminho "/gesto" (quando acessado via método GET) à função 'handleGesto'.
  server.on("/gesto", HTTP_GET, handleGesto);
  // Associa qualquer outro caminho não encontrado à função 'handleNotFound'.
  server.onNotFound(handleNotFound);

  // Inicia o servidor web, que agora está pronto para receber requisições.
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

// ==================================================================================
// BLOCO 9: Função de Loop Principal
// ==================================================================================
// Esta função é executada continuamente, repetindo para sempre após o setup().
void loop() {
  // 1. LER OS SENSORES E DETERMINAR O GESTO

  // --- Sub-bloco 9.1: Leitura Analógica dos Sensores ---
  // Lê a voltagem (valor analógico) de cada pino LDR.
  // O valor no ESP32 varia de 0 (0V) a 4095 (3.3V).
  int valor_LDR1 = analogRead(LDR1);
  int valor_LDR2 = analogRead(LDR2);
  int valor_LDR3 = analogRead(LDR3);
  int valor_LDR4 = analogRead(LDR4);
  int valor_LDR5 = analogRead(LDR5);

  // --- Sub-bloco 9.2: Binarização dos Dados ---
  // Converte a leitura analógica (0-4095) em binária (0 ou 1)
  // usando o 'LIMITE' definido no Bloco 4.
  // (leitura > LIMITE) ? Saida[X] = 0 : Saida[X] = 1;
  // Se a leitura for ALTA (ex: 3500, muita luz), Saida = 0.
  // Se a leitura for BAIXA (ex: 1000, pouca luz/sombra), Saida = 1.
  (valor_LDR1 > LIMITE) ? Saida[0] = 0 : Saida[0] = 1;
  (valor_LDR2 > LIMITE) ? Saida[1] = 0 : Saida[1] = 1;
  (valor_LDR3 > LIMITE) ? Saida[2] = 0 : Saida[2] = 1;
  (valor_LDR4 > LIMITE) ? Saida[3] = 0 : Saida[3] = 1;
  (valor_LDR5 > LIMITE) ? Saida[4] = 0 : Saida[4] = 1;

  // 2. PROCURAR O GESTO NA TABELA

  // --- Sub-bloco 9.3: Reconhecimento do Gesto ---
  bool achouLetra = false; // Flag (bandeira) para rastrear se encontramos uma correspondência.
  // Itera (passa por) toda a tabela 'Alfabeto' (do índice 0 ao 25).
  for (int i = 0; i < 26; i++) {
    // Usa a função do Bloco 6 para comparar a 'Saida' (leitura atual)
    // com a 'chave' da letra na posição 'i' da tabela.
    if (comparaArrays(Saida, Alfabeto[i].chave, 5)) {
      letraReconhecida = Alfabeto[i].valor; // Atualiza a variável global (Bloco 5).
      achouLetra = true;                     // Marca que encontramos.
      break; // Interrompe o loop 'for'. Já achamos a letra, não precisa procurar mais.
             // (É por isso que as duplicatas só retornam a primeira que encontram)
    }
  }

  // --- Sub-bloco 9.4: Tratamento de Gesto Não Reconhecido ---
  // Se o loop 'for' terminou e 'achouLetra' ainda é 'false',
  // significa que o padrão lido (array 'Saida') não corresponde a nenhuma
  // 'chave' na tabela 'Alfabeto'.
  if (!achouLetra) {
    letraReconhecida = '?'; // Define a saída global como '?'
  }

  // --- Sub-bloco 9.5: Debugging Serial ---
  // Imprime o estado binário lido e a letra resultante (ou '?')
  // no Monitor Serial. Essencial para calibrar o 'LIMITE' e testar os gestos.
  Serial.print("Saida LDRs: [");
  Serial.print(Saida[0]); Serial.print(", ");
  Serial.print(Saida[1]); Serial.print(", ");
  Serial.print(Saida[2]); Serial.print(", ");
  Serial.print(Saida[3]); Serial.print(", ");
  Serial.print(Saida[4]);
  Serial.print("] -> Letra: ");
  Serial.println(letraReconhecida);

  // 3. PROCESSAR REQUISIÇÕES DO APP

  // --- Sub-bloco 9.6: Processamento do Servidor Web ---
  // ESSENCIAL: Esta função verifica se algum cliente (App Inventor) fez uma
  // requisição HTTP ao servidor. Se fez, o servidor processa essa requisição
  // (executando 'handleGesto' ou 'handleNotFound', conforme a rota).
  server.handleClient();

  // --- Sub-bloco 9.7: Delay ---
  // Pausa a execução do loop por 500ms (meio segundo).
  // Isso diminui a taxa de leitura dos sensores e dá tempo ao ESP32
  // para lidar com as tarefas de rede (Wi-Fi e Servidor) sem sobrecarga.
  delay(500);
}

