#include<Wire.h>
#include<Ticker.h>


// <-------------------- I2C  funkciji zacetek -------------------------->
void I2CWriteRegister(uint8_t I2CDevice, uint8_t RegAdress, uint8_t Value){
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

void I2CReadRegister(uint8_t I2CDevice, uint8_t RegAdress, uint8_t NBytes, uint8_t *Value){
  Wire.beginTransmission(I2CDevice);
  // Napravi sporočimo naslov registra, s katerega želimo brati:
  Wire.write(RegAdress);
  // Končamo prenos:
  Wire.endTransmission();
  
  // Napravi sporočimo, da želimo prebrati določeno število 8-bitnih registrov:
  Wire.requestFrom(I2CDevice, NBytes);
  for (int q = 0; q < NBytes; q++) {
    // Preberemo naslednji 8-bitni register oz. naslednji bajt:
    *Value = (uint8_t)Wire.read();
    Value++;
    //uint32_t vrednost = Wire.read();
  }

}

// <-------------------- I2C  Funkcije konec -------------------------->

#define PIN_LED 2
  // Naslov MPU9250 na I2C vodilu
  #define MPU_ADD 104
  // Naslov registra za pospesek 
  #define ACC_MEAS_REG 59
// Naslov registra za ziroskop
#define GYRO_MEAS_REG 67
#define delilnik 16384
#define RATE 25
// Stevilo uzorcev za kalibracijo
#define CAL_NO 100
// Stevilo uzorcev za branje
#define READ_NO 10

uint8_t LED_niz[4] = {0xfe, 0xfd, 0xfb,0xf7 }; // led diode
uint8_t LED_niz1[2] = {0xfe, 0xff };
#define I2C_ADD_IO1 32

Ticker tick,tickLED,readSensor;
// Globalni stevec zanke 
uint8_t iter = 0;

// Meritve sa senzorja 
uint8_t gyroMeas[] = {0,0,0,0,0,0};
uint8_t accMeas[] = {0,0,0,0,0,0};
// Kalibracione vrednosti 
float gyroX_off = 0; 
float gyroY_off = 0; 
float gyroZ_off = 0; 

// Meritve pospeškometra
float gyroX ,gyroY, gyroZ;
// Kalibracione vrednosti 
float accX_off = 0; 
float accY_off = 0; 
float accZ_off = 0; 
// Meritve  acc
float accX ,accY;
float accZ=-1.00;


void MPU9250_init(){
  // Resetiraj MPU9250 senzora => Register PWR_MGMT_1 (107)
  I2CWriteRegister(MPU_ADD,107,128); // 128 = 1000 0000
  // Pocakaj
  delay(500);
  // Preveri ID od senzora => Register WHO_AM_I (117) 
  uint8_t ID;
  I2CReadRegister(MPU_ADD,117,1,&ID);
  Serial.println("ID:");
  Serial.println(ID, HEX);
  // Gyroscope Conf => Register GYRO_CONFIG (27) 
  // 4 in 3 bit dolocata obseg 
  I2CWriteRegister(MPU_ADD,27,0); // 
  delay(100);
  // Accelerator Conf => Register ACCEL_CONFIG (28)
  // 4 in 3 bit dolocata obseg 
  // Opciono => Register ACCEL_CONFIG_2 (29)
  I2CWriteRegister(MPU_ADD,28,0); // 
  delay(100);
}

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

void calibrateGyro(){
  
  uint32_t ITER = 10; // Stevilo uzorcev za glajenje 
  int32_t tmp;
  Serial.println("Kalibracija ziroskopa");
  for(int i=0; i< CAL_NO; i++){
      I2CReadRegister(MPU_ADD,GYRO_MEAS_REG,6,gyroMeas);
     // GYRO_XOUT = Gyro_Sensitivity * X_angular_rate
     tmp = (((int8_t)gyroMeas[0] << 8) + (uint8_t)gyroMeas[1]);
     gyroX_off += tmp*1.0/131.0;
      // GYRO_YOUT = Gyro_Sensitivity * Y_angular_rate
     tmp = (((int8_t)gyroMeas[2] << 8) + (uint8_t)gyroMeas[3]);
     gyroY_off += tmp*1.0/131.0;
     // GYRO_ZOUT = Gyro_Sensitivity * Z_angular_rate
     tmp = (((int8_t)gyroMeas[4] << 8) + (uint8_t)gyroMeas[5]);
     gyroZ_off += tmp*1.0/131.0;
     Serial.print("...");
  }
  Serial.println("\nKonec kalibracije");

  gyroX_off /= CAL_NO;
  gyroY_off /= CAL_NO;
  gyroZ_off /= CAL_NO;
  
  Serial.println("Ziroskop X osa");
  Serial.println(gyroX_off);
  Serial.println("Ziroskop Y osa");
  Serial.println(gyroY_off);
  Serial.println("Ziroskop Z osa");
  Serial.println(gyroZ_off);

}
void calibrateACC(){
  digitalWrite(PIN_LED, 0);

  delay(1000);

  int samp = CAL_NO;
  int32_t table;
  Serial.println("Kalibracija ziroskopa");

  for(int i=0; i< CAL_NO; i++){
      Wire.beginTransmission(MPU_ADD);
      Wire.write(ACC_MEAS_REG);
      Wire.endTransmission();
      
      //** Branje: pospeškometera
      Wire.requestFrom(MPU_ADD, 6);
      table = (int8_t)Wire.read();
      table = table << 8;
      table += (uint8_t)Wire.read();
      accX_off += (table/ delilnik)/RATE;
      table = (int8_t)Wire.read();
      table = table << 8;
      table += (uint8_t)Wire.read();
      accY_off += (table/ delilnik)/RATE;
      table = (int8_t)Wire.read();
      table = table << 8;
      table += (uint8_t)Wire.read();
      accZ_off += (table/ delilnik)/RATE;
  
      delay(1000/RATE);
  }
  Serial.println("\n Konec kalibracije");

  accX_off /= CAL_NO;
  accY_off /= CAL_NO;
  accZ_off /= CAL_NO;
  
  Serial.println("pospeskomer X osa");
  Serial.println(accX_off);
  Serial.println("pospeskomer Y osa");
  Serial.println(accY_off);
  Serial.println("pospeskomer Z osa");
  Serial.println(accZ_off);

}
void readACC() {
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
  // Da se izognemo meritvenemu hrupu
  // računamo povprečno vrednost na podlagi RATE uzorcev
  // RATE - število uzorcev
  // acc_x(i) =  (table(i) / 131.0f) - acc_x_calibrated
  // acc_x = sum_{i=0}^{RATE-1}(acc_x(i)/RATE) -> LATEX 
  accX += ((table / delilnik)-accX_off)/RATE;

   table = (int8_t)Wire.read();
  table = table << 8;
  table += (uint8_t)Wire.read();
  accY += ((table / delilnik)-accY_off)/RATE;

  table = (int8_t)Wire.read();
  table = table << 8;
  table += (uint8_t)Wire.read();
  accZ += ((table / delilnik)-accZ_off)/RATE;

  if (count % RATE == 0)
  {
    // Izpišemo
    Serial.print("ACC_X: X= ");
    Serial.print(accX);
    Serial.println("\n");
     Serial.print("ACC_Y: Y= ");
    Serial.print(accY);
    Serial.println("\n");
     Serial.print("ACC_Z: Z= ");
    Serial.print(accZ);
    Serial.println("\n");
    // resetiramo vrednost
    accX = 0;
    accY = 0;
    accZ = -1;
  }
  // števec 
  count = count+1;
  digitalWrite(PIN_LED, 1);
}
void readGyro(){
  int32_t tmp;
  // TODO
}

void setup() {
  Serial.println("Zacetek");
  // Serijska komunikacija
  Serial.begin(115200);
  // I2C
  Wire.begin(12,14);
  // SDA - 12 pin
  // SCL - 14 pin
  Wire.setClock(100000);
  // Podesavanje senzorja 
  // https://github.com/bolderflight/mpu9250/blob/main/src/mpu9250.cpp
  MPU9250_init();
  // Kalibracija 
  //calibrateGyro();
  Serial.println("Kalibracijo gyroskopa in pospeskometra zacenjam cez 5 sec.");
  Serial.println("Prosim postavite napravo na ravnino in jo pustite na miru.");
  tickLED.attach_ms(1100,utripLED);

  delay(5000);
  tickLED.detach(); // Izklopite Ticker
  tickLED.attach_ms(200,utripLED);
    
    
  calibrateACC();
  // Branje senzorja
  //readSensor.attach(0.1, readGyro);
  tickLED.detach();
  tickLED.attach_ms(400,utripLED1);
  tick.attach_ms(30, readACC);
     
 
  
}

void loop() {
  // put your main code here, to run repeatedly:

}