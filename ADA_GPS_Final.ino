#include <SD.h>
#include <SPI.h>
#include <Adafruit_GPS.h>

// Definisci i pin per la scheda SD e il GPS
#define SD_CS_PIN 4

uint32_t timer = millis();
// Inizializza il GPS
Adafruit_GPS GPS(&Serial1);
String folderName;
String datiVoloPath;
String maxDatiPath;
String FlightStart = "";
String EndFlight = "";
String Date = "";
String duration = "";

float previousAltitude = 0.0;
unsigned long previousTime = 0;
float vario = 0.00;
float maxalt = 0.00;
float maxspeed = 0.00;
float maxvario = 0.00;
float course = 0.00;
char durationBuffer[20];
const unsigned long interval = 2000;  // Intervallo di 2000 millisecondi

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(8, OUTPUT);
  Serial.begin(115200);  // Inizializza la comunicazione seriale
  Serial1.begin(9600);
  if (!SD.begin(SD_CS_PIN)) {  // Inizializza la scheda SD
                               //  Serial.println("Errore nell'inizializzazione della scheda SD");
    return;
  }
  //Serial.println("Scheda SD inizializzata");
  GPS.begin(9600);  // Inizializza il GPS
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  delay(200);

  while (!GPS.fix) {  // Attendi che il GPS ottenga un fix
    char c = GPS.read();
    digitalWrite(LED_BUILTIN, LOW);
    if (GPS.newNMEAreceived()) {
      if (!GPS.parse(GPS.lastNMEA())) {
        digitalWrite(LED_BUILTIN, HIGH);
        continue;
      }
    }
    digitalWrite(LED_BUILTIN, HIGH);
  }
  createFolderAndOpenFiles();  // Crea la cartella e i file
  delay(30000);
}
void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  char c = GPS.read();  // Leggi i dati dal GPS
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) {
      return;
    }
  }
  if (millis() - timer > 300) {  // Gestisci la frequenza di scrittura con millis()
    timer = millis();            // reset the timer
    digitalWrite(8, HIGH);       // turn the LED on (HIGH is the voltage level)
    File datiVoloFile;           // Scrivi i dati nei file
    File maxDatiFile;
    /* Serial.print("Percorso datiVoloPath: ");
    Serial.println(datiVoloPath);
    Serial.print("Percorso maxDatiPath: ");
    Serial.println(maxDatiPath);*/
    float currentAltitude = GPS.altitude;
    float currentSpeed = (GPS.speed * 1852) / 1000;
    Serial.println(currentSpeed);
    if (datiVoloFile = SD.open(datiVoloPath.c_str(), FILE_WRITE)) {
      char buffer[200];
      sprintf(buffer, "%02d/%02d/%02d, %02d:%02d:%02d, %.4f , %.4f , %.2f , %.2f , %d , %.2f",
              (GPS.day), (GPS.month), (GPS.year),
              GPS.hour + 1, GPS.minute, GPS.seconds,  //GPS.milliseconds, .%03d
              GPS.longitude, GPS.latitude,
              currentAltitude, currentSpeed, GPS.satellites, vario);  //(int)GPS.speed
      datiVoloFile.println(buffer);
      datiVoloFile.close();
    } else {
      // Serial.println("Errore nella scrittura del file DatiVolo.txt");
    }
    calculateFlightTimes();
    // char durationBuffer[20];
    calculateDuration();                                         // Calcola la durata del volo
    calculateVarioAndCourse(GPS.altitude, GPS.speed, millis());  // Calcola il vario
    calculateMaxValues(GPS.altitude, GPS.speed, vario);          // Calcola i valori massimi

    if (maxDatiFile = SD.open(maxDatiPath.c_str(), O_RDWR)) {
      char maxBuffer[200];  // Sovrascrivi i valori massimi nel file
                            // sprintf(maxBuffer, "%.2f, %.2f, %.2f, %.2f, %s, %s, %s, %s", maxalt, maxspeed, maxvario, course, durationBuffer, FlightStart.c_str(), EndFlight.c_str(), Date.c_str());
      sprintf(maxBuffer, "%.2f, %.2f, %.2f, %.2f, %s, %s, %s, %s", maxalt, maxspeed, maxvario, course, durationBuffer, FlightStart.c_str(), EndFlight.c_str(), Date.c_str());
      maxDatiFile.print(maxBuffer);  // Usa print invece di println per evitare di aggiungere una nuova riga
      maxDatiFile.close();
    } else {
      //Serial.println("Errore nella scrittura del file MaxDati.txt");
    }
    digitalWrite(8, LOW);  // turn the LED on (HIGH is the voltage level)
  }
  digitalWrite(LED_BUILTIN, HIGH);
}
void createFolderAndOpenFiles() {
  Date = String(GPS.day) + "/" + String(GPS.month) + "/" + String(GPS.year);
  folderName = String(GPS.day) + String(GPS.month) + String(GPS.hour) + String(GPS.minute);  // Crea il nome della cartella
  if (!SD.exists(folderName.c_str())) {
    if (SD.mkdir(folderName.c_str())) {
      //Serial.println("Cartella creata con successo: " + folderName);
    } else {
      //Serial.println("Errore nella creazione della cartella: " + folderName);
    }
  } else {
    //Serial.println("La cartella esiste già: " + folderName);
  }
  datiVoloPath = folderName + "/DatiVolo.txt";  // Crea i file all'interno della cartella
  maxDatiPath = folderName + "/MaxDati.txt";
  File datiVoloFile;
  File maxDatiFile;
  /* Serial.print("Percorso datiVoloPath: ");
  Serial.println(datiVoloPath);
  Serial.print("Percorso maxDatiPath: ");
  Serial.println(maxDatiPath);*/
  if (datiVoloFile = SD.open(datiVoloPath.c_str(), FILE_WRITE)) {
    datiVoloFile.close();
  } else {
    //Serial.println("Errore nella creazione del file DatiVolo.txt");
  }
  if (maxDatiFile = SD.open(maxDatiPath.c_str(), FILE_WRITE)) {
    maxDatiFile.close();
  } else {
    //Serial.println("Errore nella creazione del file MaxDati.txt");
  }
}
//------------------------------------------------------------------------------
void calculateFlightTimes() {
  char timeBuffer[20];
  if (FlightStart == "") {
    sprintf(timeBuffer, "%02d:%02d:%02d", GPS.hour + 1, GPS.minute, GPS.seconds);
    FlightStart = String(timeBuffer);
  }
  sprintf(timeBuffer, "%02d:%02d:%02d", GPS.hour + 1, GPS.minute, GPS.seconds);
  EndFlight = String(timeBuffer);
}
void calculateDuration() {
  int startHour, startMinute, startSecond;
  int endHour, endMinute, endSecond;

  sscanf(FlightStart.c_str(), "%02d:%02d:%02d", &startHour, &startMinute, &startSecond);
  sscanf(EndFlight.c_str(), "%02d:%02d:%02d", &endHour, &endMinute, &endSecond);

  int durationSeconds = (endHour * 3600 + endMinute * 60 + endSecond) - (startHour * 3600 + startMinute * 60 + startSecond);
  int hours = durationSeconds / 3600;
  int minutes = (durationSeconds % 3600) / 60;
  int seconds = durationSeconds % 60;
  sprintf(durationBuffer, "%02d:%02d:%02d", hours, minutes, seconds);
  //return String(durationBuffer);
  //sscanf(duration.c_str(), "%02d:%02d:%02d",&hours, &minutes, &seconds);
  Serial.println(durationBuffer);
}
void calculateVarioAndCourse(float currentAltitude, float currentSpeed, unsigned long currentTime) {
  if (currentTime - previousTime >= interval) {
    vario = (currentAltitude - previousAltitude) / (interval / 1000.0);  // Calcola la velocità di variazione dell'altitudine in m/s
    course += (currentSpeed * (interval / 3600000.0)) * 1000;            // Calcola la distanza percorsa in metri
    previousAltitude = currentAltitude;
    previousTime = currentTime;
  }
}
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
