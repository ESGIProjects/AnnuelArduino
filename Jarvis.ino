#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>
#include <Keypad.h>
#include <ctype.h>
#include <RFID.h>

/**
   PINS 4 AND 10 ARE USED BY THE SHIELD !!!
*/

/**
   Pins for RFID reader
*/
#define RST_PIN 9
#define SDA_PIN 8

/**
   Variables for manage password
*/
File passwordFile;
String password;
String attempt;

/**
   Variable for Buzzer
*/
const int buzzer = 53;
;

/**
   Variables for Ethernet Shield
*/
byte mac[] = { 0xDC, 0xAD, 0xBA, 0xEF, 0xFA, 0xED };
EthernetServer server(80); //port 80 is default for HTTP
EthernetClient client;
IPAddress ip(192, 168, 1, 37);
char serverName[] = "jarvis-esgi.herokuapp.com";

/**
   Variables for Keyboard
*/
const byte R_SIZE = 4;
const byte C_SIZE = 4;
char keys[R_SIZE][C_SIZE] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[R_SIZE] = {36, 34, 32, 30}; //black pins on keyboard
byte colPins[C_SIZE] = {37, 35, 33, 31};
Keypad kp = Keypad(makeKeymap(keys), rowPins, colPins, R_SIZE, C_SIZE);

/**
   Variables for PIR (5V)
*/
int pirPin = 2;
int pirState = LOW;
int val = 0;
int calibrationDelay = 30;

/**
   Variable for RFID reader
*/
RFID rfid(SDA_PIN, RST_PIN);
int UID[5];

boolean isStandBy = true;
boolean isSetAlarm = false;

void setup() {
  Serial.begin(9600);
  noTone(buzzer);
  pinMode(buzzer, OUTPUT);
  pinMode(pirPin, INPUT);
  initializeSD();
  setPassword("1234");
  checkPassword("1234");
  SPI.begin();
  initEthernetConnection();
  pirCalibration();
  standBy();
  //rfid.init();
}

void loop() {
  alarm();
  if (isSetAlarm) {
    if (attempt.length() == 4) {
      checkPassword(attempt);
      attempt = "";
      isStandBy = true;
    }
    readKeyboard();
    alarmEnable();
  }
  else {
    if (isStandBy) {
      standBy();
    }
    analyze();
    //readKeyboard();
    //getPassword();
    //checkRFID();
  }
}

void initEthernetConnection() {
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());
}

void alarmEnable() {
  EthernetClient client = server.available();
  if (client) {
    Serial.println("New client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        String postPassword;
        char c = client.read();
        Serial.write(c);
        if (c == '\n' && currentLineIsBlank) {
          char inData[20];
          char inChar;
          byte index = 0;
          while (client.available()) {
            inChar = client.read();
            inData[index] = inChar;
            index++;
            inData[index] = '\0';
          }

          String attempt(inData);
          String attemptTest;

          for (int i = 0; i < attempt.length(); i++) {
            if (isDigit(attempt.charAt(i))) {
              attemptTest = attemptTest + attempt.charAt(i);
            }
          }

          Serial.println(attemptTest);
          Serial.println();

          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          if (checkPassword(attemptTest)) {
            Serial.println("The password is correct !");
            //we shutdown the alarm sound
            isSetAlarm = false;
            client.println("success");
          }
          else {
            Serial.println("The password is incorrect !");
            client.println("failure");
          }
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void postRequest() {
  if (client.connect(serverName, 80)) {
    Serial.println("Connection succeeded !");
    // build the POST request
    Serial.println("Sending post request...");
    client.println("POST /api/alert HTTP/1.1");
    client.println("Host: jarvis-esgi.herokuapp.com");
    client.println("Content-Type: text/plain");
    client.println("Cache-Control: no-cache");
    client.println();
    Serial.println("Request is send !");
    client.stop();
  }
  else {
    Serial.println("Connection failed !");
    shutdown();
  }
}

void checkRFID() {
  if (rfid.isCard()) {
    if (rfid.readCardSerial()) {
      Serial.print("L'UID est: ");
      for (int i = 0; i <= 4; i++) {
        UID[i] = rfid.serNum[i];
        Serial.print(UID[i], DEC);
        Serial.print(".");
      }
      Serial.println("");
    }
    rfid.halt();
  }
  delay(100);
}

void alarm() {
  if (isSetAlarm) {
    tone(buzzer, 1000);
  }
  else {
    noTone(buzzer);
  }
}

void analyze() {
  val = digitalRead(pirPin);
  Serial.println(val);
  if (val == HIGH) {
    delay(300);
    if (pirState == LOW) {
      Serial.println("Motion detected !");
      pirState = HIGH;
      postRequest();
      isSetAlarm = true;
    }
  }
  else {
    delay(300);
    if (pirState == HIGH) {
      Serial.println("Motion ended !");
      pirState = LOW;
    }
  }
}

void pirCalibration() {
  Serial.println("Pir Sensor Calibration !");
  for (int i = 0; i < calibrationDelay; i++) {
    Serial.println(i);
    delay(1000);
  }
}

char readKeyboard() {
  char key = kp.getKey();
  if (isSetAlarm) {
    if (key != NO_KEY && isDigit(key)) {
      Serial.println(key);
      attempt += key;
    }
  }
  return key;
}

void initializeSD() {
  /* we check if the communication is OK with the pin 4 which is the pin used by
    the shield for the SD card port*/
  if (!SD.begin(4)) {
    // if it's not OK, we stop the program
    Serial.println("Communication is NOK ! \n ");
    shutdown();
  }
  Serial.println("Communication is OK !");
}

boolean checkPassword(String attempt) {
  passwordFile = SD.open("pf.txt");
  String content;
  while (passwordFile.available()) {
    content = content + (char)passwordFile.read();
  }
  passwordFile.close();
  if (content.equals(attempt)) {
    Serial.println("Success !");
    isSetAlarm = false;
    return true;
  }
  Serial.println("Failure !");
  return false;
}

void setPassword(String password) {
  if (password.length() > 4) {
    Serial.println("The password is too long !");
    return;
  }
  else if (password.length() < 4) {
    Serial.println("The password is too short !");
    return;
  }
  else if (!isValidPassword(password)) {
    Serial.println("The password must contains only numbers !");
    return;
  }

  //if the file "pf.txt" exists, it will be erased
  if (SD.exists("pf.txt")) {
    SD.remove("pf.txt");
    Serial.println("The password file is erased !");
  }

  //create the file "pf.txt"
  passwordFile = SD.open("pf.txt", FILE_WRITE);
  passwordFile.print(password);
  passwordFile.close();
  Serial.println("The password is defined !");
}

boolean isValidPassword(String password) {
  for (byte i = 0; i < password.length(); i++) {
    if (!isDigit(password.charAt(i))) {
      return false;
    }
  }
  return true;
}

void shutdown() {
  Serial.println("The program is shutdown !");
  while (1) {
    //we create an infinite loop in order to simulate a shutdown
  }
}

void standBy() {
  if (isStandBy) {
    Serial.println("The program is in stand by !");
    Serial.println("Press D if you want to start the alarm !");
    while (isStandBy) {
      if (readKeyboard() == 'D') {
        for (int i = 0; i < 10; i++) {
          tone(buzzer, 1000,100);
          Serial.println(i);
          delay(1000);
          noTone(buzzer);
        }
        tone(buzzer, 1000, 2000);
        delay(2000);
        Serial.println("The program is running !");
        isStandBy = false;
      }
    }
  }
}
