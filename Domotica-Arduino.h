#ifndef Domotica-Arduino_h
#define Domotica-Arduino_h


#include <math.h>  //necesario para calcular temperatura
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "UniversalTelegramBot.h"  //https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <stdarg.h>
#include <Time.h> //http://www.leantec.es/blog/42_Como-medir-el-tiempo-con-Arduino-y-la-librer%C3%AD.html

// Pines de la arduino y temperatura
#define RELE_UNO D4
#define LED_NORMAL D6
#define TEMP_CALOR 25.0
#define LED_CALOR D7
#define TEMP_FRIO 17.0
#define LED_FRIO D8
#define ANALOG_TEMP A0

#define TAM_ARRAY 3  // array de string que se le pasa por referencia
#define TIEMPO_ESPERA_MSG 1500 // mean time between scan messages
#define CONTADOR_MAX 15  // tiempo(30seg) para checkear el modo automatico
#define CONTADOR_WARNING 4800  // tiempo(60min)*2 para mandar un msg, bucle se ejecuta cada 1.5seg, 3600/1.5 = 2400 * 2 
#define BOTtoken API_BOT  // token bot

#define TIEMPO 0, 0, 0, 1, 1, 1970  //siempre que iniciamos el contador empezados desde la misma fecha


typedef struct {
  unsigned long tiempoActual;   //last time messages' scan has been done
  bool activo;
  unsigned long timer;  //tiempo
  time_t horaInicial; //variable que lleva la hora
  String accion_timer; //on - off
  boolean modo_automatico;  //modo automatico para la temperatura
  int temperatura_max;  //temperatura maxima para el modo automatico
  int contador_max; //contador para que se ejecute el modo automatico cada 15seg
  int contador_warning; //contador para envios periodicos mientras este activo modo automatico
} timers_globales;


void conecta_wifi();
double calculaTemperatura();
void genera_teclado(String chat_id, String msg);
void show_start(String chat_id);
void set_rele(String accion, String chat_id, String msgOpcional);
bool get_rele();
void set_timer(String accion, int tiempo, String chat_id);
String get_timer();
void check_timer();
void modo_automatico(String accion, String temperatura, String chat_id);
void check_modo_automatico();
void parseaComando(String comando, String arrays[TAM_ARRAY]);
void botTrataMensajes(int numNewMessages);
void check_telegram_msg();
void ajustaLedTemperatura(float temperatura);


#endif
