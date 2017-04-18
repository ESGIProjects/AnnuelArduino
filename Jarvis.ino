#include <SPI.h>
#include <SD.h>
#include <ctype.h>

File passwordFile;

void setup() {
  Serial.begin(9600);
  initializeSD();
  setPassword("1234");
}

void loop() {

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

