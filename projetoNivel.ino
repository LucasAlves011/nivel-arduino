#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#include <HCSR04.h>

#define O OUTPUT
#define I INPUT
#define L LOW
#define H HIGH

//PORTAS
#define RELE 2
#define BUZZER 7
#define SENSOR_NIVEL_BOMBA_BAIXO 3
#define SENSOR_NIVEL_BOMBA_CIMA 4
#define ULTRASSONICO_CIMA_TRIG 12
#define ULTRASSONICO_CIMA_ECHO 11

LiquidCrystal_I2C lcd(0x27,16,2);
UltraSonicDistanceSensor ultrassonico(ULTRASSONICO_CIMA_TRIG,ULTRASSONICO_CIMA_ECHO);
RTC_DS3231 rtc;

// (3600 * HORA) + (60 * MINUTO) + SEGUNDO
const int HORARIO_INICIO_PERMITIDO = (3600 * 7) + (60 * 0) + 0;
const int HORARIO_FIM_PERMITIDO = (3600 * 21) + (60 * 30) + 0;

//MEDIDAS
#define ALTURA_CAIXA_DAGUA 71 //cm
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

void setup() {
  Serial.begin(9600);

  if(!rtc.begin()){
    Serial.println("Teste");
    Serial.flush();
    abort();
  }

  //RETIRAR QUANDO FOR PARA PRODUCAO
  rtc.adjust(DateTime(__DATE__,__TIME__));  
  // pinMode(12,O);
  // pinMode(11,I);
  
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
  attDataHoraPercentual(agora,45);
  if(horarioPermiteExecutar(agora) ){
     
    Serial.print("Permite");
  }else {
    // Serial.print("Nao permite\n");
  }

  Serial.print("\n ============== MEDICOES ==============\n");
  Serial.print("Sensor: ");
  Serial.print(ultrassonico.measureDistanceCm());
  Serial.print(" - Media: ");
  Serial.print(mediaDescartadora(ultrassonico.measureDistanceCm()));
  
  delay(500);
}

bool horarioPermiteExecutar(DateTime horaAtual){
  int horaAtualParaSegundos = (horaAtual.hour() * 3600) + (horaAtual.minute() * 60) + horaAtual.second();  
  (horaAtualParaSegundos >= HORARIO_INICIO_PERMITIDO && horaAtualParaSegundos < HORARIO_FIM_PERMITIDO) ? true : false;
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
  lcd.setCursor(13,0);
  lcd.print(percentualCaixaDagua/10 % 10);
  lcd.print(percentualCaixaDagua % 10);
  lcd.print("%");  
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
