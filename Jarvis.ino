#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>
#include <Keypad.h>
#include <ctype.h>
#include <RFID.h>

/**
 * Pins for RFID reader
 */
#define SS_PIN 9
#define RST_PIN 8

/**
 * Variables for manage password
 */
File passwordFile;
String password;
String attempt;

/**
 * Variable for Buzzer
 */
const int buzzer = 9;

/**
 * Variables for Ethernet Shield
 */
byte mac[] = { 0xDC, 0xAD, 0xBA, 0xEF, 0xFA, 0xED };
EthernetClient client;
IPAddress ip(192,168,1,1);
char serverName[] = "jarvis-esgi.herokuapp.com";

/**
 * Variables for Keyboard
 */
const byte R_SIZE = 4;
const byte C_SIZE = 4;
char keys[R_SIZE][C_SIZE] = { 
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[R_SIZE] = {36,34,32,30}; //black pins on keyboard
byte colPins[C_SIZE] = {37,35,33,31};
Keypad kp = Keypad(makeKeymap(keys), rowPins, colPins, R_SIZE, C_SIZE);


/**
 * Variables for PIR
 */
int pirPin = 2;
int pirState = LOW;
int val = 0;
int calibrationDelay = 30;
boolean isCalibrate = false;

/**
 * Variable for RFID reader
 */
RFID rfid(SS_PIN, RST_PIN); 
int UID[5];

void setup() {
  Serial.begin(9600);
  pinMode(buzzer,OUTPUT);
  pinMode(pirPin, INPUT);
  initializeSD();
  setPassword("1234");
  SPI.begin();
  rfid.init();

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  postRequest();
}

void postRequest(){
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
  } 
  else {
    Serial.println("Connection failed !");
  }  
}

void loop() {
  //if(!isCalibrate){
  //  pirCalibration();
  //}
  analyze();
  //readKeyboard();
  //checkRFID();
}

void checkRFID(){
  if (rfid.isCard()) {  
    if (rfid.readCardSerial()) {        
      Serial.print("L'UID est: ");
      for(int i=0;i<=4;i++){
        UID[i]=rfid.serNum[i];
        Serial.print(UID[i],DEC);
        Serial.print(".");
      }
      Serial.println("");
    }          
    rfid.halt();
  }
  delay(100);
}

void analyze(){
  val = digitalRead(pirPin);
  Serial.println(val);
  if(val == HIGH){
      delay(300);
      if(pirState == LOW){
        Serial.println("Motion detected !");
        pirState = HIGH;
        tone(buzzer,1000);
        delay(300);
      }
   }
   else{
    delay(300);
    if(pirState == HIGH){
      Serial.println("Motion ended !");
      pirState =LOW;
      noTone(buzzer);
      delay(300);
    }
  } 
}

void pirCalibration(){
  Serial.println("Pir Sensor Calibration !");
  for(int i = 0; i < calibrationDelay; i++){
    Serial.println(i);
    delay(1000);
  }
  isCalibrate = true;
}

void readKeyboard(){
  char key = kp.getKey();
  if(key != NO_KEY && isDigit(key)){
    Serial.println(key);
    attempt += key;
  }
}

void initializeSD(){
  /* we check if the communication is OK with the pin 4 which is the pin used by
  the shield for the SD card port*/
  if(!SD.begin(4)){
    // if it's not OK, we stop the program
    Serial.println("Communication is NOK !");
    return;
  }
  Serial.println("Communication is OK !");
}

String getPassword(){
}

void setPassword(String password){
  if(password.length() > 6){
    Serial.println("The password is too long !");
    return;
  }
  else if(password.length() < 4){
    Serial.println("The password is too short !");
    return;
  }
  else if(!isValidPassword(password)){
    Serial.println("The password must contains only numbers !");
    return;
  }
  
  //if the file "pf.txt" exists, il will be erased
  if(SD.exists("pf.txt")){
     SD.remove("pf.txt");
     Serial.println("The password file is erased !");
  }
  
  //create the file "pf.txt"
  passwordFile = SD.open("pf.txt",FILE_WRITE);
  passwordFile.print(password);
  passwordFile.close();
  Serial.println("The password is defined !");  
}

boolean isValidPassword(String password){
  for(byte i = 0; i < password.length(); i++){
    if(!isDigit(password.charAt(i))){
      return false;
    }
  }
  return true;
}
