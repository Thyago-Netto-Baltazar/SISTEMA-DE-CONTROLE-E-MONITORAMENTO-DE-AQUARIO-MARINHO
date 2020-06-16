/////////////////////////////////////////////////////////////////////
//UNIVERSIDADE FEDERAL DO ABC                                      //
//TRABALHO DE GRADUAÇÃO                                            //
//ENGENHARIA DE INSTRUMENTAÇÃO AUTOMAÇÃO E ROBÓTICA                //
//ORIENTADOR: PROF. DR. FILIPE IEDA FAZANARO                       //
//ALUNOS:                                                          //
//THYAGO NETTO BALTAZAR              RA:11078011                   //
//RENAN TADEU BALDINI DE BRITO       RA:11070113                   //
//                                                                 //
//AUTOMAÇÃO DE AQUÁRIO MARINHO                                     //
/////////////////////////////////////////////////////////////////////

//Bibliotecas usadas
#include <LinkedList.h>
#include <Gaussian.h>
#include <GaussianAverage.h>
#include <NTPClient.h> //Biblioteca do NTP.
#include <WiFi.h> //Biblioteca do WiFi.
#include <LiquidCrystal.h>  //Inclui biblioteca para display lcd
#include <FirebaseESP32.h>
#include "Adafruit_VL53L0X.h"//Inclui biblioteca para sensor de distância Infravermelho


//-------- Configurações de Wi-fi-----------
//const char* ssid = "wi-fi-baldinis";
//const char* password = "10021993alan";
char* ssid = "CiaBaltazar";
char* password = "4815162342";
//-------- Configurações de relógio on-line-----------
WiFiUDP udp;
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000);//Cria um objeto "NTP" com as configurações.

//Definição da pinagem utili ózada no display lcd

//LiquidCrystal lcd(13,12,16,4,2,15); //(RS,EN,D4,D5,D6,D7)
const int rs = 13, en = 12, d4 = 17, d5 =16, d6 = 2, d7 = 15;//Cria constantes do tipo inteiro 
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);//Atribui ao objeto "lcd" constantes as quais definem seus pinos de ligação

//Array simbolo grau
byte grau[8] ={ B00001100,
                B00010010,
                B00010010,
                B00001100,
                B00000000,
                B00000000,
                B00000000,
                B00000000,};

//GPIOs usados
#define sensor_pH 34
#define led 2 // Define o LED ao pino D4.
#define Rele_1 23 //Relé da luminária branca
#define Rele_2 32 //Relé da luminária azul
//NÃO USAR pinos 21 e 22, são usados pelo sensor de distância infravermelho
#define pinNTC 35 // 
#define Rele_8_Resfria 5   //Relé que aciona a bomba do circuito de resfriamento de água
#define Rele_7_Aquece 18  //Relé que aciona a bomba do circuito de aquecimento de água
#define Rele_5_Peltier 19 //Relé que aciona a pastilha Peltier
#define Rele_4 4 //Relé de ajuste de nível
#define pin_sensor 14


//Variáveis globais
String horario;            // Variável que armazenara a hora
//String horario_acion_br; //Horário de acionamento da luminária branca
//String horario_desacion_br; //Horário de desacionamento da luninária branca com valor pré definido
//String option;
float temperatura;
float pH;
float densidade;
boolean luz_branca;
boolean luz_azul;
boolean valor_s1;
String horario_troca_agua;
//String data_troca_agua;

/*Cria duas variaveis, para o host e para a senha do firebase*/
#define Host "https://aquario-ufabc.firebaseio.com/"//seu host
#define Senha_Fire "IRqOpPXfIrRkwTYIH1UlOPehkCcYc72DAX7anFy1"//sua chave do firebase

//Define o número de amostras realizadas para o cálculo da média 
GaussianAverage pH_MediaGausseana(100);
GaussianAverage NTC_MediaGausseana(100);
GaussianAverage Temperatura_MediaGausseana(5);

//Inicializa Sensor de distância infravermelho
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

void setup() {

//Define sensor de nivel como entrada
pinMode(pin_sensor, INPUT);

//Define o sensor de pH como entrada
pinMode(sensor_pH ,INPUT);

// initialize serial communication at 115200 bits per second:
Serial.begin(115200);

//---------------------------------------------------------------------------------------------------
//Menu serial
/*  pinMode(led, OUTPUT);      // Define o pino como saída.
  digitalWrite(led, 0);      // Apaga o LED.
  pinMode(Rele_1, OUTPUT);
  digitalWrite(Rele_1, LOW);
  
Serial.println("Menu: ");
Serial.println("[1] Ajuste de temporização do acionamento da luminária branca");
Serial.println("[2] Ajuste de temporização do desacionamento da luminária branca");
Serial.println("[3] Ajuste de temporização do acionamento da luminária azul");
Serial.println("[4] Ajuste de temporização do desacionamento da luminária azul");
*/
//---------------------------------------------------------------------------------------------------
lcd.begin(40,2); //Define o display com 40 colunas e 2 linhas
//Executa rotina de inicialização
inicializacao();  

//Inicializa conexão WIFI
WiFi.begin(ssid, password);
//Enquanto o status do WIFI for diferente de conectado, escreva na serial "Conectando ao WIFI..."
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.println("Conectando ao WIFI..");
    lcd.clear(); //Limpa o display
    lcd.print("Conectando ao WIFI..");
  }

  //Escreve na serial que o WIFI está conectado
  Serial.println("WIFI Conectado");   
  lcd.clear(); //Limpa o display
  lcd.print("WIFI Conectado!"); 
  delay(1000); 
  lcd.clear(); //Limpa o display

  delay(200);               
  ntp.begin();               // Inicia o NTP.
  ntp.forceUpdate();         // Força o Update.
  Firebase.begin(Host, Senha_Fire);//Conecta-se ao Firebase
  delay(200);
  
//Cria o caractere customizado com o simbolo do grau
lcd.createChar(0, grau);

//Cria condições para executar as leituras do sensor de distância infravermelho
  Serial.begin(9600);

  while (! Serial) {
    delay(1);
  }
  Serial.println("Adafruit VL53L0X test");
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
  Serial.println(F("VL53L0X API Simple Ranging example\n\n"));

  //Setup do módulo de temperatura
  pinMode(pinNTC ,INPUT);
  pinMode(Rele_8_Resfria, OUTPUT);
  digitalWrite(Rele_8_Resfria, HIGH);
  pinMode(Rele_7_Aquece, OUTPUT);
  digitalWrite(Rele_7_Aquece, HIGH);
  pinMode(Rele_5_Peltier, OUTPUT);
  digitalWrite(Rele_5_Peltier, HIGH);
    Serial.begin(115200);

  //Setup módulo Sensor de Nível
  Serial.begin(9600);
  pinMode(pin_sensor, INPUT);

  //Portas Luz OUTPUT
  pinMode(Rele_1, OUTPUT);
  pinMode(Rele_2, OUTPUT);
  
}//Fim do setup

//Funções 
//Lê e retorna variáveis do tipo "string" enviadas pela comunicação serial
String leStringSerial(){
  String conteudo = "";
  char caractere;
  
  // Enquanto receber algo pela serial
  while(Serial.available() > 0) {
    // Lê byte da serial
    caractere = Serial.read();
    // Ignora caractere de quebra de linha
    if (caractere != '\n'){
      // Concatena valores
      conteudo.concat(caractere);
    }
    // Aguarda buffer serial ler próximo caractere
    delay(10);
  }

    //Testa função "leStringSerial"
    
 // Serial.print("Recebi: ");
 //Serial.println(conteudo);
    
  return conteudo;
}//Fim da função leStringSerial

//Função mensagem de inicialização do display lcd
void inicializacao(){
                   lcd.clear(); //Limpa o display
                   lcd.print("UFABC- TG ENGENHARIA IAR/2020");
                   lcd.setCursor(0,2);
                   lcd.print("AUTOMACAO DE AQUARIO MARINHO");
                   delay(3000);
                   lcd.clear(); //Limpa o display
                   lcd.print("Professor Orientador");
                   lcd.setCursor(0,2);
                   lcd.print("Dr. Filipe Ieda Fazanaro");
                   delay(3000);
                   lcd.clear(); //Limpa o display
                   lcd.print("Desenvolvido por...");
                   delay(1000);
                   lcd.clear(); //Limpa o display
                   lcd.print("Thyago Netto Baltazar");
                   lcd.setCursor(0,2);
                   lcd.print("Renan Tadeu Baldini de Brito");
                   delay(3000);
                   lcd.clear(); //Limpa o display
              }// Fim da função de inicialização do lcd


// the loop routine runs over and over showing the voltage on A0

void loop() {

//Loop sensor de nivel
valor_s1 = digitalRead(pin_sensor);
 // Serial.print(valor_s1);
    delay(10);

//Armazena na variável HORA, o horário atual.
  horario = ntp.getFormattedTime();
 // Serial.println(horario); 
 delay(10);

//Lê entrada analógica do sensor de pH
int sensor_pH_Value = analogRead(sensor_pH);
pH_MediaGausseana += sensor_pH_Value;
pH_MediaGausseana.process();

// Convert the analog reading (which goes from 0 - 4095) to a voltage (0 - 3.3V):
 pH = pH_MediaGausseana.mean * (14 / 4095.0);
float pH_semfiltro = sensor_pH_Value * (14 / 4095.0);
// print out the value you read:
/*
//Usado para plotar o gráfico de comparação com o pH usando a Média Gausseana
//Imprime na serial uma tabela com a leitura de cada um dos sensores e o respectivo horário da leitura
//Imprime horário
Serial.print(horario);
Serial.print(" ");
//Serial.print("PH: "); 
Serial.print(pH);
Serial.print(" ");
Serial.print("PH2: ");
Serial.println(pH_semfiltro); 
delay(700);
*/

//variáveis criadas apenas como exemplo para mostrar parametros no display lcd
//float t = 0.00; // valor da temperatura
//float d = 0.00; // valor da densidade
//temperatura = 0.00; //valor da temperatura

int sensorNTC = analogRead(pinNTC);
    NTC_MediaGausseana += sensorNTC;
    NTC_MediaGausseana.process();
    int sensor = NTC_MediaGausseana.mean;
    float tensaoResistor = 0.19 + (sensor*(3.27/4095));// 0.19 é o erro entre tensão física e calculada pelo ESP32, 3.33 o valor de tensão fornecido pelo ESP e 4095 a resolução.
    float corrente = (tensaoResistor/9890.0)*1000;
    float tensaoSensor = 3.27 - (tensaoResistor);
    float resistencia = abs((tensaoSensor/corrente)*1000);
    float Temp = log(resistencia); // Salvamos o Log(resistance) em Temp (de Temporário) para não precisar calcular 4 vezes depois.
    Temp = 1 / (0.00028386514 + ((0.000348586482) * Temp) + (-0.000000186357303 * Temp * Temp * Temp));   // Agora Temp recebe o valor de temperatura calculado por Steinhart-Hart.
    Temp = Temp - 273.15;  // Convertendo Kelvin em Celsius 
    float Temperatura = Temp;
    Temperatura_MediaGausseana += Temperatura;//Realizando a média Gausseana para diminuir flutuações.
    Temperatura_MediaGausseana.process();
    Temp = Temperatura_MediaGausseana.mean; 
    temperatura = Temp;
//    Serial.print(temperatura);
//    Serial.print("Tensão Sensor: ");
//    Serial.print(tensaoSensor);
//    Serial.println(" V");
//    Serial.print("Tensão Resistor: ");
//    Serial.print(tensaoResistor);
//    Serial.println(" V");
//    Serial.print("Resistência: ");
//    Serial.print(resistencia);
//    Serial.println(" ohms");
//    Serial.print("Corrente: ");
//    Serial.print(corrente);
//    Serial.println(" mA");
//    Serial.println("");    
//    Serial.print("Sensor: ");
//    Serial.print(sensor);
//     
//    Serial.println("");
//
//    Serial.print("Temperatura: ");
//    Serial.print(Temp);
//    Serial.println("°C");
//    Serial.println("");    

//Controle de temperatura
      if (Temp >= 28)
        {
          digitalWrite(Rele_7_Aquece, HIGH);//Resfriando a água
          digitalWrite(Rele_8_Resfria, LOW);
          digitalWrite(Rele_5_Peltier, LOW);          
        }
      else if (Temp >= 25)
        {
         digitalWrite(Rele_8_Resfria, HIGH);//Desativa na faixa escolhida
         digitalWrite(Rele_5_Peltier, HIGH);
         digitalWrite(Rele_7_Aquece, HIGH);
        }  
      else
        {
          digitalWrite(Rele_8_Resfria, HIGH);//Aqecendo a temperatura
          digitalWrite(Rele_7_Aquece, LOW);
          digitalWrite(Rele_5_Peltier, LOW);          
        }  

    delay(1000);
    
//Leitura da desnsidade
  VL53L0X_RangingMeasurementData_t measure;
  Serial.print("Reading a measurement… ");
  lox.rangingTest(&measure, false);
    
    float reading = measure.RangeMilliMeter;
    float readingCM = (reading/10);
    //Ultra_MediaGausseana += reading;
    //Ultra_MediaGausseana.process();
    //float reading2 = Ultra_MediaGausseana.mean;
    densidade = ((1000+(30*((readingCM-13.6)/(12.6-13.6)))));
      delay(100);

//Mostrando valores dos parâmetros no display LCD
//Carregango valores nas variaveis para o firebase e display LCD
  Firebase.setFloat("aquario-ufabc/temperatura", temperatura);
  Firebase.setFloat("aquario-ufabc/pH", pH);
  Firebase.setFloat("aquario-ufabc/densidade", densidade);
  
//Firebase.setBool("aquario-ufabc/luz_branca",luz_branca ); //cria variável "luz_branca" no firebase
  luz_branca = Firebase.getBool("aquario-ufabc/luz_branca");//recebe do firebase o valor da variável "luz_branca"
  delay(50);
  digitalWrite(Rele_1, luz_branca );//iguala o estado do relé 1 ao estado da variável  "luz_branca"
  //Serial.print("Luminária branca: ");
 // Serial.print(luz_branca);

  //if(luz_branca == 0){digitalWrite(Rele_1, 0 );}
  //else if(luz_branca == 1){digitalWrite(Rele_1, 1 );}
  
//Firebase.setBool("aquario-ufabc/luz_azul",luz_azul ); //cria variável "luz_azul" no firebase
  luz_azul = Firebase.getBool("aquario-ufabc/luz_azul");//recebe do firebase o valor da variável "luz_azul"
  delay(50);
  digitalWrite(Rele_2, luz_azul);//iguala o estado do relé 2 ao estado da variável  "luz_azul"
  //Serial.print("Luminária azul: ");
  //Serial.print(luz_azul);
  
  Firebase.setBool("aquario-ufabc/nivel",valor_s1 ); //envia para o firebase o valor da variável "valor_s1" para variável "nivel"(do firebase)
 // valor_s1 = Firebase.getBool("aquario-ufabc/nivel");//recebe do firebase o valor da variável "nivel"
  delay(50);
  digitalWrite(Rele_4, valor_s1);//iguala o estado do relé 4 ao estado da variável "valor_s1"
  
  //Data e Horário de Alarme de troca de água
  //Firebase.setString("aquario-ufabc/horario_troca_agua", horario_troca_agua);//envia para o firebase o valor da variável "horario_troca_agua" 
 // Firebase.setString("aquario-ufabc/data_troca_agua", data_troca_agua);      //envia para o firebase o valor da variável 'horario_troca_agua"
  
//Escreve os valores dos parametros no display lcd
lcd.begin(40,2); //Define o display com 40 colunas e 2 linhas
lcd.clear(); //Limpa o display
lcd.setCursor(0,0);
lcd.print("Temp : ");
lcd.print(" ");
lcd.setCursor(7,0);
lcd.print(temperatura,2);
lcd.setCursor(12,0);

//Mostra o simbolo do grau formado pelo array
lcd.write((byte)0);
lcd.setCursor(13,0);
lcd.print("C");

//Mostra o simbolo do grau quadrado
//lcd.print((char)223);

lcd.setCursor(20,0);
lcd.print("pH : ");
lcd.print(" ");
lcd.setCursor(27,0);
lcd.print(pH,2);

lcd.setCursor(0,1);
lcd.print("Dens : ");
lcd.print(" ");
lcd.setCursor(7,1);
lcd.print(densidade,2);
lcd.setCursor(12,1);
lcd.print("g/mL");

lcd.setCursor(20,1);
lcd.print("Hora:");
lcd.print(" ");
lcd.setCursor(27,1);
lcd.print(horario);

/*
//Acionamento das luminárias via serial 
// Se receber algo pela serial
  if (Serial.available() > 0){
    
    // Lê toda string recebida
    int vez = 0;
    String option = leStringSerial();

                                                  while(option == "1"){if (vez != 1) {Serial.println("Digite o horário de acionamento da luminária branca no formato 00:00:00");
                                                                        vez = 1;// colocar aqui o código que quer executar penas uma vez
                                                                                     }              
                                                                        //digitalWrite(led, 1);   // O LED Acende
                                                                        delay(100);
                                                                        
                                                                        if (Serial.available()){
                                                                        horario_acion_br = leStringSerial();
                                                                        Serial.print("Horário de acionamento da luminária branca: ");
                                                                        Serial.println(horario_acion_br);
                                                                        delay(100);
                                                                        return;
                                                                        }
                                                  }                                                                       
                                                  while(option == "2"){if (vez != 1) {Serial.println("Digite o horário de desacionamento da luminária branca no formato 00:00:00");
                                                                       vez = 1;// colocar aqui o código que quer executar penas uma vez
                                                                                     }              
                                                                        //digitalWrite(led, 1);   // O LED Acende
                                                                        delay(100);
                                                                        
                                                                        if (Serial.available()){
                                                                        horario_desacion_br = leStringSerial();
                                                                        Serial.print("Horário de desacionamento da luminária branca: ");
                                                                        Serial.println(horario_desacion_br);
                                                                        delay(100);
                                                                        return;                                                                                                                                              
                                                                        }
                                                  }
                
  }//Fim da verificação serial

//Verifica se o horário de acionamento e desacionamento das funções temporizadas são compatíveis com os respectivos horários estabelecidos pelo usuário
                                  if (horario_acion_br == horario)
                                                            { // Ao atingir o período definido
                                                              digitalWrite(led, 1);   // O LED Acende
                                                              digitalWrite(Rele_1, HIGH);
                                                              //luz_branca = 1;
                                                             // Firebase.setBool("aquario-ufabc/luz_branca",luz_branca );
                                                            }
                                  if (horario_desacion_br == horario)
                                                            { // Ao atingir o período definido
                                                              digitalWrite(led, 0);   // O LED Apaga
                                                              digitalWrite(Rele_1, LOW);
                                                              //luz_branca = 0;
                                                             // Firebase.setBool("aquario-ufabc/luz_branca",luz_branca );
                                                            }

*/
}//Fim do loop
