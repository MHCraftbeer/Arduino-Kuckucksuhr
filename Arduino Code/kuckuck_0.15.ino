/*
  Version mit Lautsprecher
  Changelog
  0.15 Lichtsensor über Serialmonitor einstellen und im eeprom speichern
  0.14 Bug das es um 13 uhr 13 mal schlug entfernt
  0.13 Einstellung hinzugefügt um zwischen lautsprecher und kuckuck hin und herzuschalten
  0.12 
  0.11 Transistor fuer lautsprecher eingebaut, gegen rauschen. Elko in reihe zu LS eingebaut gegen knacken beim schalten des transistors
  0.10 Lautsprecher inkl sd karte integriert (lautsprecher kann abhaengig vom netzteil durchgaengig rauschen)
  0.05 Uhrzeit kann jetzt fuer 60sec beim anschliessen ueber serial monitor eingestellt werden
  0.04 Einstellung fuer postionen raus und rein hinzugefuegt
  0.03 Stromsparmodus integriert
  0.02 Lichtsensor integriert damit kuckuck nur kommt wenns hell ist
  0.01 sommerzeit eingebaut, servo funktioniert
  31,10,2018 11,59,55
*/

const float firmware = 0.15;

//https://www.elecrow.com/wiki/index.php?title=Tiny_RTC

#include <DS3231.h>
#include <Wire.h>
#include <EEPROM.h>
DS3231 Clock;
bool Century=false;
bool h12;
bool PM;
byte ADay, AHour, AMinute, ASecond, ABits;
int eepromaddr = 0;

#include <Servo.h>
Servo myservo;
#include <Adafruit_SleepyDog.h>
#include <SD.h>  
#include <TMRpcm.h>
TMRpcm tmrpcm;
#include <SPI.h>

//Pins
const byte SERVO = 3;
const byte LICHT = A3;
const byte SPEAKER = 9;
const byte CS = 4;
const byte TRANS = A0;

//Einstellungen
const bool speaker = true; //schaltet zwischen lautsprecher und anderen modus
const byte posraus = 60; //je niedriger desto weiter raus
const byte posrein = 190; //je hoeher desto weiter rein
int nacht = 100;

//Programme
boolean summertime_EU(int year, byte month, byte day, byte hour, byte tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{
  if (month < 3 || month > 10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (month > 3 && month < 10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if (month == 3 && (hour + 24 * day) >= (1 + tzHours + 24 * (31 - (5 * year / 4 + 4) % 7)) || month == 10 && (hour + 24 * day) < (1 + tzHours + 24 * (31 - (5 * year / 4 + 1) % 7)))
    return true;
  else
    return false;
}

void kuckuckspeaker(byte count) {
  byte pos;
  byte i;
  myservo.attach(SERVO);
  digitalWrite(TRANS, HIGH);
  for (pos = posrein; pos > posraus; pos -= 1) { //raus
    myservo.write(pos);
    delay(4);
  }
  myservo.detach();
  delay(300);
  for (i = 1; i <= count; i++) {
    tmrpcm.play("cuckoo.wav");
    delay(1200);
  }
  myservo.attach(SERVO);
  digitalWrite(TRANS, LOW);
  for (pos = posraus; pos < posrein; pos += 1) { //rein
    myservo.write(pos);
    delay(4);
  }
  myservo.detach();
}

void kuckuck(byte count) {
  byte pos;
  byte i;
  for (i = 1; i <= count; i++) {
    myservo.attach(SERVO);
    for (pos = posrein; pos > posraus; pos -= 1) { //raus
      myservo.write(pos);
      delay(4);
    }
    myservo.detach();
    delay(200);
    myservo.attach(SERVO);
    for (pos = posraus; pos < posrein; pos += 2) { //rein
      myservo.write(pos);
      delay(5);
    }
    myservo.detach();
    delay(1000);
  }
}

void setup () {
  Wire.begin();
  Serial.begin(9600);
  pinMode(LICHT, INPUT);
  pinMode(TRANS, OUTPUT);
  digitalWrite(TRANS, LOW);
  Serial.print("Kuckucksuhr Firmware: ");
  Serial.println(firmware);
  if (speaker) {
    tmrpcm.speakerPin = SPEAKER;
    if (!SD.begin(CS)) {  // see if the card is present and can be initialized:
      Serial.println("SD fail");  
      return;   // don't do anything more if not
    }
    Serial.println("SD ok");  
  }
  Serial.print("T=");
  Serial.print(Clock.getTemperature(), 2);
  Serial.println(" C");
  if (Clock.oscillatorCheck()) {
    Serial.println("Oszillatorcheck: OK");
  } else {
    Serial.println("Oszillatorcheck: Error");
  }
  Serial.println();
  Serial.println("Internal time: ");
  Serial.print(Clock.getDate(),DEC);
  Serial.print('.');
  Serial.print(Clock.getMonth(Century), DEC);
  Serial.print('.');
  int jahr = 2000;
  if (Century) jahr += 100;
  jahr += Clock.getYear();
  Serial.print(jahr);
  Serial.print(" ");
  Serial.print(Clock.getHour(h12, PM),DEC);
  Serial.print(':');
  Serial.print(Clock.getMinute(), DEC);
  Serial.print(':');
  Serial.println(Clock.getSecond(), DEC);
  Serial.println("Light threshold number: ");
  nacht = EEPROM.read(0);
  Serial.println(nacht);
  unsigned long t = millis();
  byte c = 0;
  byte i = 0;
  byte da[3];
  byte mo[3];
  byte ye[5];
  byte ho[3];
  byte mi[3];
  byte se[3];
  byte li[4];
  Serial.println("Enter time and light threshold in format: day.month.year_hour:minute:second_light (xx.xx.xxx_xx:xx:xx_xxx)");
  Serial.println("The light treshold must be an integer number between 0 and 255");
  while (i < 100 && c < 60) {
    if (millis() > t + 2000) {
    t = millis();
    c++;
    c++;
    Serial.print(60 - c);Serial.println(" seconds left");
    }
    if (Serial.available() > 0) {
       i++;
       byte x = Serial.read();
       x -= 48;
       switch (i) {
         case 1:
           da[1] = x;
           break;
         case 2:
           da[2] = x;
           break;
         case 4:
           mo[1] = x;
           break;
         case 5:
           mo[2] = x;
           break;
         case 7:
           ye[1] = x;
           break;
         case 8:
           ye[2] = x;
           break;
         case 9:
           ye[3] = x;
           break;
         case 10:
           ye[4] = x;
           break;
         case 12:
           ho[1] = x;
           break;
         case 13:
           ho[2] = x;
           break;
         case 15:
           mi[1] = x;
           break;
         case 16:
           mi[2] = x;
           break;
         case 18:
           se[1] = x;
           break;
         case 19:
           se[2] = x;
           break;
         case 21:
           li[1] = x;
           break;
         case 22:
           li[2] = x;
           break;
         case 23:
           li[3] = x;
           i = 100;
           int da1 = da[1] * 10 + da[2];
           int mo1 = mo[1] * 10 + mo[2];
           int ye1 = ye[1] * 1000 + ye[2] *100 + ye[3] * 10 +ye[4];
           ye1 -= 2000;
           int ho1 = ho[1] * 10 + ho[2];
           int mi1 = mi[1] * 10 + mi[2];
           int se1 = se[1] * 10 + se[2];
           int li1 = li[1] * 100 + li[2] * 10 + li[3];
           Serial.print("Time and date: ");
           Serial.print(da1);
           Serial.print(".");
           Serial.print(mo1);
           Serial.print(".");
           Serial.print(ye1);
           Serial.print(" ");
           Serial.print(ho1);
           Serial.print(":");
           Serial.print(mi1);
           Serial.print(":");
           Serial.println(se1);
           Serial.print("Light threshold: ");
           Serial.println(li1);
           nacht = li1;
           EEPROM.write(0, nacht);
           delay(100);
           Clock.setClockMode(false); // set to 24h
           Clock.setYear(ye1);
           Clock.setMonth(mo1);
           Clock.setDate(da1);
           Clock.setHour(ho1);
           Clock.setMinute(mi1);
           Clock.setSecond(se1);
           delay(2000);
           Serial.println("Internal time was updated according to Input!");
           Serial.print(Clock.getDate(),DEC);
           Serial.print('.');
           Serial.print(Clock.getMonth(Century), DEC);
           Serial.print('.');
           int jahr = 2000;
           if (Century) jahr += 100;
           jahr += Clock.getYear();
           Serial.print(jahr);
           Serial.print(" ");
           Serial.print(Clock.getHour(h12, PM),DEC);
           Serial.print(':');
           Serial.print(Clock.getMinute(), DEC);
           Serial.print(':');
           Serial.print(Clock.getSecond(), DEC);
           Serial.println();
           break;
      }
    }
  }
  delay(2000);
}
void loop () {
  int a = analogRead(LICHT);
  //Serial.println(a);
  //delay(100);
  if (a < 4 * nacht) Watchdog.sleep();
  else {
    byte minut = Clock.getMinute();
    byte sekund = Clock.getSecond();
    if (minut != 59) Watchdog.sleep();
    else if (sekund < 50) Watchdog.sleep();
    else {
      int jahr = 2000;
      if (Century) jahr += 100;
      jahr += Clock.getYear();
      byte monat = Clock.getMonth(Century);
      byte tag = Clock.getDate();
      byte stund = Clock.getHour(h12, PM);
      if (summertime_EU(jahr, monat, tag, stund, 1)) {
        stund += 1;
        if (stund == 24) stund = 0;
      }
      byte count = stund + 1;
      if (count > 12) count -= 12;
      if (minut == 59 && sekund == 59) {
        if (speaker) kuckuckspeaker(count);
        else kuckuck(count);
      }
    }
  }
}
