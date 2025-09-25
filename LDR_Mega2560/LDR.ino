//struct chave-valor para a leitura do LDR
struct ChaveValor{
  byte chave[5];
  char valor;
};

ChaveValor Alfabeto[26] = {
  {{0,0,0,0,1}, 'A'}, 
  {{1,1,1,1,0}, 'B'}, 
  {{0,0,0,0,0}, 'C'}, 
  {{0,0,0,0,0}, 'Ç'}, 
  {{0,0,0,1,0}, 'D'}, 
  {{0,0,0,0,0}, 'E'}, 
  {{1,1,1,0,1}, 'F'}, 
  {{0,0,0,1,0}, 'G'}, 
  {{0,0,1,1,0}, 'H'}, 
  {{1,0,0,0,0}, 'I'},
  {{1,0,0,0,0}, 'J'}, 
  {{0,0,1,1,1}, 'K'}, 
  {{0,0,0,1,1}, 'L'}, 
  {{0,1,1,1,0}, 'M'}, 
  {{0,0,1,1,0}, 'N'}, 
  {{0,0,0,0,0}, 'O'}, 
  {{0,0,1,1,1}, 'P'}, 
  {{0,0,0,1,1}, 'Q'}, 
  {{0,0,1,1,0}, 'R'}, 
  {{0,0,0,0,0}, 'S'},
  {{1,1,1,0,0}, 'T'}, 
  {{0,0,1,1,0}, 'U'}, 
  {{0,0,1,1,0}, 'V'}, 
  {{0,1,1,1,0}, 'W'}, 
  {{0,0,0,1,0}, 'X'}, 
  {{1,0,0,0,1}, 'Y'}, 
  {{0,0,0,1,0}, 'Z'}
};

const int LDR1 = A0;
const int LDR2 = A1;
const int LDR3 = A2;
const int LDR4 = A3;
const int LDR5 = A4;
byte Saida[5];

//comparação da saída dos LDR com a chave das letras do alfabeto
bool comparaArrays(byte arr1[], byte arr2[], int tamanho_chave){
  for(int i=0; i < tamanho_chave; i++){
    if(arr1[i] != arr2[i])
      return false;
  }
  return true;
}


void setup() {
  Serial.begin(9600);
}


void loop() {
  //LEITURA DOS PINOS
  int valor_LDR1 = analogRead(LDR1);
  int valor_LDR2 = analogRead(LDR2);
  int valor_LDR3 = analogRead(LDR3);
  int valor_LDR4 = analogRead(LDR4);
  int valor_LDR5 = analogRead(LDR5);
 //METRICA PARA DECIDIR QUANDO DEDO ESTA ABAIXADO OU LEVANTADO (0 OU 1)
  (valor_LDR1 > 1005) ? Saida[0] = 0 : Saida[0] = 1;
  (valor_LDR2 > 1005) ? Saida[1] = 0 : Saida[1] = 1;
  (valor_LDR3 > 1005) ? Saida[2] = 0 : Saida[2] = 1;
  (valor_LDR4 > 1005) ? Saida[3] = 0 : Saida[3] = 1;
  (valor_LDR5 > 1005) ? Saida[4] = 0 : Saida[4] = 1;
  Serial.print("Codigo: ");
  Serial.print(Saida[0]);
  Serial.print(Saida[1]);
  Serial.print(Saida[2]);
  Serial.print(Saida[3]);
  Serial.print(Saida[4]);
  Serial.print("\n=============\n");
  //PRINTA APENAS LETRAS CORRESPONDENTES
  for(int i=0;i<26;i++){
    if(comparaArrays(Saida, Alfabeto[i].chave, 5)){
      Serial.print("Letra: ");
      Serial.print(Alfabeto[i].valor);
      Serial.print("\n");
    }
  }
  delay(1000);
}
