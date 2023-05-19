#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "password.h"

BlynkTimer timer;

void send()
{
    int koraki = 1;
    float razdalja = 5;
    float kalorije = 10;
    char Tip[] = "Hoja";

    Blynk.virtualWrite(V0, koraki);
    Blynk.virtualWrite(V1, razdalja);
    Blynk.virtualWrite(V2, kalorije);
    Blynk.virtualWrite(V3, Tip);
}

void setup()
{
    Serial.begin(115200);

    Blynk.begin(BLYNK_AUTH_TOKEN, SSID, PASSWORD);

    timer.setInterval(1000L, send);
}

void loop()
{
    Blynk.run();
    timer.run();
}
