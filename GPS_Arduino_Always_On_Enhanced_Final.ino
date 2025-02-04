
/*     MaxData.txt "MaxAlt: , MaxSpeed: , MaxVario: , Course: , Durata: StartFlight, EndFlight;
       DatiVolo.txt Date,Time,location.lng(),location.lat(),altitude.meters(),speed.kmph(), satellites.value(), vario);
      */
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SdFat.h>

const float threshold = 0.7;  // 70%
const int cs_sd = 3;          // pin di CS della SD
const int TState = 200;
static const int RXPin = 7, TXPin = 6;  // comunicazione GPS
static const uint32_t GPSBaud = 9600;
int ledPin;  // 11 rosso, 12 blu, 13 verde

float maxspeed = 0, maxalt = 0, maxvario = 0;
float course = 0.0;
unsigned long startTime = 0;
unsigned long previousTime = 0;
unsigned long interval = 2000;  // Intervallo di 2 secondi (modificabile dall'utente)
float vario = 0.0;
float previousAltitude = 0.0;
bool firstAltitudeMeasured = false;
String FlightStart = "";
String EndFlight = "";
String Date = "";
TinyGPSPlus gps;                  // oggetto TinyGPSPlus
SoftwareSerial ss(RXPin, TXPin);  // connessione seriale al GPS

SdFat SD;
SdFile diyFile;
SdFile MaxdiyFile;

//------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  ss.begin(GPSBaud);
  if (!SD.begin(cs_sd)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  if (!diyFile.open("DatiVolo.txt", O_RDWR | O_CREAT | O_AT_END)) {
    Serial.println("Error opening DatiVolo.txt!");
    return;
  }
  pinMode(ledPin, OUTPUT);
}

//------------------------------------------------------------------------------
void loop() {
  int satellite = gps.satellites.value();

  if (satellite > 0) {
    ledPin = 13;

    unsigned long currentTime = millis();
    float currentAltitude = gps.altitude.meters();
    float currentSpeed = gps.speed.kmph();

    if (currentAltitude > 0) {
      if (!firstAltitudeMeasured) {
        previousAltitude = currentAltitude;
        startTime = currentTime;
        previousTime = currentTime;
        firstAltitudeMeasured = true;
        Date = String(gps.date.day()) + "/" + String(gps.date.month()) + "/" + String(gps.date.year());
      }

      calculateVarioAndCourse(currentAltitude, currentSpeed, currentTime);
      calculateFlightTimes();
    }

    char buffer[200];
    sprintf(buffer, "%02d/%02d/%04d , %02d:%02d:%02d , %.6f , %.6f , %d  , %d  , %d , %.2f",
            gps.date.day(), gps.date.month(), gps.date.year(),
            gps.time.hour() + 1, gps.time.minute(), gps.time.second(),
            gps.location.lng(), gps.location.lat(),
            (int)gps.altitude.meters(), (int)gps.speed.kmph(), gps.satellites.value(), vario);

    diyFile.println(buffer);
    diyFile.sync();  // Assicurati che i dati siano scritti immediatamente

    // Calcola e aggiorna i valori massimi
    calculateMaxValues(currentAltitude, currentSpeed, vario);

    // Calcola la durata
    char durationBuffer[20];
    calculateDuration(currentTime, durationBuffer);

    // Sovrascrivi i valori massimi nel file
    if (MaxdiyFile.open("MaxData.txt", O_RDWR | O_CREAT | O_TRUNC)) {
      char maxBuffer[200];
      sprintf(maxBuffer, "%.2f, %.2f, %.2f, %.2f, %s, %s, %s, %s",
              maxalt, maxspeed, maxvario, course, durationBuffer, FlightStart.c_str(), EndFlight.c_str(),Date.c_str());
      MaxdiyFile.print(maxBuffer);  // Usa print invece di println per evitare di aggiungere una nuova riga
      MaxdiyFile.sync();
      MaxdiyFile.close();
    }

    digitalWrite(ledPin, LOW);  // turn the LED on (HIGH is the voltage level)
  } else {
    ledPin = 11;
    Serial.println("looking");
    digitalWrite(ledPin, LOW);  // turn the LED on (HIGH is the voltage level)
    delay(TState);
    digitalWrite(ledPin, HIGH);  // turn the LED off by making the voltage LOW
    delay(TState);
  }

  DelayGPS(200);
}

//------------------------------------------------------------------------------
void calculateVarioAndCourse(float currentAltitude, float currentSpeed, unsigned long currentTime) {
  if (currentTime - previousTime >= interval) {
    vario = (currentAltitude - previousAltitude) / (interval / 1000.0);  // Calcola la velocità di variazione dell'altitudine in m/s
    course += (currentSpeed * (interval / 3600000.0)) * 1000;  // Calcola la distanza percorsa in metri
    previousAltitude = currentAltitude;
    previousTime = currentTime;
  }
}

//------------------------------------------------------------------------------
void calculateMaxValues(float currentAltitude, float currentSpeed, float vario) {
  if (currentAltitude > maxalt) {
    maxalt = currentAltitude;
  }
  if (currentSpeed > maxspeed) {
    maxspeed = currentSpeed;
  }
  if (vario > maxvario) {
    maxvario = vario;
  }
}

//------------------------------------------------------------------------------
void calculateFlightTimes() {
  char timeBuffer[20];
  if (FlightStart == "") {
    sprintf(timeBuffer, "%02d:%02d:%02d", gps.time.hour() + 1, gps.time.minute(), gps.time.second());
    FlightStart = String(timeBuffer);
  }
  sprintf(timeBuffer, "%02d:%02d:%02d", gps.time.hour() + 1, gps.time.minute(), gps.time.second());
  EndFlight = String(timeBuffer);
}

//------------------------------------------------------------------------------
void calculateDuration(unsigned long currentTime, char* buffer) {
  unsigned long elapsedTime = currentTime - startTime;
  int hours = elapsedTime / 3600000;
  int minutes = (elapsedTime % 3600000) / 60000;
  int seconds = (elapsedTime % 60000) / 1000;
  sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
}

//------------------------------------------------------------------------------
void DelayGPS(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

//------------------------------------------------------------------------------
void closeFile() {
  if (diyFile.isOpen()) {
    diyFile.close();
  }
}
