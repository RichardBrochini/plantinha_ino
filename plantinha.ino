#include <SoftwareSerial.h>
#include "DHT.h"

SoftwareSerial espSerial(11, 10); // RX, TX

//colocar o ssid e senha do wifi
const char* ssid    = "";
const char* pass    = "";

//colocar o id do painel
const String idpainel = "idpainel";

//colocar o endereço da api de chamada
const String http_api = "http_api";

int tentativas =0;

//variaveis de medida dos sensores
String request = "";
String srtTemp = "";
String srtUm   = "";
String srtSolo = "";
String strLuzVioleta = "";
String strLuz = "";
String acao   = "";
String retornoWeb = "";

//endereço da API
const char* API_SERVER = "API_SERVER";

//variaveis para verificar retorno das resposta da pota serial do wi-fi
char* findLINK = (char*) "Linked";
char* findOK = (char*) "OK";
char* findMolhar = (char*) "molhar";
char* findRY = (char*) "ready";
char* findGT = (char*) ">";
char* findDP = (char*) ":";
char* findHD = (char*) "\r\n\r\n";
char* findBT = (char*) "\r\n";

//Define confs sensor de umidade e temperatura
const int pino_dht = 8; // pino onde o sensor DHT está conectado
float temperatura; // variável para armazenar o valor de temperatura
float umidade; // variável para armazenar o valor de umidade
DHT dht(pino_dht, DHT11); // define o pino e o tipo de DHT


// Define pino do sensor de solo
const int PINO_SENSOR = A0;

//Declaracao da variavel que armazena as leituras do sensor de solo
int leitura_sensor_solo = 0;

//define pino do rele da bomba
const int PINO_RELE = 4;

//define rele da luz
const int PINO_RELE_LUZ = 2;

const int pinoLDR = A1; // pino onde o LDR está conectado
int leituraLuz = 0; // variável para armazenar o valor lido pelo ADC
float tensaoLuz = 0.0; // variável para armazenar a tensão


void setup() {

  //seta velocidade de retorno de dados da porta serial e do console serial
  Serial.begin(9600);
  espSerial.begin(9600);
  
  dht.begin(); // inicializa o sensor DHT

  // configura o pino com LDR como entrada  
  pinMode(pinoLDR, INPUT);
  
   //Define o pino conectado ao sensor como uma entrada do sistema
  pinMode(PINO_SENSOR, INPUT);
  
  //Define o pino conectado ao rele como uma saida do sistema
  pinMode(PINO_RELE, OUTPUT);

  //Inicia o pino conectado ao rele da bomba com nivel logico baixo
  digitalWrite(PINO_RELE, LOW);

  //Define o pino conectado ao rele da LUZ como uma saida do sistema
  pinMode(PINO_RELE_LUZ, OUTPUT);

  //Inicia o pino conectado ao rele da LUZ com nivel logico baixo
  digitalWrite(PINO_RELE_LUZ, HIGH);
  
  
}

void loop() { // loop principal
 resetESP();
 
 //coleta dados do sensor de solo
 leitura_sensor_solo = analogRead(PINO_SENSOR);
 
 //dados do sensor de luz
   leituraLuz = analogRead(pinoLDR);

 //dados do sensro de temperatura
 medirTemperatura();

 srtSolo  = String(leitura_sensor_solo);
 srtUm    = String(umidade);
 srtTemp  = String(temperatura);
 strLuz   = String(leituraLuz);

 tentativas=0;
 while(!connectToWiFi()){  
  tentativas++;
  if(tentativas==5){
      resetESP();
   }
 }
  // formata a url para enviar os dados e receber ação para os atuadores
  request = http_api+"/?t="+srtTemp+"&u="+srtUm+"&s="+srtSolo+"&v="+strLuzVioleta+"&l="+strLuz+"&acao="+acao+"&p="+idpainel;
  Serial.println(request);
  httpRequest(request);  
  delay(10000);
}

bool connectToWiFi() {
  
  espSerial.begin(9600);
  espSerial.setTimeout(3000);

  while (espSerial.available()) {
    Serial.write(espSerial.read());
   }
   
    Serial.println(F("[ESP] procurando wifi"));
    Serial.println(F("[ESP] Conectando na rede"));
    espSerial.print(F("AT+CWJAP=\""));
    espSerial.print(ssid);
    espSerial.print(F("\",\""));
    espSerial.print(pass);
    espSerial.println("\"");
    Serial.println(espSerial.readString());
    Serial.println("leu retorno");
    if (!espSerial.find(findOK)) {
      Serial.println(F("[ESP] WiFi falhou tentar de novo"));
      return false;
   }
  Serial.println(F("[ESP] WiFi esta ativa"));
  return true;
}

void medirTemperatura(){
    // A leitura da temperatura ou umidade pode levar 250ms
  // O atraso do sensor pode chegar a 2 segundos
  temperatura = dht.readTemperature(); // lê a temperatura em Celsius
  umidade = dht.readHumidity(); // lê a umidade
  Serial.print(dht.readTemperature());
  // Se ocorreu alguma falha durante a leitura
  if (isnan(umidade) || isnan(temperatura)) {
    Serial.println("Falha na leitura do Sensor DHT!");
  }
  else { // Se não
    // Imprime o valor de temperatura  
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.print(" *C ");
    
    Serial.print("\t"); // tabulação
  
    // Imprime o valor de umidade
    Serial.print("Umidade: ");
    Serial.print(umidade);
    Serial.print(" %\t"); 
    
    Serial.println(); // nova linha
  }
}

bool httpRequest(String site) {

  int cmdLength = 44 + site.length() + strlen(API_SERVER);

  espSerial.print(F("AT+CIPSTART=\"TCP\",\""));
  espSerial.print(API_SERVER);
  espSerial.println(F("\",80"));
  
  validaDecisaoSErvidor(espSerial.readString());
  
  espSerial.print(F("AT+CIPSEND="));
 espSerial.println(cmdLength);
  if (!espSerial.find(findGT)) {
    Serial.println(F("[ESP] Erro no teste de envio")); return false;
  }

  espSerial.print(F("GET "));
  espSerial.print(site);
  espSerial.print(F(" HTTP/1.0\r\n"));
  espSerial.print(F("Host: "));
  espSerial.print(API_SERVER);
  espSerial.print(F("\r\n"));
  espSerial.print(F("Connection: close\r\n"));
  espSerial.println();
  
  if (!espSerial.find(findDP)) {
    Serial.println(F("Nao foi possivel enviar os dados"));
    espSerial.print(F("AT+CIPCLOSE"));
    return false;
  }
  
  validaDecisaoSErvidor(espSerial.readString());
  return true;
}

bool validaDecisaoSErvidor(String data){
  Serial.println("leu o retorno");
  retornoWeb = data;
  String codigoWeb = codigoWebServer(retornoWeb);
  if(codigoWeb=="41"){
         if(acao=""){ 
            digitalWrite(PINO_RELE, HIGH);
            acao = "agua";
            delay(2000);
            digitalWrite(PINO_RELE, LOW);
         }
         return true;
  }
  if(codigoWeb=="40"){
        digitalWrite(PINO_RELE, LOW);
        acao = "";
        return true;
  }

  if(codigoWeb=="31"){
        digitalWrite(PINO_RELE_LUZ, HIGH);
        acao = "";
        return true;
  }

  if(codigoWeb=="30"){
        digitalWrite(PINO_RELE_LUZ, LOW);
        acao = "";
        return true;
  }
  return false; 
  Serial.println("Pega o retorno");
}
String codigoWebServer(String data){
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;
  int continuar = 0;
  String retorno = "-1";
  for(int i=0; i<=maxIndex && found<=maxIndex; i++){
    if(data.charAt(i)=='4'){
        retorno = (String) data.charAt(i);
        retorno += (String) data.charAt(i+1);
    }     
    if(data.charAt(i)=='3'){
        retorno = (String) data.charAt(i);
        retorno += (String) data.charAt(i+1);
    }    
  }
  Serial.println(retorno);
  return retorno;
}

void resetESP() {
  Serial.println("resetar ESP");
  espSerial.println("AT+RST");
  espSerial.println("AT+CWMODE=1");
  delay(1000);
  if(espSerial.find("OK") ) Serial.println("Modulo Resetado");
}
