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
#define SENSOR_BUFFER_BAIXO 7
#define ULTRASSONICO_CIMA_TRIG 12
#define ULTRASSONICO_CIMA_ECHO 11
#define LED_BOMBA 52
#define TOM 10
#define BUFFER_ 48

LiquidCrystal_I2C lcd(0x27,16,2);
UltraSonicDistanceSensor ultrassonico(ULTRASSONICO_CIMA_TRIG,ULTRASSONICO_CIMA_ECHO);
RTC_DS3231 rtc;

// (3600 * HORA) + (60 * MINUTO) + SEGUNDO
const long HORARIO_INICIO_PERMITIDO =  ((3600L * 7) + (60L * 0) + 0L); //25.200
const long HORARIO_FIM_PERMITIDO = ((3600L * 23) + (60L * 30) + 0L);  //77.400

//MEDIDAS
#define ALTURA_CAIXA_DAGUA 70 //cm
#define ALTURA_TAMPA_CAIXA 12 //cm


typedef struct Nivel {
  float niveis[10] = {0,0,0,0,0};
  int index = 0;
}Nivel;

Nivel kk;
Nivel *ultimosNiveis = &kk;

byte pontos[8] = {B00000, B00101, B00000, B01010, B00000, B00010, B11111};
char daysOfTheWeek[7][12] = {"Domingo","Segunda-Feira","Terça-Feira","Quarta-Feira","Quinta-Feira","Sexta-Feira","Sabado"};
DateTime agora;
void attDataHoraPercentual(DateTime dataHora, int percentualCaixaDagua);

bool status = true;
float leituraAtualNormalizada = 0;
float leituraAtualBruta = 0;
int percentualCaixaMedia = 0;

int volumeBuzzer = 6;

void setup() {
  Serial.begin(9600);

  if(!rtc.begin()){
    Serial.println("Teste");
    Serial.flush();
    abort();
  }

  //RETIRAR QUANDO FOR PARA PRODUCAO
  rtc.adjust(DateTime(__DATE__,__TIME__));  
  pinMode(RELE,O);
  pinMode(BUZZER,O);
  pinMode(LED_BOMBA,O);
  pinMode(SENSOR_BUFFER_BAIXO,INPUT_PULLUP);  
  pinMode(SENSOR_BUFFER_CIMA,INPUT_PULLUP);
  
  digitalWrite(SENSOR_BUFFER_BAIXO,L);  
  digitalWrite(SENSOR_BUFFER_CIMA,L);

  
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
bool horarioPermite = false;
bool a = false;
bool b = false;

void loop() {
  
  agora = rtc.now();
  leituraAtualBruta = ultrassonico.measureDistanceCm();
  leituraAtualNormalizada = constrain(leituraAtualBruta,ALTURA_TAMPA_CAIXA,  ALTURA_CAIXA_DAGUA);
  percentualCaixaMedia = 100 - map(mediaDescartadora(leituraAtualNormalizada),ALTURA_TAMPA_CAIXA ,ALTURA_CAIXA_DAGUA, 0, 100);
  attDataHoraPercentual(agora,percentualCaixaMedia);
 
  if((horarioPermiteExecutar(agora) && percentualCaixaMedia < 40) || status ){
    if(!status){
      status = true;
      alarmeEntrada();
    }

    if(percentualCaixaMedia == 99)
      status = false;
             
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
    status = false;    
  }  

  Serial.print("\n");
  Serial.print("Bruta: ");
  Serial.print(leituraAtualBruta);
  Serial.print(" Normalizada: ");  
  Serial.print(leituraAtualNormalizada);
  Serial.print(" - Media: ");
  Serial.print(mediaDescartadora(leituraAtualNormalizada));
  Serial.print(" - Status: ");
  Serial.print(status);  
  a = digitalRead(SENSOR_BUFFER_BAIXO);
  b = digitalRead(SENSOR_BUFFER_CIMA);
  Serial.print(" - BUFFER_BAIXO: ");
  Serial.print(a);
   Serial.print(" - BUFFER_CIMA: ");
  Serial.print(b);  
  
  delay(100);
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
  
  if((horaAtualParaSegundos >= HORARIO_INICIO_PERMITIDO && horaAtualParaSegundos < HORARIO_FIM_PERMITIDO))  
    return true;
  else     
    return false;  
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
