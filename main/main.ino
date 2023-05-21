#include <Wire.h>
#include <Ticker.h>
#include "equation.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "password.h"
#include "constants.h"

uint8_t LED_niz[4] = {0xfe, 0xfd, 0xfb, 0xf7}; // led diode
uint8_t LED_niz1[2] = {0xfe, 0xff};

// Count to 50, then reset
uint8_t write_count = 0;



// Meritve iz senzorja
float x_acc_array[SIZE];
float y_acc_array[SIZE];
float z_acc_array[SIZE];
uint8_t accMeas[] = {0, 0, 0, 0, 0, 0};
// Kalibracijske vrednosti
float accX_off = 0;
float accY_off = 0;
float accZ_off = 0;
// Meritve  acc
float accX = 0;
float accY = 0;
float accZ = 0;

// podatki za poslat
float speed = 0;
float distance = 0;
float calories = 0;
uint16_t totalSteps = 0;
char *type;

BlynkTimer timer;
Ticker tick, tickLED, readSensor;
Equation eq;

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

// funkcija za utrip diode
void utripLED()
{
  static uint8_t LED_stanje = 0;
  LED_stanje = (LED_stanje + 1) % 4;
  Wire.beginTransmission(I2C_ADD_IO1);
  Wire.write(LED_niz[LED_stanje]);
  Wire.endTransmission();
}
// funkcija 2 za utrip diode 
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

    delay(1000 / CAL_NO);
  }
  Serial.println("\n Konec kalibracije");

  accX_off /= CAL_NO;
  accY_off /= CAL_NO;
  accZ_off /= CAL_NO;

  Serial.println("Offset pospeskometra v X osi [g]");
  Serial.println(accX_off);
  Serial.println("Offset pospeskometra v Y osi [g]");
  Serial.println(accY_off);
  Serial.println("Offset pospeskometra v Z osi [g]");
  Serial.println(accZ_off);
}
void readACC()
{
  static uint32_t count = 0;
  digitalWrite(PIN_LED, 0);
  int32_t table = 0; 

  //**** MPU-9250
  //**** Naslov registra
  // "zapiši", od katerega naslova registra dalje želimo brati
  Wire.beginTransmission(MPU_ADD);
  Wire.write(ACC_MEAS_REG);
  Wire.endTransmission();

  //** Branje: pospešek v x_osi,y_osi, z-osi
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
  // konec branja ------------------------------------

  if (write_count == SIZE)
  {
    // Obdelaj podatke

    // POSTOPEK 1  - vzemi najvecjo os in izracunaj korake z njo
    //float *max_acc = eq.get_highest_acc(x_acc_array, y_acc_array, z_acc_array);
    //uint8_t steps = eq.calc_steps(max_acc, TIME_STEP);
    
    // POSTOPEK 2
    //float *magnitude = eq.get_magnitude(x_acc_array, y_acc_array, z_acc_array); // returnes smooteth magnitudes
    //uint8_t steps = eq.calc_steps(magnitude, TIME_STEP);
    
    //POSTOPEK 3 Z ODVODI
    float *magnitude = eq.get_magnitude(x_acc_array, y_acc_array, z_acc_array); // returnes smooteth magnitudes
    uint8_t steps = eq.calc_steps_deriv(magnitude, TIME_STEP);

    free(magnitude);

    // Napolni podatki za posiljanje
    totalSteps += steps;
    speed = eq.calc_speed(steps);
    distance += eq.calc_distance(steps);
    calories += eq.calc_calories(speed);
    type = eq.infer_movement_type(steps);

    // Serial.println(avg_speed);
    Serial.println("....................................................");
    Serial.printf("Stevilo korakov: %d\n",totalSteps);
    Serial.println("....................................................");
    Serial.println();
    // Izpišemo
    Serial.print("ACC_X [m/s]: X= ");
    Serial.print(accX * 9.81);
    Serial.println("\n");
    Serial.print("ACC_Y [m/s]: Y= ");
    Serial.print(accY * 9.81);
    Serial.println("\n");
    Serial.print("ACC_Z [m/s]: Z= ");
    Serial.print(accZ * 9.81);
    Serial.println("\n");
    write_count = 0;
   
  }
  // Dodaj trenutni count v array
  x_acc_array[write_count] = accX;
  y_acc_array[write_count] = accY;
  z_acc_array[write_count] = accZ;
  // števec
  write_count++;
  digitalWrite(PIN_LED, 1);
}

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

  // Kalibracija
  Serial.println("Kalibracijo pospeskometra zacenjam cez 5 sec.");
  Serial.println("Prosim postavite napravo na ravnino in jo pustite na miru.");
  tickLED.attach_ms(1100, utripLED); // dodani utripi diod 

  delay(5000);       // to do 5000
  tickLED.detach(); // Izklopite Ticker
  tickLED.attach_ms(200, utripLED);

  calibrateACC();
  // Branje senzorja
  tickLED.detach();
  // dodajanje utripanja diod
  tickLED.attach_ms(400, utripLED1);
  // Branje pospeskometra in izracun korakov
  tick.attach_ms(TIME_STEP, readACC);

  Blynk.begin(BLYNK_AUTH_TOKEN, SSID, PASSWORD);

  timer.setInterval(200L, send);
}

void loop()
{
  // povezava na spletni monitoring sistem
  Blynk.run();
  timer.run();
}