#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

char ssid[] = "";
char pass[] = "";

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

    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

    timer.setInterval(1000L, send);
}

void loop()
{
    Blynk.run();
    timer.run();
}
