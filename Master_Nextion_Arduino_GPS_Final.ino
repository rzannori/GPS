
/*     MaxData.txt "MaxAlt: , MaxSpeed: , MaxVario: , Course: , Durata: ";
       DatiVolo.txt Date,Time,location.lng(),location.lat(),altitude,speed, satellites, vario);
      
    */


#include "EasyNextionLibrary.h"  // Include EasyNextionLibrary
#include <SPI.h>
#include <SdFat.h>

EasyNex myNex(Serial);
SdFat SD;
SdFile dataFile;
const int chipSelect = 4;                                          // Pin di selezione della scheda SD
String Course, Durata, StartFlight, EndFlight, TimeGraph, Date;    // Variabili per i dati
int MaxAlt, MaxSpeed, MaxVario, AltGraph, SpeedGraph, VarioGraph;  // Variabili per i dati di volo
int Speed, Vario, Altm, Speedm, Variom;
int Alt;
float VarioFloat;

void setup() {
  myNex.begin(9600);
  Serial.begin(9600);
  if (!SD.begin(chipSelect)) {
    Serial.println("Errore nell'apertura della scheda SD!");
    return;
  }
  myNex.NextionListen();
  // Verifica l'esistenza dei file e legge i dati
  StaticDatadiy();
  myNex.writeNum("page0.p0.pic", 6);
  myNex.writeStr("page0.t14.txt", "SDCard Ready");
  myNex.writeNum("page0.p2.pic", 3);
  myNex.writeStr("page0.t11.txt", Date);  // l'indicazione della pagina rende il valore globale. quindi cambiando pagina viene memorizzato
  myNex.writeNum("page0.t11.pco", 2016);
  myNex.writeStr("page0.t12.txt", StartFlight);
  myNex.writeNum("page0.t12.pco", 2016);
}

void loop() {
  myNex.NextionListen();
}


// Sottoprogramma per leggere i dati di volo dal file DatiVolo.txt
void trigger1() {
  if (dataFile.open("DatiVolo.txt", O_READ)) {
    char line[100];
    while (dataFile.fgets(line, sizeof(line))) {  // Legge ogni riga del file
      Alt = getColumnValue(line, 4).toInt();
      AltGraph = map(Alt, 0, 350, 0, 255);             // Supponendo che il valore originale sia tra 0 e 350
      Speed = getColumnValue(line, 5).toInt();         // Legge la colonna 6 e converte in int
      SpeedGraph = map(Speed, 0, 250, 0, 255);         // Supponendo che il valore originale sia tra 0 e 250
      VarioFloat = getColumnValue(line, 7).toFloat();  // Legge la colonna 7 e converte in int
      Vario = VarioFloat * 100;
      VarioGraph = map(Vario, -600, 600, 0, 255);

      int buffer[6] = { Alt, AltGraph, Speed, SpeedGraph, Vario, VarioGraph };

      myNex.writeNum("n0.val", buffer[0]);
      myNex.writeNum("NAltGraph.val", buffer[1]);
      myNex.writeNum("n1.val", buffer[2]);
      myNex.writeNum("NSpeedGraph.val", buffer[3]);
      myNex.writeNum("x0.val", buffer[4]);
      myNex.writeNum("NVarioGraph.val", buffer[5]);
      delay(25);
    }
    dataFile.close();
  }
}
// Apertura e lettura del file MaxData.txt
void trigger2() {
  myNex.writeStr("page2.t15.txt", "  ");
  myNex.writeNum("p3.pic", 5);
  myNex.writeStr("page2.t20.txt", Durata);
  myNex.writeStr("page2.t21.txt", StartFlight);
  myNex.writeStr("page2.t22.txt", EndFlight);
  myNex.writeStr("page2.t23.txt", Course);
  myNex.writeNum("z1.val", Altm);
  myNex.writeNum("n2.val", MaxAlt);
  myNex.writeNum("z0.val", Speedm);
  myNex.writeNum("n3.val", MaxSpeed);
  myNex.writeNum("z2.val", Variom);
  myNex.writeNum("n4.val", MaxVario);
}

// Verifica l'esistenza dei file MaxData.txt e DatiVolo.txt
void StaticDatadiy() {
  if (SD.exists("MaxData.txt") && SD.exists("DatiVolo.txt")) {
    Serial.println("File MaxData.txt e DatiVolo.txt trovati");

    // Apertura e lettura del file MaxData.txt
    if (dataFile.open("MaxData.txt", O_READ)) {
      char line[100];
      dataFile.fgets(line, sizeof(line));          // Legge la prima riga del file
      MaxAlt = getColumnValue(line, 0).toInt();    // Legge la prima colonna e converte in int
      MaxSpeed = getColumnValue(line, 1).toInt();  // Legge la seconda colonna e converte in int
      MaxVario = getColumnValue(line, 2).toInt();  // Legge la terza colonna e converte in int
      Course = getColumnValue(line, 3);            // Legge la quarta colonna
      Durata = getColumnValue(line, 4);            // Legge la quinta colonna
      StartFlight = getColumnValue(line, 5);       // Legge la sesta colonna
      EndFlight = getColumnValue(line, 6);
      Date = getColumnValue(line, 7);
      dataFile.close();
      if (MaxAlt < 51) {
        Altm = MaxAlt + 310;
      } else {
        Altm = map(MaxAlt, 51, 300, 0, 230);
      }
      if (MaxSpeed < 51) {
        Speedm = MaxSpeed + 300;
      } else {
        Speedm = map(MaxSpeed, 51, 300, 0, 230);
      }
      Variom = map(MaxVario, -20, 20, 0, 180);


    } else {
      Serial.println("Errore nell'apertura del file MaxData.txt");
    }
  } else {
    Serial.println("File MaxData.txt o DatiVolo.txt non trovato");
  }
}


// Funzione per ottenere il valore di una colonna da una riga
String getColumnValue(char* data, int column) {
  int commaIndex = 0;
  int startIndex = 0;
  int endIndex = 0;
  for (int i = 0; i <= column; i++) {
    startIndex = endIndex;
    while (data[endIndex] != ',' && data[endIndex] != '\0') {
      endIndex++;
    }
    if (i == column) {
      return String(data + startIndex).substring(0, endIndex - startIndex);
    }
    endIndex++;
  }
  return "";
}
