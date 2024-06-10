/*
  IMU Capture
  Questo sketch utilizza il sensore IMU (Inertial Measurments Unit)
  che si trova a bordo dell'Arduino nano 33 BLE sense per leggere
  i dati dell'accelerometro e del giroscopio e scriverli sul serial monitor
  Di default il sensore viene campionato a 119 Hz, cioè 119 volte in un secondo.
  Lo sketch, attende che vi sia un movimento significativo, in cui il
  valore di accelerazione sia superiore ad una certa soglia, e in tal caso recupera
  e visualizza le 119 misurazioni che vengono fatte nel successivo intervallo di
  tempo pari ad un secondo.
  
  Puoi usare il serial plotter per vedere i dati in forma grafica
  
  Questo è lo sketch per la Rev. 1 delle schede:
  - Arduino Nano 33 BLE o Arduino Nano 33 BLE Sense board
  
  Created by Don Coleman, Sandeep Mistry
  Modified by Dominic Pajak, Sandeep Mistry
  Modified by Spazio Chirale
  This example code is in the public domain.
*/

#include <Arduino_LSM9DS1.h> // IMU usato nella Rev. 1 delle Board Nano 33 BLE

const float accelerationThreshold = 2.5; // soglia significativa in valori G
const int numSamples = 119; // numero di campioni da leggere (1 secondo)

int samplesRead; // variabile per tenere traccia del numero di campioni letti

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // inizializzo l'IMU
  if (!IMU.begin()) {
    Serial.println("Inizializzazione del sensore IMU fallita!");
    while (1);
  }

  // stampo l'intestazione del flusso CSV
  Serial.println("aX,aY,aZ,gX,gY,gZ");
}

void loop() {
  float aX, aY, aZ, gX, gY, gZ;

  // attendo un movimento significativo
  while (true) {
    if (IMU.accelerationAvailable()) {
      // leggo i valori di accelerazione
      IMU.readAcceleration(aX, aY, aZ);

      // sommo tra loro i valori assoluti
      float aSum = fabs(aX) + fabs(aY) + fabs(aZ);

      // verifico che il valore complessivo sia sopra la soglia
      if (aSum >= accelerationThreshold) {
        // inizializzo il conteggio dei campioni
        samplesRead = 0;
        break; // esco dal ciclo di attesa
      }
    }
  }

  // loop per recuperare 119 campioni
  // corrispondenti ad un intervallo pari ad 1 secondo
  // poiché l'IMU viene campionato a 119 Hz.
  while (samplesRead < numSamples) {
    // verifico che siano disponibili sia i valori dell'accelerometro
    // che del giroscopio
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      // leggo i valori
      IMU.readAcceleration(aX, aY, aZ);
      IMU.readGyroscope(gX, gY, gZ);

      samplesRead++;

      // stampo i dati separati da virgole (formato CSV)
      Serial.print(aX, 3);
      Serial.print(',');
      Serial.print(aY, 3);
      Serial.print(',');
      Serial.print(aZ, 3);
      Serial.print(',');
      Serial.print(gX, 3);
      Serial.print(',');
      Serial.print(gY, 3);
      Serial.print(',');
      Serial.print(gZ, 3);
      Serial.println();

      if (samplesRead == numSamples) {
        // è l'ultimo dei 119 campioni per cui inserisco una linea vuota
        Serial.println();
      }
    }
  }
}
