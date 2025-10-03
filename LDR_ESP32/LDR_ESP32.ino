//struct chave-valor para a leitura do LDR
struct ChaveValor{
  byte chave[5];
  char valor;
};

ChaveValor Alfabeto[27] = {
  {{0,0,0,0,1}, 'A'}, 
  {{1,1,1,1,0}, 'B'}, 
  {{0,0,0,0,0}, 'C'}, 
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

const int LDR1 = 33;
const int LDR2 = 32;
const int LDR3 = 35;
const int LDR4 = 34;
//const int LDR5 = 36; //pino SVP
byte Saida[5];

const int LIMITE = 4000;

//comparação da saída dos LDR com a chave das letras do alfabeto
bool comparaArrays(byte arr1[], byte arr2[], int tamanho_chave){
  for(int i=0; i < tamanho_chave; i++){
    if(arr1[i] != arr2[i])
      return false;
  }
  return true;
}


void setup() {
  Serial.begin(115200);
}

// the loop function runs over and over again forever
void loop() {
  int valor_LDR1 = analogRead(LDR1);
  int valor_LDR2 = analogRead(LDR2);
  int valor_LDR3 = analogRead(LDR3);
  int valor_LDR4 = analogRead(LDR4);
  int valor_LDR5 = analogRead(LDR5);
  //METRICA PARA DECIDIR QUANDO DEDO ESTA ABAIXADO OU LEVANTADO (0 OU 1)
  (valor_LDR1 > LIMITE) ? Saida[0] = 0 : Saida[0] = 1;
  (valor_LDR2 > LIMITE) ? Saida[1] = 0 : Saida[1] = 1;
  (valor_LDR3 > LIMITE) ? Saida[2] = 0 : Saida[2] = 1;
  (valor_LDR4 > LIMITE) ? Saida[3] = 0 : Saida[3] = 1;
  //(valor_LDR5 > LIMITE) ? Saida[4] = 0 : Saida[4] = 1;
  Serial.print(valor_LDR1);
  Serial.print("\n");
  Serial.print(valor_LDR2);
  Serial.print("\n");
  Serial.print(valor_LDR3);
  Serial.print("\n");
  Serial.print(valor_LDR4);
  Serial.print("\n");
  //Serial.print(valor_LDR5);
  Serial.print("\n");
  delay(1000);
}
