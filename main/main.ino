#include <Wire.h>
#include <Ticker.h>
#include "equation.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "password.h"

// <-------------------- I2C  funkciji zacetek -------------------------->
void I2CWriteRegister(uint8_t I2CDevice, uint8_t RegAdress, uint8_t Value)
{
  // I2CDevice - Naslov I2C naprave
  // RegAddress - Naslov registra
  // Value - Vrednost za vpisati v register

  Wire.beginTransmission(I2CDevice);
  // Napravi sporočimo naslov registra, s katerega želimo brati:
  Wire.write(RegAdress);
  // Posljemo vrednost
  Wire.write(Value);
  Wire.endTransmission();
}

void I2CReadRegister(uint8_t I2CDevice, uint8_t RegAdress, uint8_t NBytes, uint8_t *Value)
{
  Wire.beginTransmission(I2CDevice);
  // Napravi sporočimo naslov registra, s katerega želimo brati:
  Wire.write(RegAdress);
  // Končamo prenos:
  Wire.endTransmission();

  // Napravi sporočimo, da želimo prebrati določeno število 8-bitnih registrov:
  Wire.requestFrom(I2CDevice, NBytes);
  for (int q = 0; q < NBytes; q++)
  {
    // Preberemo naslednji 8-bitni register oz. naslednji bajt:
    *Value = (uint8_t)Wire.read();
    Value++;
    // uint32_t vrednost = Wire.read();
  }
}

// <-------------------- I2C  Funkcije konec -------------------------->

#define PIN_LED 2
// Naslov MPU9250 na I2C vodilu
#define MPU_ADD 104
// Naslov registra za pospesek
#define ACC_MEAS_REG 59
#define delilnik 16384.0f
#define RATE 50
// Stevilo uzorcev za kalibracijo
#define CAL_NO 100
// Stevilo vzorcev za branje
#define READ_NO 10
// Total array length
#define TOTAL_COUNT 50
// CASOVNI KORAK BELEZENJA
#define TIME_STEP 40
uint8_t LED_niz[4] = {0xfe, 0xfd, 0xfb, 0xf7}; // led diode
uint8_t LED_niz1[2] = {0xfe, 0xff};
#define I2C_ADD_IO1 32

// Count to 50, then reset
uint8_t write_count = 0;
uint16_t totalSteps = 0;
float x_acc_array[TOTAL_COUNT];
float y_acc_array[TOTAL_COUNT];
float z_acc_array[TOTAL_COUNT];

BlynkTimer timer;
Ticker tick, tickLED, readSensor;
Equation eq;
// Globalni stevec zanke
uint8_t iter = 0;

// Meritve iz senzorja
uint8_t accMeas[] = {0, 0, 0, 0, 0, 0};

// Kalibracijske vrednosti
float accX_off = 0;
float accY_off = 0;
float accZ_off = 0;
// Meritve  acc
float accX = 0;
float accY = 0;
float accZ = 0;

u_int32_t counter = 0;

void MPU9250_init()
{
  // Resetiraj MPU9250 senzor => Register PWR_MGMT_1 (107)
  I2CWriteRegister(MPU_ADD, 107, 128); // 128 = 1000 0000
  // Pocakaj
  delay(500);
  uint8_t ID;
  I2CReadRegister(MPU_ADD, 117, 1, &ID);
  Serial.println("ID:");
  Serial.println(ID, HEX);
  // 4 in 3 bit dolocata obseg
  I2CWriteRegister(MPU_ADD, 27, 0); //
  delay(100);
  I2CWriteRegister(MPU_ADD, 28, 0); //
  delay(100);
}

void utripLED()
{
  static uint8_t LED_stanje = 0;
  LED_stanje = (LED_stanje + 1) % 4;
  Wire.beginTransmission(I2C_ADD_IO1);
  Wire.write(LED_niz[LED_stanje]);
  Wire.endTransmission();
}
void utripLED1()
{
  static uint8_t LED_stanje = 0;
  LED_stanje = (LED_stanje + 1) % 2;
  Wire.beginTransmission(I2C_ADD_IO1);
  Wire.write(LED_niz1[LED_stanje]);
  Wire.endTransmission();
}

void calibrateACC()
{
  digitalWrite(PIN_LED, 0);

  delay(1000);

  int32_t table;

  for (int i = 0; i < CAL_NO; i++)
  {
    Wire.beginTransmission(MPU_ADD);
    Wire.write(ACC_MEAS_REG);
    Wire.endTransmission();

    //** Branje: pospeškometera
    Wire.requestFrom(MPU_ADD, 6);
    table = (int8_t)Wire.read();
    table = table << 8;
    table += (uint8_t)Wire.read();
    accX_off += (table / delilnik);
    table = (int8_t)Wire.read();
    table = table << 8;
    table += (uint8_t)Wire.read();
    accY_off += (table / delilnik);
    table = (int8_t)Wire.read();
    table = table << 8;
    table += (uint8_t)Wire.read();
    accZ_off += (table / delilnik);

    delay(1000 / RATE);
  }
  Serial.println("\n Konec kalibracije");

  accX_off /= CAL_NO;
  accY_off /= CAL_NO;
  accZ_off /= CAL_NO;

  Serial.println("pospeskomer v X osi");
  Serial.println(accX_off);
  Serial.println("pospeskomer v Y osi");
  Serial.println(accY_off);
  Serial.println("pospeskomer v Z osi");
  Serial.println(accZ_off);
}
void readACC()
{
  static uint32_t count = 0;
  digitalWrite(PIN_LED, 0);
  int32_t table;

  //**** MPU-9250
  //**** Naslov registra
  // "zapiši", od katerega naslova registra dalje želimo brati
  Wire.beginTransmission(MPU_ADD);
  Wire.write(ACC_MEAS_REG);
  Wire.endTransmission();

  //** Branje: pospešek v x_osi
  //** Zdaj mikrokrmilnik bere od naslova ACC_X_OUT
  //** Bere dva bajta prvi bajt:
  Wire.requestFrom(MPU_ADD, 6);
  table = (int8_t)Wire.read();
  table = table << 8;
  table += (uint8_t)Wire.read();
  accX = ((table / delilnik) - accX_off);

  table = (int8_t)Wire.read();
  table = table << 8;
  table += (uint8_t)Wire.read();
  accY = ((table / delilnik) - accY_off);

  table = (int8_t)Wire.read();
  table = table << 8;
  table += (uint8_t)Wire.read();
  accZ = ((table / delilnik) - accZ_off);

  /*

    Kliči funkcije iz equation.h

  */

  if (write_count == TOTAL_COUNT)
  {
    // Obdelaj podatke
    float *highest_acc = eq.get_highest_acc(x_acc_array, y_acc_array, z_acc_array);

    // for (int i = 0; i < TOTAL_COUNT; i++)
    //   printf("%f ", highest_acc[i]);
    // Serial.println("\n");

    uint8_t steps = eq.calc_steps(highest_acc, TIME_STEP);
    totalSteps += steps;
    float avg_speed = eq.calc_speed(steps);
    Serial.println("....................................................");
    Serial.println(totalSteps);
    Serial.println("Counter:");
    Serial.println(counter);

    write_count = 0;
    // Izpišemo
    Serial.print("ACC_X: X= ");
    Serial.print(accX * 9.81);
    Serial.println("\n");
    Serial.print("ACC_Y: Y= ");
    Serial.print(accY * 9.81);
    Serial.println("\n");
    Serial.print("ACC_Z: Z= ");
    Serial.print(accZ * 9.81);
    Serial.println("\n");
    accX = 0;
    // resetiramo vrednost
    // accX = 0;
    // accY = 0;
    // accZ = 0;
  }

  /*

    Konec if stavka

  */

  // Dodaj trenutni count v array
  // Serial.printf("write c %d\n", write_count);
  x_acc_array[write_count] = accX;
  y_acc_array[write_count] = accY;
  z_acc_array[write_count] = accZ;

  // števec
  counter++;
  write_count++;
  digitalWrite(PIN_LED, 1);
}

void send()
{
  float razdalja = 10;
  float kalorije = 1000;
  char Tip[] = "Hoja";

  Blynk.virtualWrite(V0, totalSteps);
  Blynk.virtualWrite(V1, razdalja);
  Blynk.virtualWrite(V2, kalorije);
  Blynk.virtualWrite(V3, Tip);
}

void setup()
{
  Serial.println("Zacetek");
  // Serijska komunikacija
  Serial.begin(115200);

  // I2C
  Wire.begin(12, 14);
  // SDA - 12 pin
  // SCL - 14 pin
  Wire.setClock(100000);
  // Podesavanje senzorja
  // https://github.com/bolderflight/mpu9250/blob/main/src/mpu9250.cpp
  MPU9250_init();

  // Določi velikost arraya za interval pospeška pri korakih
  eq.setSize(TOTAL_COUNT);

  // Kalibracija
  Serial.println("Kalibracijo pospeskometra zacenjam cez 5 sec.");
  Serial.println("Prosim postavite napravo na ravnino in jo pustite na miru.");
  tickLED.attach_ms(1100, utripLED);

  delay(5000);
  tickLED.detach(); // Izklopite Ticker
  tickLED.attach_ms(200, utripLED);

  calibrateACC();
  // Branje senzorja
  tickLED.detach();
  tickLED.attach_ms(400, utripLED1);
  tick.attach_ms(TIME_STEP, readACC);

  Blynk.begin(BLYNK_AUTH_TOKEN, SSID, PASSWORD);

  timer.setInterval(200L, send);
}

void loop()
{
  Blynk.run();
  timer.run();
}