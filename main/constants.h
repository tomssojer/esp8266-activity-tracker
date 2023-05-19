#define HEIGHT 1.80
#define WEIGHT 80
#define PIN_LED 2

// Naslov MPU9250 na I2C vodilu
#define MPU_ADD 104
#define I2C_ADD_IO1 32

// Naslov registra za pospešek
#define ACC_MEAS_REG 59

#define delilnik 16384.0f

// Število vzorcev za kalibracijo
#define CAL_NO 100.0f

// Časovni korak beleženja v ms
#define TIME_STEP 40

// Število beleženj preden se podatki pošljejo na Blynk
#define SIZE 50

// Spodnja meja beleženja v ms
#define MIN_TIME_STEP 200