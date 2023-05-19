#include <Wire.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "equation.h"
#include "password.h"
#include "constants.h"

// <-------------------- Začetek I2C funkcije -------------------------->
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
// <-------------------- Konec I2C funkcije -------------------------->

// <--------------------- Začetek definicij ------------------------->

// Naslov LED diod
uint8_t LED_niz[4] = {0xfe, 0xfd, 0xfb, 0xf7};
uint8_t LED_niz1[2] = {0xfe, 0xff};

uint8_t write_count = 0;
uint16_t totalSteps = 0;
float speed = 0;
float distance = 0;
float calories = 0;
char *type;
float x_acc_array[SIZE];
float y_acc_array[SIZE];
float z_acc_array[SIZE];

// Meritve iz senzorja
uint8_t accMeas[] = {0, 0, 0, 0, 0, 0};

// Kalibracijske vrednosti
float accX_off = 0;
float accY_off = 0;
float accZ_off = 0;

// Meritve pospeška
float accX = 0;
float accY = 0;
float accZ = 0;

// Klici knjižnic
BlynkTimer timer;
Ticker tick, tickLED, readSensor;
Equation eq;

// <--------------------- Konec definicij ------------------------->

void MPU9250_init()
{
  // Resetiraj MPU9250 senzor => Register PWR_MGMT_1 (107)
  I2CWriteRegister(MPU_ADD, 107, 128);
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

  int32_t table = 0;

  for (int i = 0; i < CAL_NO; i++)
  {
    Wire.beginTransmission(MPU_ADD);
    Wire.write(ACC_MEAS_REG);
    Wire.endTransmission();

    // Branje pospeškometra
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

    delay(1000 / CAL_NO);
  }

  accX_off /= CAL_NO;
  accY_off /= CAL_NO;
  accZ_off /= CAL_NO;

  Serial.println("\n Konec kalibracije");
}

// Branje pospeška
void readACC()
{
  static uint32_t count = 0;
  digitalWrite(PIN_LED, 0);
  int32_t table = 0;

  // MPU-9250
  Wire.beginTransmission(MPU_ADD);
  Wire.write(ACC_MEAS_REG);
  Wire.endTransmission();

  // Branje: pospešek v x_osi
  // Zdaj mikrokrmilnik bere od naslova ACC_X_OUT
  // Bere dva bajta prvi bajt:
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
    Klici metod iz equation.h
  */
  if (write_count == SIZE)
  {
    // float *highest_acc = eq.get_highest_acc(x_acc_array, y_acc_array, z_acc_array);
    // Izračunaj magnitudo pospeškov
    float *magnitude = eq.get_magnitude(x_acc_array, y_acc_array, z_acc_array);

    // Določi število korakov, dodaj v skupni števec
    uint8_t steps = eq.calc_steps(magnitude, TIME_STEP);
    totalSteps += steps;

    // Določi še ostale vrednosti
    speed = eq.calc_speed(steps);
    distance += eq.calc_distance(steps);
    calories += eq.calc_calories(speed);
    type = eq.infer_movement_type(steps);

    Serial.println("........");
    Serial.println(totalSteps);

    // Resetiraj
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

    free(magnitude);
  }

  // Dodaj trenutni count v array
  x_acc_array[write_count] = accX;
  y_acc_array[write_count] = accY;
  z_acc_array[write_count] = accZ;

  // Povečaj števec
  write_count++;
  digitalWrite(PIN_LED, 1);
}

// Send to Blynk - steps, distance, calories, type and speed
void send()
{
  Blynk.virtualWrite(V0, totalSteps);
  Blynk.virtualWrite(V1, distance);
  Blynk.virtualWrite(V2, calories);
  Blynk.virtualWrite(V3, type);
  Blynk.virtualWrite(V4, 3.6 * speed);
}

void setup()
{
  // Serijska komunikacija
  Serial.begin(115200);

  // I2C
  Wire.begin(12, 14);
  // SDA - 12 pin
  // SCL - 14 pin
  Wire.setClock(100000);
  MPU9250_init();

  // Določi velikost arraya za interval pospeška pri korakih
  // eq.setSize(SIZE);

  // Kalibracija
  Serial.println("Kalibracijo pospeškometra začenjam čez 5 sec.");
  Serial.println("Prosim postavite napravo na ravnino in jo pustite na miru.");
  tickLED.attach_ms(1100, utripLED);

  delay(500); // to do 5000
  tickLED.detach();
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