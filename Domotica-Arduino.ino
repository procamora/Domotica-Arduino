/**
   @file Domotica-Arduino.cpp
   @brief Gestion de un rele a traves de telegram

   @author Pablo Rocamora pablojoserocamora@gmail.com
   @date 13/11/2016
*/

/** Lista de comandos para mandar a botfather
  start - Inicia la conversacion
  get_temp - Obtienes la temperatua actual
  set_rele - Activa/Desactiva el rele
  get_rele - Obtienes el estado actual del rele
  set_timer - Activas el temporizador para una accion en x tiempo
  get_timer - Obtienes el tiempo restante para que se active el temporizador
  help - Muestra la ayuda
*/

#include <math.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "UniversalTelegramBot.h"  //https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <stdarg.h>

#include "credentials.h"  // todos los valores son String

// Pines de la arduino y temperatura
#define RELE_UNO D4
#define LED_NORMAL D6
#define TEMP_CALOR 25.0
#define LED_CALOR D7
#define TEMP_FRIO 17.0
#define LED_FRIO D8
#define ANALOG_TEMP A0

#define TAM_ARRAY 3  // array de string que se le pasa por referencia
#define CONVERSION_MILLIS 60000  // para conversion minutos a milisegundos
#define tiempo_espera 1500 //mean time between scan messages

#define BOTtoken API_BOT  // token bot

// Inicializamos Telegram BOT
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

typedef struct {
  unsigned long tiempoActual;   //last time messages' scan has been done
  bool activo;
  unsigned long timer;  //tiempo * CONVERSION_MILLIS
  unsigned long sumaTimer;  //millis()
  String accion_timer; //on - off
} timers_globales;

timers_globales global_timer;


void conecta_wifi() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("conectando a ");
  Serial.println(SSID_WIFI);
  WiFi.begin(SSID_WIFI, PASS_WIFI);  //definidas en credentials.h
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void setup() {
  pinMode(RELE_UNO, OUTPUT);
  pinMode(LED_FRIO, OUTPUT);
  pinMode(LED_CALOR, OUTPUT);
  pinMode(LED_NORMAL, OUTPUT);

  conecta_wifi();

  global_timer.activo = false;

  genera_teclado(ID_TELEGRAM, "Inicio arduino");
}


// IMPORTANTE TIENE QUE ESTAR CONECTADO AL 3.3V EN Wemos D1 R2
double calculaTemperatura(int RawADC) {
  double Temp;
  Temp = log(10000.0 * ((1024.0 / RawADC - 1)));
  Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp )) * Temp );
  Temp = Temp - 273.15;            // Convert Kelvin to Celcius
  //Temp = (Temp * 9.0)/ 5.0 + 32.0; // Convert Celcius to Fahrenheit
  return Temp;
}


void genera_teclado(String chat_id, String msg) {
  String fila1 = "[\"/set_rele\", \"/get_rele\", \"/get_temp\"],";
  String fila2 = "[\"/set_timer\", \"/get_timer\"]";
  String fila3 = "[\"/start\", \"/help\"],";

  String keyboardJson = "[" + fila1 + fila2  + fila3 + "]";
  bot.sendMessageWithReplyKeyboard(chat_id, msg, "HTML", keyboardJson, true, true, false);
}


/**
   @brief funcion para mandar un mensaje con todos los comandos disponibles
   Importante: el formateo tiene que ser HTML para que funcione el _, en markdown no funciona

   @param chat_id String id del usuario al que manda un mensaje
*/
void show_start(String chat_id) {
  String msg = "Bienvenido al BOT de Pablo para la gestion de temperatura\n";
  msg += "/start : Inicia la conversacion \n";
  msg += "/get_temp : Obtienes la temperatua actual \n";
  msg += "/get_rele : Obtienes el estado actual del rele \n";
  msg += "/set_rele : Activa / Desactiva el rele \n";
  msg += "/get_timer : Obtienes el tiempo restante para que se active el temporizador \n";
  msg += "/set_timer : Activas el temporizador para una accion en x tiempo \n";
  msg += "/help : Muestra la ayuda \n";

  genera_teclado(chat_id, msg);
}


/**
   @brief funcion para activar un rele, si se pasa sin ninguna accion crea botones con las acciones.

   @param accion String opcion para el rele: on - off
   @param chat_id String id del usuario al que manda un mensaje
*/
void set_rele(String accion, String chat_id) {
  /*Serial.println(accion);
    Serial.println(accion.length());*/
  String msg = "parametro de /set_rele incorecto\nUso: /set_rele on";
  if (accion == "on") {
    digitalWrite(RELE_UNO, HIGH);    // turn the LED off (LOW is the voltage level)
    msg = "Encendido rele 1";
  }
  else if (accion == "off") {
    digitalWrite(RELE_UNO, LOW);    // turn the LED off (LOW is the voltage level)
    msg = "Apagado rele 1";
  }
  //imprimimos mensaje dependiendo de si ha acabado o espera el parametro
  if (accion.length() == 0) {
    String msg = "Que acción que deseas hacer con el rele?\n";
    msg += "/set_rele on : Encender el rele\n";
    msg += "/set_rele off : Apagar el rele\n";
    msg += "/exit : Cancelar la accion de cambio del rele\n";
    String keyboardJson = "[[\"/set_rele on\", \"/set_rele off\"], [\"/exit\"]]";
    //String keyboardJson = "[[{\"text\":\"/on\", \"callback_data\":\"/on\"},
    //{\"text\":\"off\", \"callback_data\":\"/off\"}]]";
    bot.sendMessageWithReplyKeyboard(chat_id, msg, "HTML", keyboardJson, true, true);
  }
  else
    genera_teclado(chat_id, msg); //mando mensaje + pongo nuevo teclado
}


/**
   @brief funcion para obtener el estado del pin al que esta conectado el rele.

   @return boolean digitalRead retorna 1 si esta activa y se convierte a true
*/
bool get_rele() {
  return bool(digitalRead(RELE_UNO));
}


/**
   @brief funcion para activar un temporizador con una accion para el rele,
   si se pasa sin ninguna accion crea botones con las acciones.

   @param accion String opcion para el rele: on - off
   @param tiempo int tiempo en minutos antes de que sale el temporizador
   @param chat_id String id del usuario al que manda un mensaje
*/
void set_timer(String accion, int tiempo, String chat_id) {
  global_timer.timer = tiempo * CONVERSION_MILLIS;
  global_timer.accion_timer = accion;
  global_timer.sumaTimer = millis();
  global_timer.activo = true;
  String msg = "Parametro de /set_timer incorrecto\nUso:/set_timer off 20";

  if (accion == "on")
    msg = "Programado encendido en " + String(tiempo) + " min";
  else if (accion == "off")
    msg = "Programado apagado en " + String(tiempo) + " min";

  //imprimimos mensaje dependiendo de si ha acabado o espera el parametro
  if (accion.length() == 0) {
    String msg = "Que acción que deseas hacer con el temporizador?";

    String fila1 = "[\"/set_timer on 1\", \"/set_timer on 10\", \"/set_timer on 30\"],";
    String fila2 = "[\"/set_timer off 1\", \"/set_timer off 10\", \"/set_timer off 30\"],";
    String fila3 = "[\"/exit\"]";
    String keyboardJson = "[" + fila1 + fila2  + fila3 + "]";

    bot.sendMessageWithReplyKeyboard(chat_id, msg, "HTML", keyboardJson, true, true);
  }
  else
    genera_teclado(chat_id, msg); //mando mensaje + pongo nuevo teclado
}


/**
   @brief funcion para obtener el tiempo restante de temporizador, va de 0 a 99 no de 0 a 59.
   Esta funcion solo es llamada si global_timer.activo esta a true

   @return String con el tiempo que falta para que salte el timer
*/
String get_timer() {
  float tiempo = (global_timer.timer + global_timer.sumaTimer) - millis();
  return String(tiempo / CONVERSION_MILLIS);
}


/**
   @brief funcion para parsear un string y dejar el resultado en un array,
   parsea el comando y 2 argumentos como maximo

   @param comando String a parsear
   @param arrays[TAM_ARRAY] array de string que se le pasa por referencia
*/
void parseaComando(String comando, String arrays[TAM_ARRAY]) {
  char delimitador = ' ';
  int commaIndex = comando.indexOf(delimitador);
  //  Search for the next comma just after the first
  int secondCommaIndex = comando.indexOf(delimitador, commaIndex + 1);

  arrays[0] = comando.substring(0, commaIndex);
  arrays[1] = comando.substring(commaIndex + 1, secondCommaIndex);
  arrays[2] = comando.substring(secondCommaIndex + 1); //+1 para quitar espacio en blanco

  //cuando solo hay 1 parametro el 2 y 3 contienen el primer parametro, asi los dejo vacios
  if (arrays[1][0] == '/')
    arrays[1] = "";
  if (arrays[2][0] == '/')
    arrays[2] = "";
}



void botTrataMensajes(int numNewMessages, float temp) {
  for (int i = 0; i < numNewMessages; i++) {

    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    /*Serial.println(chat_id);
      Serial.println(text);*/

    //elimino el caracter / del comando, /set_rele on => set_rele on
    //String comando = bot.message[i][5].substring(1, bot.message[i][5].length());

    String parse_comandos[TAM_ARRAY]; //array que contrandra comando mas argumentos
    parseaComando(text, parse_comandos);

    /*Serial.println(parse_comandos[0]);
      Serial.println(parse_comandos[1]);
      Serial.println(parse_comandos[2]);*/
    //MODO ADMINISTRADOR
    if (String(ID_TELEGRAM) == chat_id) {
      if (parse_comandos[0] == "/exit")
        genera_teclado(chat_id, "Accion cancelada"); //mando mensaje + pongo nuevo teclado

      else if (parse_comandos[0] == "/get_temp")
        bot.sendMessage(chat_id, "La temperatura es: " + String(temp) + "*C", "");

      else if (parse_comandos[0] == "/set_rele")
        set_rele(parse_comandos[1], chat_id);

      else if (parse_comandos[0] == "/get_rele") {
        String msg;
        if (get_rele())
          msg = "Encendido";
        else
          msg = "Apagado";
        bot.sendMessage(chat_id, "Estado del rele: " + msg, "");
      }

      else if (parse_comandos[0] == "/set_timer")
        set_timer(parse_comandos[1], parse_comandos[2].toInt(), chat_id);

      else if (parse_comandos[0] == "/get_timer") {
        if (global_timer.activo)
          bot.sendMessage(chat_id, "Tiempo de temporizador " + get_timer() + " min.", "");
        else
          bot.sendMessage(chat_id, "Temporizador inactivo" , "");
      }

      else if (parse_comandos[0] == "/start") {
        bot.sendMessage(chat_id, "Entras en modo Administrador", "");
        show_start(chat_id);
      }

      else if (parse_comandos[0] == "/help")
        show_start(chat_id);

      else
        bot.sendMessage(chat_id, "Comando desconocido :(", "");
    }

    //MODO INVITADO
    else {
      if (parse_comandos[0] == "/start") {
        bot.sendMessage(chat_id, "Entras en modo Invitado", "");
        show_start(chat_id);
      }

      else
        bot.sendMessage(String(ID_TELEGRAM), "User: " + chat_id + " @" + bot.messages[i].username + " dice: " + text, "");
    }
  }
}


void ajustaLedTemperatura(float temperatura) {
  if (temperatura > TEMP_CALOR) {
    digitalWrite(LED_FRIO, LOW);
    digitalWrite(LED_CALOR, HIGH);
    digitalWrite(LED_NORMAL, LOW);
  }
  else if (temperatura < TEMP_FRIO) {
    digitalWrite(LED_FRIO, HIGH);
    digitalWrite(LED_CALOR, LOW);
    digitalWrite(LED_NORMAL, LOW);
  }
  else {  //temperatura normal
    digitalWrite(LED_FRIO, LOW);
    digitalWrite(LED_CALOR, LOW);
    digitalWrite(LED_NORMAL, HIGH);
  }
}


void loop() {
  if (global_timer.activo)
    if (millis() == global_timer.timer + global_timer.sumaTimer) {
      Serial.println("entrado temporizador");
      set_rele(global_timer.accion_timer, String(ID_TELEGRAM));
      global_timer.activo = false;
    }

  if (millis() > global_timer.tiempoActual + tiempo_espera)  {
    global_timer.tiempoActual = millis();

    int readTemp = analogRead(ANALOG_TEMP);
    double temp =  calculaTemperatura(readTemp);

    int numNewMessages = bot.getUpdates(bot.last_message_recived + 1);
    while (numNewMessages) {
      Serial.println("got response");
      botTrataMensajes(numNewMessages, temp);
      numNewMessages = bot.getUpdates(bot.last_message_recived + 1);
    }
  }
}
