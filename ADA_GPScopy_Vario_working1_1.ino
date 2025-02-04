#include <SD.h>
#include <SPI.h>
#include <Adafruit_GPS.h>
#include <Adafruit_MPL3115A2.h>

// Define pins for the SD card and GPS
#define SD_CS_PIN 4

// Initialize GPS and barometer
Adafruit_GPS GPS(&Serial1);
Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();

uint32_t timer = millis();
String folderName;
String datiVoloPath;
String maxDatiPath;
String FlightStart = "";
String EndFlight = "";
String Date = "";
String duration = "";

float previousAltitude = 0.0;
float previousTime = 0.0;
float vario = 0.00;
float maxalt = 0.00;
float maxspeed = 0.00;
float maxvario = 0.00;
float course = 0.00;
float ElapsedTimeInThermal = 0.0; // Elapsed time with positive vario
int maxTimeInThermal = 0; // Maximum time in thermal

char durationBuffer[20];
const unsigned long interval = 2000;  // Intervallo di 2000 millisecondi
const float TVario = 2.0; // User-defined period for vario timing (seconds)

unsigned long positiveStartTime = 0;
unsigned long negativeStartTime = 0;
bool isPositivePeriod = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(8, OUTPUT);
  Serial.begin(115200);  // Initialize serial communication
  Serial1.begin(9600);
  if (!SD.begin(SD_CS_PIN)) {  // Initialize the SD card
    Serial.println("Errore nell'inizializzazione della scheda SD");
    return;
  }
  Serial.println("Scheda SD inizializzata");

  // Initialize GPS
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  delay(200);

  // Initialize barometer
  if (!baro.begin()) {
    Serial.println("Barometer initialization failed!");
    while (1);
  }
  
  // Set sea level pressure (e.g., 101325 Pa for standard sea level pressure)
  baro.setSeaPressure(1020);

  while (!GPS.fix) {  // Wait for the GPS to get a fix
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
  createFolderAndOpenFiles();  // Create the folder and files
  delay(30000);
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  while (Serial1.available()) {
    char c = GPS.read();  // Read data from the GPS
    if (GPS.newNMEAreceived()) {
      if (!GPS.parse(GPS.lastNMEA())) {
        return;
      }
      
      if (millis() - timer > 300) {  // Manage the write frequency with millis()
        timer = millis();            // reset the timer
        digitalWrite(8, HIGH);       // turn the LED on (HIGH is the voltage level)
        File datiVoloFile;           // Write data to the files
        File maxDatiFile;
     
     // Read altitude from the barometer
      float currentAltitude = baro.getAltitude();
      float currentSpeed = (GPS.speed * 1852) / 1000;
      // Calculate current time in seconds from GPS data
      float currentTime = calculateCurrentTimeFromGPS(GPS);

      // Call calculateVario with the current altitude and current time
      calculateVario(currentAltitude, currentTime);
        if (datiVoloFile = SD.open(datiVoloPath.c_str(), FILE_WRITE)) {
          char buffer[200];
          sprintf(buffer, "%02d/%02d/%02d, %02d:%02d:%02d, %.4f , %.4f , %.2f , %.2f , %d , %.2f, %.2f",
                  (GPS.day), (GPS.month), (GPS.year),
                  GPS.hour + 1, GPS.minute, GPS.seconds,  // GPS.milliseconds, %03d
                  GPS.longitude, GPS.latitude,
                  currentAltitude, currentSpeed, GPS.satellites, vario, ElapsedTimeInThermal);
          datiVoloFile.println(buffer);
          datiVoloFile.close();
        } else {
          Serial.println("Errore nella scrittura del file DatiVolo.txt");
        }

        calculateFlightTimes();
        calculateDuration();                                         // Calculate flight duration
        calculateMaxValues(currentAltitude, currentSpeed, vario, ElapsedTimeInThermal); // Calculate max values

        if (maxDatiFile = SD.open(maxDatiPath.c_str(), O_RDWR)) {
          char maxBuffer[200];  // Overwrite the max values in the file
          sprintf(maxBuffer, "%.2f, %.2f, %.2f, %.2f, %s, %s, %s, %s, %d", maxalt, maxspeed, maxvario, course, durationBuffer, FlightStart.c_str(), EndFlight.c_str(), Date.c_str(), maxTimeInThermal);
          maxDatiFile.print(maxBuffer);  // Use print instead of println to avoid adding a new line
          maxDatiFile.close();
        } else {
          Serial.println("Errore nella scrittura del file MaxDati.txt");
        }
        digitalWrite(8, LOW);  // turn the LED off
      }
    }
  }
  digitalWrite(LED_BUILTIN, HIGH);
}

// Function to calculate current time in seconds from GPS data
float calculateCurrentTimeFromGPS(Adafruit_GPS &gps) {
  float currentTime = (gps.hour * 3600.0) + (gps.minute * 60.0) + gps.seconds + (gps.milliseconds / 1000.0);
  return currentTime;
}

void createFolderAndOpenFiles() {
  Date = String(GPS.day) + "/" + String(GPS.month) + "/" + String(GPS.year);
  folderName = String(GPS.day) + String(GPS.month) + String(GPS.hour) + String(GPS.minute);  // Create the folder name
  if (!SD.exists(folderName.c_str())) {
    if (SD.mkdir(folderName.c_str())) {
      Serial.println("Cartella creata con successo: " + folderName);
    } else {
      Serial.println("Errore nella creazione della cartella: " + folderName);
    }
  } else {
    Serial.println("La cartella esiste già: " + folderName);
  }
  datiVoloPath = folderName + "/DatiVolo.txt";  // Create the files inside the folder
  maxDatiPath = folderName + "/MaxDati.txt";
  File datiVoloFile;
  File maxDatiFile;

  if (datiVoloFile = SD.open(datiVoloPath.c_str(), FILE_WRITE)) {
    datiVoloFile.close();
  } else {
    Serial.println("Errore nella creazione del file DatiVolo.txt");
  }
  if (maxDatiFile = SD.open(maxDatiPath.c_str(), FILE_WRITE)) {
    maxDatiFile.close();
  } else {
    Serial.println("Errore nella creazione del file MaxDati.txt");
  }
}

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
  Serial.println(durationBuffer);
}

void calculateVario(float currentAltitude, float currentTime) {
  // Calculate the rate of altitude change in m/s
  float timeChange;
  if (previousTime != 0) {  // Ensure this is not the first reading
    float altitudeChange = currentAltitude - previousAltitude;
   timeChange = currentTime - previousTime;  // Time change in seconds

    // Ensure timeChange is not zero to avoid division by zero and check for reasonable timeChange
    if (timeChange > 0.01) {  // Time change should be at least 1 millisecond to avoid excessively high vario values
      vario = altitudeChange / timeChange;
    } else {
      vario = 0;
    }
  }

  // Update previous values
  previousAltitude = currentAltitude;
  previousTime = currentTime;

  // Calculate the elapsed time with positive vario
  if (vario > 0) {
    if (!isPositivePeriod) {
      // Start the positive period timing if not already started
      positiveStartTime = millis();
      isPositivePeriod = true;
    } else if (millis() - positiveStartTime >= TVario * 1000) {
      // Positive period has been active for more than TVario
      negativeStartTime = 0;  // Reset negative period timing
      ElapsedTimeInThermal += timeChange;  // Add the time change to ElapsedTimeInThermal
    }
  } else {
    if (isPositivePeriod) {
      // Vario turned negative, start negative period timing
      if (negativeStartTime == 0) {
        negativeStartTime = millis();
      } else if (millis() - negativeStartTime >= TVario * 1000) {
        // Negative period has been active for more than TVario
        isPositivePeriod = false;
        positiveStartTime = 0;  // Reset positive period timing
      }
    }
  }
}

void calculateMaxValues(float currentAltitude, float currentSpeed, float vario, float ElapsedTimeInThermal) {
  if (currentAltitude > maxalt) {
    maxalt = currentAltitude;
  }
  if (currentSpeed > maxspeed) {
    maxspeed = currentSpeed;
  }
  if (vario > maxvario) {
    maxvario = vario;
  }
  if (ElapsedTimeInThermal > maxTimeInThermal) {
    maxTimeInThermal = ElapsedTimeInThermal;
  }
}