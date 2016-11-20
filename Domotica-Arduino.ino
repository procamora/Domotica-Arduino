/**
   @file Domotica-Arduino.cpp
   @brief Gestion de un rele a traves de telegram

   @author Pablo Rocamora pablojoserocamora@gmail.com
   @date 13/11/2016
*/

#include <math.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266TelegramBOT.h>

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
#define tiempo_espera 2000 //mean time between scan messages

// Inicializamos Telegram BOT
#define BOTtoken API_BOT  // token bot
#define BOTname NAME_BOT  // nombre visible del bot
#define BOTusername ALIAS_BOT  //alias, acaba en ...._bot
TelegramBOT bot(BOTtoken, BOTname, BOTusername);


typedef struct {
  unsigned long tiempoActual;   //last time messages' scan has been done
  bool activo;
  unsigned long temporizador;  //tiempo * CONVERSION_MILLIS
  unsigned long sumaTemporizador;  //millis()
  String accion_temporizador; //on - off
} temporizadores_globales;

typedef struct {
  bool set_rele;
  bool set_temporizador;
} variables_globales;

variables_globales gobal_tg;
temporizadores_globales global_temporizador;


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

  gobal_tg.set_rele = false;
  gobal_tg.set_temporizador = false;

  global_temporizador.activo = false;

  bot.begin();      // Iniciamos el bot
  bot.sendMessage(ID_TELEGRAM, "Inicio arduino", "");
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


/**
   @brief funcion para obtener el estado del pin al que esta conectado el rele.
   digitalRead retorna 1 si esta activa y se convierte a true

   @param user string  con el id del usuario al que le contesta
*/
void show_start(String user) {
  //HACER ESTO EN UN SOLO ENVIO DE TELEGRAM
  String msg = "Bienvenido al BOT de Pablo para la gestion de la temperatura \
  /get_temp : Obtener temperatura";
  String keyboardJson = "[[\"/ledon\", \"/ledoff\"],[\"/status\"]]";
  bot.sendMessage(user, msg, keyboardJson);
}


void set_rele(String accion, String user) {
  Serial.println(accion);
  Serial.println(accion.length());
  String msg = "parametro de /set_rele incorecto";
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
    gobal_tg.set_rele = true;
    bot.sendMessage(user, "Que acción que deseas hacer con el rele? /on /off", "");
  }
  else
    bot.sendMessage(user, msg, "");

}

/**
   @brief funcion para obtener el estado del pin al que esta conectado el rele.
   digitalRead retorna 1 si esta activa y se convierte a true
*/
bool get_rele() {
  return bool(digitalRead(RELE_UNO));
}


void set_temporizador(String accion, int tiempo, String user) {
  global_temporizador.temporizador = tiempo * CONVERSION_MILLIS;
  global_temporizador.accion_temporizador = accion;
  global_temporizador.sumaTemporizador = millis();
  global_temporizador.activo = true;
  String msg = "Parametro de /set_temporizador incorrecto";

  if (accion == "on")
    msg = "Programado encendido en " + String(tiempo) + " min";
  else if (accion == "off")
    msg = "Programado apagado en " + String(tiempo) + " min";
  bot.sendMessage(user, msg, "");
}


/**
   @brief funcion para obtener el tiempo restante de temporizador, va de 0 a 99 no de 0 a 59.
   Esta funcion solo es llamada si global_temporizador.activo esta a true
*/
String get_temporizador() {
  float tiempo = (global_temporizador.temporizador + global_temporizador.sumaTemporizador) - millis();
  return String(tiempo / CONVERSION_MILLIS);
}


/**
   @brief funcion para parsear un string y dejar el resultado en un array, parsea el comando y 2 argumentos como maximo

   @param comando string a parsear
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


/**
  start - Inicia la conversacion
  get_temp - Obtienes la temperatua actual
  set_rele - Activa/Desactiva el rele
  get_rele - Obtienes el estado actual del rele
  set_temporizador - Activas el temporizador para una accion en x tiempo
  get_temporizador - Obtienes el tiempo restante para que se active el temporizador
  help - Muestra la ayuda
*/
void botTrataMensajes(float temp) {
  for (int i = 1; i < bot.message[0][0].toInt() + 1; i++) {
    //elimino el caracter / del comando, /set_rele on => set_rele on
    String comando = bot.message[i][5].substring(1, bot.message[i][5].length());

    String parse_comandos[TAM_ARRAY]; //array que contrandra comando mas argumentos
    parseaComando(comando, parse_comandos);

    //MODO ADMINISTRADOR
    if (String(ID_TELEGRAM) == String(bot.message[i][1])) {
      if (gobal_tg.set_rele) {
        gobal_tg.set_rele = false;
        //elimino el primer caracter que sera un '/'
        set_rele(parse_comandos[0].substring(1, parse_comandos[0].length()), bot.message[i][4]);
      }

      else if (parse_comandos[0] == "/get_temp")
        bot.sendMessage(bot.message[i][4], "La temperatura es: " + String(temp) + "*C", "");

      else if (parse_comandos[0] == "/set_rele")
        set_rele(parse_comandos[1], bot.message[i][4]);
      else if (parse_comandos[0] == "/get_rele") {
        String msg;
        if (get_rele())
          msg = "Encendido";
        else
          msg = "Apagado";
        bot.sendMessage(bot.message[i][4], "Estado del rele: " + msg, "");
      }

      else if (parse_comandos[0] == "/set_temporizador")
        set_temporizador(parse_comandos[1], parse_comandos[2].toInt(), bot.message[i][4]);
      else if (parse_comandos[0] == "/get_temporizador") {
        if (global_temporizador.activo)
          bot.sendMessage(bot.message[i][4], "Tiempo de temporizador " + get_temporizador() + " min.", "");
        else
          bot.sendMessage(bot.message[i][4], "Temporizador inactivo" , "");
      }

      else if (parse_comandos[0] == "/start") {
        show_start(bot.message[i][4]);
        bot.sendMessage(bot.message[i][4], "Entras en modo Administrador", "");
      }
      else if (parse_comandos[0] == "/help") {
        bot.sendMessage(bot.message[i][4], "No hay ayuda", "");
      }

      else
        bot.sendMessage(bot.message[i][4], "Comando desconocido :(", "");
    }

    //MODO INVITADO
    else {
      if (parse_comandos[0] == "/start") {
        show_start(bot.message[i][4]);
        bot.sendMessage(bot.message[i][4], "Entras en modo Invitado", "");
      }
      else
        //me llega una copia de los mensajes de desconocidos con su id + nombre
        bot.sendMessage(String(ID_TELEGRAM), "User: " + bot.message[i][1] + " @" +  bot.message[i][0] + " dice: " + bot.message[i][5], "");
    }
  }
  bot.message[0][0] = "";   // All messages have been replied - reset new messages
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
  if (global_temporizador.activo)
    if (millis() == global_temporizador.temporizador + global_temporizador.sumaTemporizador) {
      set_rele(global_temporizador.accion_temporizador, String(ID_TELEGRAM));
      global_temporizador.activo = false;
    }

  if (millis() > global_temporizador.tiempoActual + tiempo_espera)  {
    global_temporizador.tiempoActual = millis();

    int readTemp = analogRead(ANALOG_TEMP);
    double temp =  calculaTemperatura(readTemp);

    bot.getUpdates(bot.message[0][1]);
    botTrataMensajes(temp);
  }
}

