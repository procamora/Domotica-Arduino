# Domotica-Arduino
Proyecto para el control de un rele a traves de un bot de telegram.



# Requisitos

## Instalar la placa esp8266

Instrucciones: https://procamora.github.io/2016/09/uso-telegram-en-arduino-wemos/


## Librerias necesarias:

- ArduinoJson
- Time

## Fichero con las credenciales

El fichero `credentials.h` debe de ser:
```c
#define SSID_WIFI "WIFI"
#define PASS_WIFI "PASS"

#define API_BOT "TOKEN"

#define NAME_BOT "NAME"
#define ALIAS_BOT "@alias"
#define ID_TELEGRAM "000000"
```

![](esquema_bb.png)
