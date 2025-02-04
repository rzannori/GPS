#include <SdFat.h>
#include <SPI.h>
#include "EasyNextionLibrary.h"


EasyNex myNex(Serial);
SdFat SD;
SdFile dataFile;
int folderCount = 0;
char folderNames[4][13];  // Array di caratteri per memorizzare i nomi delle cartelle
char folderName[13];
String selectedFolder;

int MaxAlt, MaxSpeed, Altm, Speedm, MaxVario, Variom;
int Alt, Speed, Vario;
float VarioFloat, MVarioFloat, Course;
String Last, StartFlight, EndFlight, Date;


void setup() {
  myNex.begin(9600);
  Serial.begin(9600);
  if (!SD.begin(4)) {  // Inizializza la scheda SD
    myNex.writeStr("page0.t14.txt", "SDCard fail!");
    return;
  } else {

    myNex.writeNum("page0.t14.pco", 1024);
    myNex.writeStr("page0.t14.txt", "SDCard Ready");
  }
  SdFile root;  // Elenca le cartelle
  if (!root.open("/")) {
    return;
  }

  listFolders(root);
  myNex.writeStr("b50.txt", folderNames[0]);  // folder fittizio perche altrimenti salta la prima lettura
  myNex.writeStr("b3.txt", folderNames[0]);
  myNex.writeStr("b4.txt", folderNames[1]);
  myNex.writeStr("b5.txt", folderNames[2]);
  myNex.writeStr("b6.txt", folderNames[3]);
}
void loop() {
  myNex.NextionListen();  // Il loop è vuoto perché non è necessario per questo esempio
  delay(500);
}
//----------------------------- Funzione per elencare le cartelle nel setup
void listFolders(SdFile& dir) {
  SdFile file;
  while (file.openNext(&dir, O_RDONLY) && folderCount < 5) {
    if (file.isDir()) {
      file.getName(folderNames[folderCount], 13);
      //Serial.println(folderNames[folderCount]);
      if (strlen(folderNames[folderCount]) == 0) {
        file.close();
        continue;
      }
      folderCount++;
    }
    file.close();
  }
}
// ------------------------------------- NEXTION chiama il sottoprogramma che legge i dati di volo dal file DatiVolo.txt
void trigger1() {
  DatiVoloFilePath();
}
//---------------------------------------------NEXTION  richiama dati per schermata max data
void trigger2() {
  myNex.writeStr("t29.txt", Date);
  myNex.writeStr("t21.txt", StartFlight);
  myNex.writeStr("t22.txt", EndFlight);
  myNex.writeStr("t20.txt", Last);
  myNex.writeNum("x20.val", Course);
  myNex.writeNum("n50.val", MaxAlt);  ///_______
  myNex.writeNum("n2.val", MaxAlt);
  myNex.writeNum("n3.val", MaxSpeed);
  MaxVario = MVarioFloat * 10;
  myNex.writeNum("x21.val", MaxVario);
  //if (MaxAlt < 71) {
  Altm = 270 + MaxAlt;
  // } else {
  //   Altm = MaxAlt - 71;
  // }
  myNex.writeNum("z1.val", Altm);
  //if (MaxSpeed < 71) {
  Speedm = 270 + MaxSpeed;
  // } else {
  // Speedm = MaxSpeed - 71;
  //}
  myNex.writeNum("z0.val", Speedm);
  Variom = (MaxVario * 2) / 10;
  myNex.writeNum("j0.val", Variom);
}
//---------------------------------------------NEXTION  seleziona la cartella con i dati
void trigger4() {
  strcpy(folderName, folderNames[0]);
  char MaxDatiFilePath[26];
  getFilePath(folderName, "MaxDati.txt", MaxDatiFilePath);
  Serial.print("percorso = ");
  Serial.println(MaxDatiFilePath);
  readMaxDatiFile(MaxDatiFilePath);
}
//---------------------------------------------NEXTION  seleziona la cartella con i dati
void trigger5() {
  strcpy(folderName, folderNames[1]);
  char MaxDatiFilePath[26];
  getFilePath(folderName, "MaxDati.txt", MaxDatiFilePath);
  readMaxDatiFile(MaxDatiFilePath);
}
//---------------------------------------------NEXTION  seleziona la cartella con i dati
void trigger6() {
  strcpy(folderName, folderNames[2]);
  char MaxDatiFilePath[26];
  getFilePath(folderName, "MaxDati.txt", MaxDatiFilePath);
  readMaxDatiFile(MaxDatiFilePath);
}
//---------------------------------------------NEXTION  seleziona la cartella con i dati
void trigger7() {
  strcpy(folderName, folderNames[3]);
  char MaxDatiFilePath[26];
  getFilePath(folderName, "MaxDati.txt", MaxDatiFilePath);
  readMaxDatiFile(MaxDatiFilePath);
}
// -------------------------------------Funzione per ottenere il percorso completo di un file
void getFilePath(const char* folderName, const char* fileName, char* filePath) {
  snprintf(filePath, 26, "%s/%s", folderName, fileName);
}
//--------------------------------------------- Ottieni il percorso del file DatiVolo.txt
void DatiVoloFilePath() {
  char datiVoloFilePath[26];
  getFilePath(folderName, "DatiVolo.txt", datiVoloFilePath);
  readDatiVoloFile(datiVoloFilePath);
}
// -------------------------------------legge i dati di volo dal file DatiVolo.txt
void readDatiVoloFile(const char* filePath) {
  if (dataFile.open(filePath, O_READ)) {
    char line[100];
    int lineNumber = 0;                           // Aggiungi un contatore di righe
    while (dataFile.fgets(line, sizeof(line))) {  // Legge ogni riga del file
      lineNumber++;                               // Incrementa il contatore di righe
      if (strlen(line) > 0)
        Alt = getColumnValue(line, 4).toInt();
      Speed = getColumnValue(line, 5).toInt();         // Legge la colonna 6 e converte in int
      VarioFloat = getColumnValue(line, 7).toFloat();  // Legge la colonna 7 e converte in int
      Vario = VarioFloat * 100;
      int buffer[3] = { Alt, Speed, Vario };  //{ Alt, AltGraph, Speed, SpeedGraph, Vario, VarioGraph };
      myNex.writeNum("n0.val", buffer[0]);
      myNex.writeNum("n1.val", buffer[1]);
      myNex.writeNum("x10.val", buffer[2]);
    }
    dataFile.close();
  }
}
//--------------------------------------------- legge i dati in MaxDati
void readMaxDatiFile(const char* filePath) {
  // if (SD.exists(filePath)) {
  if (dataFile.open(filePath, O_READ)) {
    //dataFile.open(filePath, O_READ);
    char line[100];
    dataFile.fgets(line, sizeof(line));               // Legge la prima riga del file
    MaxAlt = getColumnValue(line, 0).toInt();         // Legge la prima colonna e converte in int
    MaxSpeed = getColumnValue(line, 1).toInt();       // Legge la seconda colonna e converte in int
    MVarioFloat = getColumnValue(line, 2).toFloat();  // getColumnValue(line, 2).toFloat();
    Course = getColumnValue(line, 3).toFloat();       // Legge la quarta colonnagetColumnValue(line, 3).toFloat() * 10;
    Last = getColumnValue(line, 4);                   // Legge la quinta colonna
    StartFlight = getColumnValue(line, 5);
    EndFlight = getColumnValue(line, 6);
    Date = getColumnValue(line, 7);
    myNex.writeStr("page0.t11.txt", Date);
    myNex.writeStr("page0.t12.txt", StartFlight);
    dataFile.close();
  }
  //}
}
//-----------------------------------------------funzione per posizionarsi su specifica colonna
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
