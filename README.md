# Integración para Alarma X-28 con ESPHome

Este proyecto permite controlar una alarma casera X-28 con protocolo MPX desde la interfaz
de HomeAssistant.

De momento permite:

- Activar/desactivar alarma
- Pasar modo estoy/me voy
- Disparar alarma (Pánico)

Si bien es controlable desde la GUI de HA, lo interesante es que ahora podemos hacer
automatizaciones con nuestra alarma. Incluso hacer que se dispare cuando algun sensor
de HA detecta algo.

## Estado del proyecto

Esta es una versión inicial y tiene una funcionalidad minima. Algunas ideas pendientes:

- Monitoreo de estado de zonas/ alarma lista
- Monitoreo de estado de alarma disparada via MPX (se puede hacer por GPIO, pero seria
  mas limpio hacerlo por MPX)
- Parametrizacion de configuraciones

## Hardware requerido

El proyecto es solo compatible con ESP32. Con ESP8266, al llamar a `attachInterrupt`, se
cuelga y se reinicia el ESP.

Para conectar a la alarma se necesita basicamente un level shifter y un driver.

![Alt text](circuito.png?raw=true "Circuito")

## Como utilizar

1. Ajustar codigo de activar/deasctivar alarma, ver funcion `void activar(bool activada)`,
   alli estan los codigos de fábrica 1251/1254
2. Programar el ESP32 con el yaml de ESPHome
3. Agregar a HA
4. Tambien se puede ver en la web en http://ip-del-nodo:80
