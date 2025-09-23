//struct chave-valor para a leitura do LDR
struct ChaveValor{
  byte chave[5];
  char valor;
};

ChaveValor Alfabeto[26] = {
  {{1,1,1,1,0}, 'A'}, 
  {{0,0,0,0,1}, 'B'}, 
  {{1,1,1,1,1}, 'C'}, 
  {{1,1,1,0,1}, 'D'}, 
  {{1,1,1,1,1}, 'E'}, 
  {{0,0,0,1,0}, 'F'}, 
  {{1,1,1,0,1}, 'G'}, 
  {{1,1,0,0,1}, 'H'}, 
  {{0,1,1,1,1}, 'I'},
  {{0,1,1,1,1}, 'J'}, 
  {{0,0,1,1,1}, 'K'}, 
  {{1,1,1,0,0}, 'L'}, 
  {{1,0,0,0,1}, 'M'}, 
  {{1,1,0,0,1}, 'N'}, 
  {{1,1,1,1,1}, 'O'}, 
  {{1,1,0,0,0}, 'P'}, 
  {{1,1,1,0,1}, 'Q'}, 
  {{1,1,0,0,1}, 'R'}, 
  {{1,1,1,1,1}, 'S'},
  {{0,0,0,1,1}, 'T'}, 
  {{1,1,0,0,0}, 'U'}, 
  {{1,1,0,0,0}, 'V'}, 
  {{1,0,0,0,1}, 'W'}, 
  {{1,1,1,0,1}, 'X'}, 
  {{0,1,1,1,0}, 'Y'}, 
  {{1,1,1,0,1}, 'Z'}
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

// the loop function runs over and over again forever
void loop() {
  int valor_LDR1 = analogRead(LDR1);
  int valor_LDR2 = analogRead(LDR2);
  int valor_LDR3 = analogRead(LDR3);
  int valor_LDR4 = analogRead(LDR4);
  int valor_LDR5 = analogRead(LDR5);
  (valor_LDR1 > 900) ? Saida[0] = 0 : Saida[0] = 1;
  (valor_LDR2 > 900) ? Saida[1] = 0 : Saida[1] = 1;
  (valor_LDR3 > 900) ? Saida[2] = 0 : Saida[2] = 1;
  (valor_LDR4 > 900) ? Saida[3] = 0 : Saida[3] = 1;
  (valor_LDR5 > 900) ? Saida[4] = 0 : Saida[4] = 1;
  Serial.print("Codigo: ");
  Serial.print(Saida[0]);
  Serial.print(Saida[1]);
  Serial.print(Saida[2]);
  Serial.print(Saida[3]);
  Serial.print(Saida[4]);
  Serial.print("\n=============\n");
  for(int i=0;i<26;i++){
    if(comparaArrays(Saida, Alfabeto[i].chave, 5)){
      Serial.print("Letra: ");
      Serial.print(Alfabeto[i].valor);
      Serial.print("\n");
    }
  }
  delay(3000);
}
