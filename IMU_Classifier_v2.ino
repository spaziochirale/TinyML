/*
  IMU Classifier
  Questo sketch legge i valori di accelerazione lineare e angolare campionati dal sensore IMU
  e utilizza il modello TensorFlow Lite importato tramite il file model.h
  per identificare la gesture.
  Il codice è originariamente sviluppato da Don Coleman e Sandeep Mistry, con contributi da
  Dominic Pajak e incluso in diversi tutorial.
  Questa versione è stata modificata da Spazio Chirale per adattare il codice
  alla versione corrente della libreria TensorFloWLite Micro, mantenuta da Chirale
  
  *** Questo è lo sketch per la versione Rev. 1 delle schede
  - Arduino Nano 33 BLE or Arduino Nano 33 BLE Sense board.
  ***
  
  This example code is in the public domain.
*/

#include <Arduino_LSM9DS1.h> // IMU usato nella Rev. 1 delle board Nano 33 BLE

#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include "model.h"

const float accelerationThreshold = 2.5; // Soglia espressa in valori G
const int numSamples = 119; // numero di campioni in 1 secondo (l'IMU viene campionato di default a 119 Hz.)

int samplesRead; // variabile per tenere traccia del conteggio di campionamento



// inseriamo nel grafo di elaborazione tutti gli operatori previsti da tflite
// conoscendo in dettaglio il grafo di elaborazione, potremmo includere selettivamente
// solo gli operatori che effettivamente utiliuzziamo, riducendo l'occupazione di memoria
tflite::AllOpsResolver tflOpsResolver;

// dichiaro e inizializzo i puntatori al modello, all'interprete
// e ai tensori di ingresso e di uscita
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

// Creo un buffer di memoria statico per il TensorFlow Lite Micro
// la dimensione è stata stimata. Solitamente si procede a tentativi per
// definire questo parametro, si parte con un valore grande e successivamente
// si prova e diminuire la dimensione finché il modello sembra funzionare bene... 
constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize];

// array per mappare i nomi delle gesture
const char* GESTURES[] = {
  "punch",
  "flex"
};

#define NUM_GESTURES (sizeof(GESTURES) / sizeof(GESTURES[0]))

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // inizializzo l'IMU
  if (!IMU.begin()) {
    Serial.println("Inizializzazione del sensore IMU fallita!");
    while (1);
  }

  // stampo i valori di configurazione dell'IMU
  Serial.print("Frequenza di campionamento Acceleromero = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.print("Frequenza di campionamento Gyroscopio = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");

  Serial.println();

  // recupero la rappresentazione come array di byte del modello TensorFlow Lite
  // model è l'array costante definito nel file model.h 
  tflModel = tflite::GetModel(model);
  // verifico che il modello fornito tramite l'array model abbia la stessa
  // versione dello schema TensorFlow Lite della libreria che stiamo usando
  // lo schema potrebbe essere stato generato con una versione di TensorFlow non
  // più compatibile con la libreria TensorFlowLite utilizzata per questo sketch 
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Gli schemi del modello e della libreria hanno versioni diverse!");
    while (1);
  }

  // Creo l'oggetto interprete per eseguire il modello
  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize);

  // Alloco la memoria per contenere i tensori di input e di output
  tflInterpreter->AllocateTensors();

  // Assegno i puntatori ai tensori di input e output del modello
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);
}

void loop() {
  float aX, aY, aZ, gX, gY, gZ;

  // se è la prima esecuzione di loop oppure ho già completato un ciclo
  // di campionamento, attendo un movimento significativo
  while (true) {
    if (IMU.accelerationAvailable()) {
      // leggo i valori dell'accelerazione
      IMU.readAcceleration(aX, aY, aZ);

      // calcolo la somma dei valori assoluti
      float aSum = fabs(aX) + fabs(aY) + fabs(aZ);

      // controllo se il valore assoluto dell'accelerazione complessiva
      // sia superiore alla soglia
      if (aSum >= accelerationThreshold) {
        // inizializzo il conteggio dei campioni
        samplesRead = 0;
        break; // esco dal ciclo di attesa
      }
    }
  }

  // ciclo per leggere tutti i campionamenti relativi alla gesture in corso
  while (samplesRead < numSamples) {
    // verifico se i dati siano stati campionati
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      // leggo i valori
      IMU.readAcceleration(aX, aY, aZ);
      IMU.readGyroscope(gX, gY, gZ);

      // normalizzo i valori letti nell'intervallo tra 0 e 1 
      // e li inserisco nel tensore di input
      tflInputTensor->data.f[samplesRead * 6 + 0] = (aX + 4.0) / 8.0;
      tflInputTensor->data.f[samplesRead * 6 + 1] = (aY + 4.0) / 8.0;
      tflInputTensor->data.f[samplesRead * 6 + 2] = (aZ + 4.0) / 8.0;
      tflInputTensor->data.f[samplesRead * 6 + 3] = (gX + 2000.0) / 4000.0;
      tflInputTensor->data.f[samplesRead * 6 + 4] = (gY + 2000.0) / 4000.0;
      tflInputTensor->data.f[samplesRead * 6 + 5] = (gZ + 2000.0) / 4000.0;

      samplesRead++;
      
      // se ho letto tutti i campionamenti, eseguo l'inferenza
      // invocando l'interprete
      if (samplesRead == numSamples) {
        TfLiteStatus invokeStatus = tflInterpreter->Invoke();
        if (invokeStatus != kTfLiteOk) {
          Serial.println("Invoke fallito!");
          return;
        }

        // Ciclo per visualizzare i valori in output
        for (int i = 0; i < NUM_GESTURES; i++) {
          Serial.print(GESTURES[i]);
          Serial.print(": ");
          Serial.print(tflOutputTensor->data.f[i]*100, 2);
          Serial.println("%");
        }
        Serial.println();
      }
    }
  }
}
