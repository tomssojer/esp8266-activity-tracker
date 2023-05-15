#include "MPU9250.h"
#include <Ticker.h>

MPU9250 mpu;
uint8_t LED_niz[4] = {0xfe, 0xfd, 0xfb,0xf7 };
uint8_t LED_niz1[2] = {0xfe, 0xff };
int interval_LED_ms = 1100;

float biasACC[3];
float biasGyro[3];

#define I2C_ADD_IO1 32
#define NUM_ITER 10
#define I2C_ADD_MPU 104 // naslov MPU na vodilu 0x68 hexadecimal
Ticker tickGUMB, tickLED;

void utripLED(){
  static uint8_t LED_stanje = 0;
  LED_stanje = (LED_stanje + 1) % 4;
  Wire.beginTransmission (I2C_ADD_IO1);
  Wire.write(LED_niz[LED_stanje]);
  Wire.endTransmission();
}
void utripLED1(){
   static uint8_t LED_stanje = 0;
  LED_stanje = (LED_stanje + 1) % 2;
  Wire.beginTransmission (I2C_ADD_IO1);
  Wire.write(LED_niz1[LED_stanje]);
  Wire.endTransmission();
}
void setup() {
   // pinMode(PIN_LED, OUTPUT); //pisanje na pin za led
    Serial.begin(115200);
    Wire.begin(12,14);
    Wire.setClock(100000);
    pinMode(2,OUTPUT);
    delay(2000);
   
    if (!mpu.setup(I2C_ADD_MPU)) {  // naslov
        while (1) {
            Serial.println("MPU connection failed. Please check your connection with `connection_check` example.");
            delay(5000);
        }
    }
   Serial.println();
    // calibrate anytime you want to
    Serial.println("Kalibracijo gyroskopa in pospeskometra zacenjam cez 5 sec.");
    Serial.println("Prosim postavite napravo na ravnino in jo pustite na miru.");
    tickLED.attach_ms(interval_LED_ms,utripLED);
  
    mpu.verbose(true);
    delay(5000);
    tickLED.detach(); // Izklopite Ticker
    tickLED.attach_ms(200,utripLED);
   /* mpu.calibrateAccelGyro();
    biasACC[0] = mpu.getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY;
    biasACC[1] = mpu.getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY;
    biasACC[2] = mpu.getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY;

    biasGyro[0] = mpu.getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY;
    biasGyro[1] = mpu.getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY;
    biasGyro[2] = mpu.getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY;*/
    //print_calibration();
    //Serial.println("CALIBRATION 2");
    for (int i = 0 ; i < NUM_ITER ; i++) {
     mpu.calibrateAccelGyro();
      biasACC[0] += mpu.getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY;
      biasACC[1] += mpu.getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY;
      biasACC[2] += mpu.getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY;

      biasGyro[0] += mpu.getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY;
      biasGyro[1] += mpu.getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY;
      biasGyro[2] += mpu.getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY;
   }
    biasACC[0] /= NUM_ITER; 
    biasACC[1] /= NUM_ITER; 
    biasACC[2] /= NUM_ITER; 

    biasGyro[0] /= NUM_ITER; 
    biasGyro[1] /= NUM_ITER; 
    biasGyro[2] /= NUM_ITER; 
    tickLED.detach();
    tickLED.attach_ms(400,utripLED1);
    print_calibration();
    
    mpu.verbose(false);
}

void loop() {
     // Serial.println("DELA.");
}

void print_calibration() {
    Serial.println("< calibration parameters >");
    Serial.println("accel bias [g]: ");
    Serial.print(biasACC[0]);
    Serial.print(", ");
    Serial.print(biasACC[1]);
    Serial.print(", ");
    Serial.print(biasACC[2]);
    Serial.println();
    Serial.println("gyro bias [deg/s]: ");
    Serial.print( biasGyro[0]);
    Serial.print(", ");
    Serial.print( biasGyro[1]);
    Serial.print(", ");
    Serial.print( biasGyro[2]);
    Serial.println();

}