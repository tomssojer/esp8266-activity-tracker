// Naslov MPU9250 na I2C vodilu
#define MPU_ADD 104
#define I2C_ADD_IO1 32

// Naslov registra za pospešek
#define ACC_MEAS_REG 59

#define delilnik 16384.0f

#define PIN_LED 2

// NASTAVLJIVI PARAMETRI SO SPODAJ -PO potrebi lahko spreminjate!

// Število vzorcev za kalibracijo
#define CAL_NO 100

// višina uporabnika
#define HEIGHT 1.80

//teža uporabnika
#define WEIGHT 80

// Časovni korak beleženja v ms - 1/frekvenca
#define TIME_STEP 40

//Prag pri računanju korakov z odvodi, da se izognemo false positives
#define DERIVATIVE_TRESHOLD 0.1f

// Število beleženj preden se podatki obdelajo (izračun korakov) in pošljejo na Blynk
#define SIZE 50

//širina okna za moving avrage filter - izberi sode številke, default=2
#define WINDOWS_SIZE 2

// Spodnja meja zaznave v ms, minimalni čas med zaznavo koraka
#define MIN_TIME_STEP 200