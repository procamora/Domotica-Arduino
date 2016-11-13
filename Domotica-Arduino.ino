/*******************************************************************
    this is a basic example how to program a Telegram Bot
    using TelegramBOT library on ESP8266
 *                                                                 *
    Open a conversation with the bot, it will echo your messages
    https://web.telegram.org/#/im?p=@EchoBot_bot
 *                                                                 *
    written by Giacarlo Bacchio
 *******************************************************************/


#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266TelegramBOT.h>
#include "DHT.h"  //https://github.com/adafruit/DHT-sensor-library

#include "credentials.h"

#define RELE_UNO D4
#define LED_NORMAL D6
#define TEMP_CALOR 25.0
#define LED_CALOR D7
#define TEMP_FRIO 17.0
#define LED_FRIO D8
#define LED_HUMEDAD D5
#define DHTPIN D2     // what digital pin we're connected to

#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);    // Initialize DHT sensor.

// Initialize Wifi connection to the router
const char* ssid = SSID_WIFI;
const char* password = PASS_WIFI;

// Initialize Telegram BOT
#define BOTtoken API_BOT  //token of TestBOT
#define BOTname NAME_BOT
#define BOTusername ALIAS_BOT
TelegramBOT bot(BOTtoken, BOTname, BOTusername);
int admin_tg = ID_TELEGRAM;         //PONER EN CREDENTIAL.H
int Bot_mtbs = 2000; //mean time between scan messages
unsigned long Bot_lasttime;   //last time messages' scan has been done
bool Start = false;



void setup() {
  pinMode(RELE_UNO, OUTPUT);
  pinMode(LED_FRIO, OUTPUT);
  pinMode(LED_CALOR, OUTPUT);
  pinMode(LED_NORMAL, OUTPUT);
  pinMode(LED_HUMEDAD, OUTPUT);

  Serial.begin(115200);
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  dht.begin();
  delay(2000); // Wait a few seconds between measurements.

  bot.begin();      // launch Bot functionalities
  bot.sendMessage(String(admin_tg), "Inicio arduino", "");
}


/********************************************
   EchoMessages - function to Echo messages
 ********************************************/
void Bot_EchoMessages(float temp, float hum) {
  for (int i = 1; i < bot.message[0][0].toInt() + 1; i++)      {
    String comando = bot.message[i][5].substring(1, bot.message[i][5].length());
    int id_user = (bot.message[i][1]).toInt();
    //bot.sendMessage(bot.message[i][4], comando, "");
    if (comando == "/get_hum") {
      bot.sendMessage(bot.message[i][4], "La humedad es: " + String(hum) + "%", "");
    }
    else if (comando == "/get_temp") {
      bot.sendMessage(bot.message[i][4], "La temperatura es: " + String(temp) + "*C", "");
    }
    else if (comando == "/set_rele_on") {
      digitalWrite(RELE_UNO, HIGH);    // turn the LED off (LOW is the voltage level)
      bot.sendMessage(bot.message[i][4], "Encendido rele 1", "");
    }
    else if (comando == "/set_rele_off") {
      digitalWrite(RELE_UNO, LOW);    // turn the LED off (LOW is the voltage level)
      bot.sendMessage(bot.message[i][4], "Apagado rele 1", "");
    }
    else if (comando == "/start") {
      bot.sendMessage(bot.message[i][4], "Bienvenido al BOT para la gesti?n de la temperatura", "");
      bot.sendMessage(bot.message[i][4], "/get_temp : Obtener temperatura", "");
      bot.sendMessage(bot.message[i][4], "/get_hum : Obtener humedad", "");
      if (admin_tg == id_user)
        bot.sendMessage(bot.message[i][4], "Entras en modo Administrador", "");
      else
        bot.sendMessage(bot.message[i][4], "Entras en modo Invitado", "");
      Start = true;
    }
    else
      bot.sendMessage(bot.message[i][4], bot.message[i][5], "");
  }
  bot.message[0][0] = "";   // All messages have been replied - reset new messages
}


void AjustaLedTemperatura(float temperatura) {
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


void AjustaLedHumedad(float humedad) {
  if (humedad < 30.0 || humedad > 50)
    digitalWrite(LED_HUMEDAD, HIGH);
  else
    digitalWrite(LED_HUMEDAD, LOW);
}


void loop() {

  if (millis() > Bot_lasttime + Bot_mtbs)  {
    Bot_lasttime = millis();

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (!isnan(h) && !isnan(t)) {
      // Compute heat index in Celsius (isFahreheit = false)
      float hic = dht.computeHeatIndex(t, h, false);

      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.print(" *C ");
      Serial.print("Heat index: ");
      Serial.print(hic);
      Serial.println(" *C ");
    }
    else {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    AjustaLedTemperatura(t);
    AjustaLedHumedad(h);

    bot.getUpdates(bot.message[0][1]);   // launch API GetUpdates up to xxx message
    Bot_EchoMessages(t, h);   // reply to message with Echo
  }
}
