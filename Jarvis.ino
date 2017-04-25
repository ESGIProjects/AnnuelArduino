#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>
#include <Keypad.h>
#include <ctype.h>

File passwordFile;
String password;
String attempt;

byte mac[] = {0xDF, 0xAD, 0xAE, 0xEF, 0xDE, 0xAD};
EthernetClient client;
char serverName[] = "https://jarvis-esgi.herokuapp.com";
int serverPort = 16898;
char pageName[] = "/alert";

const byte R_SIZE = 4;
const byte C_SIZE = 4;
char keys[R_SIZE][C_SIZE] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[R_SIZE] = {10,9,8,7};
byte colPins[C_SIZE] = {6,5,3,2};
Keypad kp = Keypad(makeKeymap(keys), rowPins, colPins, R_SIZE, C_SIZE);

void setup() {
  Serial.begin(9600);
  initializeSD();
  setPassword("1234");
}

void loop() {
  readKeyboard();
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

