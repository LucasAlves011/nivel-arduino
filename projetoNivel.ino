#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#include <HCSR04.h>

#define O OUTPUT
#define I INPUT
#define L LOW
#define H HIGH

//PORTAS
#define RELE 32 //31
#define BUZZER 7
#define SENSOR_BUFFER_CIMA 5
#define SENSOR_BUFFER_BAIXO 4
#define ULTRASSONICO_CIMA_TRIG 12
#define ULTRASSONICO_CIMA_ECHO 11
#define LED_BOMBA 52
#define BUFFER_ 48
#define LED_ESPERANDO_BUFFER_ENCHER 51
#define BTN_LIGAR_BOMBA 2
#define BTN_PAUSAR_SISTEMA_AUTOMATICO 3
#define SENSOR_PRESENCA 50

LiquidCrystal_I2C lcd(0x27,16,2);
UltraSonicDistanceSensor ultrassonico(ULTRASSONICO_CIMA_TRIG,ULTRASSONICO_CIMA_ECHO);
RTC_DS3231 rtc;

// (3600 * HORA) + (60 * MINUTO) + SEGUNDO
const long HORARIO_INICIO_PERMITIDO =  ((3600L * 7) + (60L * 0) + 0L); //25.200
const long HORARIO_FIM_PERMITIDO = ((3600L * 23) + (60L * 59) + 0L);  //77.400

//MEDIDAS
#define ALTURA_CAIXA_DAGUA 70 //cm
#define ALTURA_TAMPA_CAIXA 12 //cm

typedef struct Nivel {
  float niveis[10] = {0,0,0,0,0,0,0,0,0,0};
  int index = 0;
}Nivel;

typedef struct SensorNivel {
  bool leituras[10] = {1,1,1,1,1,1,1,1,1,1};
  int index = 0;
  bool statusAtual = true;
}SensorNivel;

Nivel k;
Nivel *ultimosNiveis = &k;
SensorNivel nivel_baixo, nivel_cima;

byte pontos[8] = {B00000, B00101, B00000, B01010, B00000, B00010, B11111};
char daysOfTheWeek[7][12] = {"Domingo","Segunda-Feira","Terça-Feira","Quarta-Feira","Quinta-Feira","Sexta-Feira","Sabado"};
DateTime agora;
void attDataHoraPercentual(DateTime dataHora, int percentualCaixaDagua);

bool statusBomba = true;
float leituraAtualNormalizada = 0;
float leituraAtualBruta = 0;
int percentualCaixaMedia = 0;

bool horarioPermite = false;
bool buffer_cima_normalizado, buffer_baixo_normalizado;
bool esperando_buffer_encher = false;
bool sistema_automatico_pausado = true;

byte partes_grafico[4][8] = {
  {B10000, B10000, B10000, B10000, B10000, B10000, B10000, B10000},
  {B11000, B11000, B11000, B11000, B11000, B11000, B11000, B11000},
  {B11100, B11100, B11100, B11100, B11100, B11100, B11100, B11100},
  {B11110, B11110, B11110, B11110, B11110, B11110, B11110, B11110}
};

void setup() {
  lcd.createChar(1, partes_grafico[0]);  
  lcd.createChar(2, partes_grafico[1]);
  lcd.createChar(3, partes_grafico[2]);
  lcd.createChar(4, partes_grafico[3]); 

  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(BTN_LIGAR_BOMBA),ligarBomba,RISING);
  attachInterrupt(digitalPinToInterrupt(BTN_PAUSAR_SISTEMA_AUTOMATICO),pausarSistemaAutomatico,RISING);

  if(!rtc.begin()){
    Serial.println("Teste");
    Serial.flush();
    abort();
  }
  
  rtc.adjust(DateTime(__DATE__,__TIME__));  //RETIRAR QUANDO FOR PARA PRODUCAO
  pinMode(RELE,O);
  pinMode(BUZZER,O);
  pinMode(LED_BOMBA,O);
  pinMode(SENSOR_BUFFER_BAIXO,I);
  pinMode(SENSOR_BUFFER_CIMA,I);
  pinMode(LED_ESPERANDO_BUFFER_ENCHER,O);
  pinMode(BTN_LIGAR_BOMBA,I);
  pinMode(SENSOR_PRESENCA,I);
  
  // digitalWrite(SENSOR_BUFFER_BAIXO,L);  
  // digitalWrite(SENSOR_BUFFER_CIMA,L);

  
  // digitalWrite(RELE,L);
  // pinMode(11,I);
  digitalWrite(LED_BOMBA,L);
  
  lcd.init();
  lcd.backlight();
  /*lcd.setCursor(0,0);
  lcd.print("Projeto Nivel");
  lcd.setCursor(0,1);
  lcd.print("de Agua"); */
  agora = rtc.now();
   
}

void loop() {  
  
  agora = rtc.now();
  leituraAtualBruta = ultrassonico.measureDistanceCm();
  leituraAtualNormalizada = constrain(leituraAtualBruta,ALTURA_TAMPA_CAIXA,  ALTURA_CAIXA_DAGUA);
  percentualCaixaMedia = 100 - map(mediaDescartadora(leituraAtualNormalizada),ALTURA_TAMPA_CAIXA ,ALTURA_CAIXA_DAGUA, 0, 100);
  /*
  atualizarTela();
  // attDataHoraPercentual(agora,percentualCaixaMedia);
  buffer_baixo_normalizado = leitura2Segundos(digitalRead(SENSOR_BUFFER_BAIXO),&nivel_baixo);
  buffer_cima_normalizado = leitura2Segundos(digitalRead(SENSOR_BUFFER_CIMA),&nivel_cima);

        //BAIXO                        BAIXO
  if((!buffer_baixo_normalizado && !buffer_cima_normalizado) || esperando_buffer_encher) {
    esperando_buffer_encher = !(buffer_cima_normalizado && buffer_baixo_normalizado);     
  }

  digitalWrite(LED_ESPERANDO_BUFFER_ENCHER,esperando_buffer_encher ? H : L);

  if(!sistema_automatico_pausado){
    if((horarioPermiteExecutar(agora) && percentualCaixaMedia < 60 && buffer_cima_normalizado && buffer_baixo_normalizado) || (statusBomba && !esperando_buffer_encher)){

      if(!statusBomba){
        statusBomba = true;
        alarmeEntrada();
      }

      if(percentualCaixaMedia >= 99)
        statusBomba = false;
              
      Serial.print(" -> LIGADO <- ");
      digitalWrite(RELE,LOW);
      digitalWrite(LED_BOMBA,H);
      Serial.print(percentualCaixaMedia);
      Serial.print("%");   
    }else {    
      Serial.print(" -> DESLIGADO <-  ");
      digitalWrite(RELE, HIGH);
      digitalWrite(LED_BOMBA,L);
      Serial.print(percentualCaixaMedia);
      Serial.print("%");
      statusBomba = false;    
    } 
  }

  Serial.print("\n");
  Serial.print("Bruta: ");
  Serial.print(leituraAtualBruta);
  Serial.print(" Normalizada: ");  
  Serial.print(leituraAtualNormalizada);
  Serial.print(" - Media: ");
  Serial.print(mediaDescartadora(leituraAtualNormalizada));
  Serial.print(" - statusBomba: ");
  Serial.print(statusBomba);  
  Serial.print(" | BUFFER_BAIXO: ");
  Serial.print(buffer_baixo_normalizado);
  Serial.print("(");
  Serial.print(digitalRead(SENSOR_BUFFER_BAIXO));
  Serial.print(")");
  Serial.print(" - BUFFER_CIMA: ");
  Serial.print(buffer_cima_normalizado);
  Serial.print("(");
  Serial.print(digitalRead(SENSOR_BUFFER_CIMA));
  Serial.print(")");
  Serial.print(" - BTN_BOMBA ");
  Serial.print(digitalRead(BTN_LIGAR_BOMBA));
  Serial.print(" - STATUS_BOMBA ");
  Serial.print(statusBomba);
  Serial.print(" - Pause ");
  Serial.print(sistema_automatico_pausado);
  */
  
  // Serial.print(" = Leitura 2sec : ");
  // Serial.print(leitura2Segundos(b,&nivel_cima));
  // Serial.print(" ");  
  // grafico();
  bool valor = digitalRead(SENSOR_PRESENCA);
  if(valor){
    Serial.print(" Detectou ");
  }else {
    Serial.print(" --------- ");
  }
  Serial.print("\n");

  delay(200);
}

void ligarBomba(){
  if(!esperando_buffer_encher)
    statusBomba = !statusBomba;
  Serial.print("  - Clicado - ");
}

void pausarSistemaAutomatico(){
  sistema_automatico_pausado = !sistema_automatico_pausado;
  digitalWrite(BTN_PAUSAR_SISTEMA_AUTOMATICO,L);
}

void alarmeEntrada(){   
  for (int i = 0 ; i < 3; i++) {
    Serial.print("\n TOCANDO ALARME...");
    Serial.print(i);    
    tone(BUZZER, 1200); 
    delay(120);
    noTone(BUZZER);
    delay(120);
    tone(BUZZER, 1200); 
    delay(120);
    noTone(BUZZER);
    delay(120);
    tone(BUZZER, 1200); 
    delay(120);
    noTone(BUZZER);
    delay(500);
  }
  Serial.print("\n");    
}
bool horarioPermiteExecutar(DateTime horaAtual){
  long horaAtualParaSegundos = (horaAtual.hour() * 3600L) + (horaAtual.minute() * 60L) + horaAtual.second();  
  
  // if((horaAtualParaSegundos >= HORARIO_INICIO_PERMITIDO && horaAtualParaSegundos < HORARIO_FIM_PERMITIDO))  
  //   return true;
  // else     
  //   return false;  

  return true;
}

void atualizarTela() {
  if(sistema_automatico_pausado){   
    if((agora.second() % 20) < 10)
      aletarSistemaPausado();      
    else
      attDataHoraPercentual(agora,percentualCaixaMedia);    
  }else {
    attDataHoraPercentual(agora,percentualCaixaMedia);
  }
}

void aletarSistemaPausado(){
  // lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   ALERTA !!!   ");  
  lcd.setCursor(0,1);
  lcd.print("AUTOMATICO DESLIGADO");
}

void grafico(){
  lcd.home();  
  int a = map(percentualCaixaMedia,0,100,0,80);

  for(int i = 0; i < a/5;i++){
    lcd.write(byte(255));
  }

  switch(a % 5){
    case 0: 
      lcd.write(byte(254));
      break;
    case 1:
      lcd.write(byte(1));
      break;
    case 2:
      lcd.write(byte(2));
      break;
    case 3:
      lcd.write(byte(3));
      break;
    case 4:
      lcd.write(byte(4));
      break;
  }
  
  for (int i = a/5 + 1; i < 16; i++){   
    lcd.setCursor(i,0);
    lcd.write(byte(254));
  }    
}

void attDataHoraPercentual(DateTime dataHora, int percentualCaixaDagua) {  
  lcd.setCursor(0,0);
  lcd.print(dataHora.day()/10 % 10);  
  lcd.print(dataHora.day() % 10);
  lcd.print("/");
  lcd.print(dataHora.month()/10 % 10);
  lcd.print(dataHora.month() % 10);
  lcd.print("/");
  lcd.print(dataHora.year());
  lcd.print("  ");
  if (percentualCaixaDagua == 100){
    lcd.setCursor(12,0);
    lcd.print("100%");
  }else{
    lcd.setCursor(12,0);
    lcd.print(" ");
    lcd.print(percentualCaixaDagua/10 % 10);
    lcd.print(percentualCaixaDagua % 10);
    lcd.print("%");  
  }
  lcd.setCursor(0,1);
  lcd.print(dataHora.hour()/10 % 10);
  lcd.print(dataHora.hour() % 10);
  lcd.print(":");
  lcd.print(dataHora.minute()/10 % 10);
  lcd.print(dataHora.minute() % 10);
  lcd.print(":");
  lcd.print(dataHora.second()/10 % 10);
  lcd.print(dataHora.second()%10);
  lcd.print("           ");
}

bool leitura2Segundos(bool leitura, SensorNivel *sensor){

  bool statusAtual = sensor -> statusAtual;

  sensor -> leituras[sensor -> index++] = leitura;

  if(sensor -> index == 10)
    sensor -> index = 0;
  
  bool primeiraOcorrencia = sensor -> leituras[0];
  
  for (int i = 0; i < 10; i++) {
    if (sensor -> leituras[i] != primeiraOcorrencia)
      return sensor -> statusAtual;
  }

  sensor -> statusAtual = primeiraOcorrencia;
  return primeiraOcorrencia;
}

float mediaDescartadora(float nivelAgua){
  int zeros = 0;
  float somaNiveis = 0;
  int maior = 0;
  int menor = 0;  

  ultimosNiveis -> niveis[ultimosNiveis -> index++] = nivelAgua;
  
  for (int i = 0; i < 10; i++) {
    somaNiveis += ultimosNiveis -> niveis[i];
    if(ultimosNiveis -> niveis[i] <= 0 )
      zeros++; 
  }  

  if(ultimosNiveis -> index >= 10)
    ultimosNiveis -> index = 0;
  
  //Localiza o maior e a menor leitura do sensor
  for (int i = 0; i < 10; i++) {
    if(ultimosNiveis -> niveis[i] > maior)
      maior = ultimosNiveis -> niveis[i];
    if(ultimosNiveis -> niveis[i] < menor || menor == 0)
      menor = ultimosNiveis -> niveis[i];    
  }
  
  //Media que descarta a maior e menor leitura  
  return (somaNiveis - maior - menor)/(8 - zeros); 
}
