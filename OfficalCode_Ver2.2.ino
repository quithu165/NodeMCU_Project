#include <Wire.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <ESP_EEPROM.h>
#include <SoftwareSerial.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>


//---------------------------------------------------------------------------------------LibraryInclude

#define DHTPIN D3     // what digital pin we're connected to
#define SS_PIN D4  //D2
#define RST_PIN D8 //D1

//---------------------------------------------------------------------------------------PinSetup
#define DHTTYPE DHT11   // DHT 11
SoftwareSerial sim(D1, D2);
LiquidCrystal_I2C lcd(0x27,16,2);
DHT dht(DHTPIN, DHTTYPE);
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key; 
int statuss = 0;
int out = 0;

const char *ssid = "MH370"; 
const char *password = "hoanglong12345"; 
const char *mqtt_server = "postman.cloudmqtt.com"; 
const char *device_id = "PJ";
void callback(char *test, byte *payload, unsigned int length);
WiFiClient espClient;
PubSubClient mqtt(mqtt_server, 16018, callback, espClient);

//---------------------------------------------------------------------------------------LibraryDeclareVariables
#define delayShowdata 2000
#define delayReadbutton 50
#define delayCheck 3000
//---------------------------------------------------------------------------------------ConstantVariable
float temp;
float humi;
bool checkExit = false;
bool checkShowdata = true;
bool checkReadbutton = true;
bool checkCheck = true;
bool Master;
bool checkWifi = true;
bool checkServer = true;
int state = 0;
int enterSignal = 0;
int prevButtonSelect = 0;
int prevButtonEnter = 0;
bool flag = false;
bool flag_RFID = true;
float buttonValue;
float timeReadbutton = 0;
float timeShowdata = 0;
float timeCheck = 0;

bool S0 = true;
bool S1 = true;
bool S2 = true;
bool S3 = true;
bool S4 = true;
bool S5 = true;
bool S6 = true;
bool S7 = true;
byte eepromVar1[8];
byte ReadCard[4];
//---------------------------------------------------------------------------------------GlobalVariableDeclare
void callback(char *test, byte *payload, unsigned int length)
{
  String message = String((char*)payload);
  message.remove(length);
}

void setup_wifi() {
 
  delay(10);
  checkWifi = true;
  WiFi.begin(ssid, password);
  float inTime = millis();
  float runTime;
  lcd.clear();
  lcd.print("Wifi connecting.");
  while (WiFi.status() != WL_CONNECTED) {
    runTime = millis();
    delay(500);
    if (runTime - inTime > 5000) {
      lcd.clear();
      lcd.print("Unvalid Wifi");
      delay(1000);
      checkWifi = false;
      break;
    }
  }
  randomSeed(micros());
  lcd.clear();
  lcd.print("Wifi connected");
  delay(1000);
}

void connect_mqtt()
{
  checkServer = true;
  float inTime = millis();
  float runTime;
  lcd.clear();
  lcd.print("SV connecting...");
  while (1)
  { 
    runTime = millis();
    delay (500);
    if (!mqtt.connect("PJ","ujgyghbn","uv9MUCnOHSgA"))
    {
      if (runTime - inTime > 5000) {
        checkServer = false;
        lcd.clear();
        lcd.print("Invalid Server");
        delay(1000);
        lcd.clear();
        return; 
      }
    }
    else break;
  }
  lcd.clear();
  lcd.print("Server connected");
  delay(1000);
  lcd.clear();

  
}
void lcd_setup(){
  Wire.begin(D10,D9);
  lcd.begin();
  lcd.home();
  lcd.backlight();
 
  
}
void lcd_print(int a, int b, String text){
  lcd.setCursor(a,b);
  lcd.print(text);
}

void lcd_S0(float &delayTime){
  float curTime = millis();
  if (curTime - delayTime >= delayCheck || checkCheck)
  {
    
     if ( ! mfrc522.PICC_IsNewCardPresent()) 
     {
        lcd.setCursor(0,0);
        lcd.print("Insert Card");
        return;
     }
     if ( ! mfrc522.PICC_ReadCardSerial()) 
      {
        return;
      }
     String content= "";
     byte letter;
     for (byte i = 0; i < mfrc522.uid.size; i++) 
     {
        content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
     }
      content.toUpperCase();
     for(int i = 0; i <4; i++) {
        ReadCard[i] = mfrc522.uid.uidByte[i];
     }
 
     if (content.substring(1) == "03 DD 7D 83" || CheckID(ReadCard) ) //change UID of the card that you want to give access
     {
       if (!CheckID(ReadCard)) {
        Master = true; 
       }
       else {
        Master = false;
       }
       statuss = 1;
       lcd.setCursor(0,0);
       lcd.print("Access Granted");
       delay(1000);
       lcd.clear();
       lcd.setCursor(5,0);
       lcd.print("Welcome!");
       delay(1000);
     }
  
     else   {
        lcd.setCursor(0,0);
        lcd.print("Access Denied");
        delay(3000);
        
     }
    delayTime = curTime;
    checkCheck = false;
  }
}
void lcd_S1(){
  lcd.clear();
  lcd_print(0,0,">1. Show data");
  lcd_print(1,1,"2. Send message");
}
void lcd_S2(){
  lcd.clear();
  lcd_print(1,0,"1. Show data");
  lcd_print(0,1,">2. Send message");
}
void lcd_S3(){
  lcd.clear();
  lcd_print(1,0,"2. Send message");
  lcd_print(0,1,">3. Add Card");
}
void lcd_S4(){
  lcd.clear();
  lcd_print(1,0,"3. Add Card");
  lcd_print(0,1,">4. Delete Card");
}
void lcd_S5() {
  lcd.clear();
  lcd_print(1,0,"4. Delete Card");
  lcd_print(0,1,">5. Clear Users");
}
void lcd_S6(){
  lcd.clear();
  lcd_print(1,0,"5. Clear Users");
  lcd_print(0,1,">6. Update Data ");
}
void lcd_S7(){
  lcd.clear();
  lcd_print(1,0,"6. Update Data");
  lcd_print(0,1,">7. Log Out ");
}
void showData(float &delayTime,float &curTime){
  
  if (curTime - delayTime >= delayShowdata || checkShowdata){
   temp = dht.readTemperature();
   humi = dht.readHumidity();
   if (isnan(humi) || isnan(temp) ) {
    lcd.clear();
    lcd.print("Fail to read");
  }
   lcd.clear();
   lcd_print(0,0,"Temp: ");
   lcd.setCursor(6,0);
   lcd.print(temp);
   lcd_print(0,1,"Humi: ");
   lcd.setCursor(6,1);
   lcd.print(humi);
   checkShowdata = false;
   delayTime = curTime;
  
  }
}
void sendSMS(){
   lcd.clear();
   lcd.print("Sending...");
   temp = dht.readTemperature();
   humi = dht.readHumidity();
   sim.print("AT+CMGF=1\r\n");
   delay(100);
   sim.print("AT+CMGS=\"0845730083\"\r\n");
   delay(100);
   if (isnan(humi) || isnan(temp))
    {
       sim.print("Fail to read from DHT sensor\r\n");
       delay(100);
       sim.write(26);
       delay(100);
    }
    
       String t = String(temp);
       String h = String(humi);
       sim.print(t+"\r\n"+h+"\r\n");
       delay(100);
       sim.write(26);
       delay(100);
    
    delay(4000);
    enterSignal = 0;
    S2 = true;
    state = 2;
    
}
void ClearUsers() {
  lcd.clear();
  for (int i = 0; i < 20; i++) {
    EEPROM.put(i, 0);
    EEPROM.commit();
  }
 
 lcd.print("All users delete");
 delay(500);
 enterSignal = 0;
 S3 = true;
}
void add() {
  lcd.clear(); 
  if ( ! mfrc522.PICC_IsNewCardPresent())
      return;

  if ( ! mfrc522.PICC_ReadCardSerial())
    {
      return;
    }
  for(int i = 0; i <4; i++) {
    ReadCard[i] = mfrc522.uid.uidByte[i];
  }

  if ((EEPROM.get(8,eepromVar1[0]) == 0 
    && EEPROM.get(9,eepromVar1[1]) == 0 
    && EEPROM.get(10,eepromVar1[2]) == 0 
    && EEPROM.get(11,eepromVar1[3]) == 0 ) &&
  (ReadCard[0] != EEPROM.get(12,eepromVar1[4]) 
    && ReadCard[1] != EEPROM.get(13,eepromVar1[5]) 
    && ReadCard[2] != EEPROM.get(14,eepromVar1[6]) 
    && ReadCard[3] != EEPROM.get(15,eepromVar1[7])) ) 
  { 
      for (byte i = 0; i < 4; i++) {
        EEPROM.put(i+8,ReadCard[i]) ;
        EEPROM.commit();
      }
      
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Add Successfully");
      enterSignal = 0;
      delay(500);
      S3 = true;
    }
  
  else if ( (EEPROM.get(12,eepromVar1[4]) == 0 
    && EEPROM.get(13,eepromVar1[5]) == 0 
    && EEPROM.get(14,eepromVar1[6]) == 0 
    && EEPROM.get(15,eepromVar1[7]) == 0) 
  && (ReadCard[0] != EEPROM.get(8,eepromVar1[0]) 
  && ReadCard[1] != EEPROM.get(9,eepromVar1[1]) 
  && ReadCard[2] != EEPROM.get(10,eepromVar1[2]) 
  && ReadCard[3] != EEPROM.get(11,eepromVar1[3]))) 
  {
      for (byte i = 0; i < 4; i++) {
        EEPROM.put(i+12,ReadCard[i]) ;
        EEPROM.commit();
      }
      for (byte i = 0; i < 4; i++) {
        Serial.print(EEPROM.get(i+12,eepromVar1[i]));
      }  
      
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Add Successfully");
      enterSignal = 0;
      delay(500);
      S3 = true;
  }
  else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Adding Error");
      enterSignal = 0;
      delay(500);
      S3 = true;
  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void del() {
  lcd.clear();
 if ( ! mfrc522.PICC_IsNewCardPresent())
    {
      return;
    }

  if ( ! mfrc522.PICC_ReadCardSerial())
    {
      return;
    }
    for(int i = 0; i <4; i++) {
      ReadCard[i] = mfrc522.uid.uidByte[i]; 
    }
  
   
    if (ReadCard[0] == EEPROM.get(8, eepromVar1[0]) 
      && ReadCard[1] == EEPROM.get(9, eepromVar1[1]) 
      && ReadCard[2] == EEPROM.get(10, eepromVar1[2]) 
      && ReadCard[3] == EEPROM.get(11, eepromVar1[3]) ) {
      for (int i = 0; i < 4; i++) {
        EEPROM.put(i + 8, EEPROM.get(i+12, eepromVar1[i])) ;
        EEPROM.commit();
      }
      for (int i = 0; i < 4; i++) {
        EEPROM.put(i + 12,0) ;
        EEPROM.commit();
      }
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("User DELETED");
      enterSignal = 0;
      delay(500);
      S4 = true;
    }
    else if (ReadCard[0] == EEPROM.get(12, eepromVar1[4]) 
    && ReadCard[1] == EEPROM.get(13, eepromVar1[5]) 
    && ReadCard[2] == EEPROM.get(14, eepromVar1[6]) 
    && ReadCard[3] == EEPROM.get(15, eepromVar1[7]) ) 
    {
      for (int i = 0; i < 4; i++) {
        EEPROM.put(i+12, 0) ;
        EEPROM.commit();      
      }
        for (byte i = 0; i < 4; i++) {
      }  
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("User DELETED");
      enterSignal = 0;
      delay(500);
      S4 = true;
    }
  
  else {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("INVALID DELETING");
    enterSignal = 0;
    delay(500);
    S4 = true;
  }

  mfrc5222.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

bool CheckID(byte a[]) {
  if ((a[0] == EEPROM.get(8,eepromVar1[0]) && a[1] == EEPROM.get(9,eepromVar1[1]) && a[2] == EEPROM.get(10,eepromVar1[2]) && a[3] == EEPROM.get(11,eepromVar1[3]) ) 
  || (a[0] == EEPROM.get(12, eepromVar1[4]) && a[1] == EEPROM.get(13, eepromVar1[5]) && a[2] == EEPROM.get(14, eepromVar1[6]) && a[3] == EEPROM.get(15, eepromVar1[7])))
    return true;
  else return false;
}
void updateData(){
  enterSignal = 0;
  if (!checkWifi){
    lcd.clear();
    lcd.print("Unvalid Wifi");
    delay(2000);
    setup_wifi();
    if (!checkWifi){
      lcd.clear();
      lcd.print("Unable connect");
      delay(2000);
      S6 = true;
      return;
    }
  }
  if (!checkServer){
      if (!checkWifi) 
          return;
      connect_mqtt();
      if (checkServer) {
      lcd.clear();
      lcd.print("Unconnect Server");
      delay(2000);
      }
      S6 = true;
      return;
    }
     
   temp = dht.readTemperature();
   humi = dht.readHumidity();
   if (isnan(humi) || isnan(temp))
    {
       temp = 00.00;
       humi = 00.00;
    }
    
  char h[5];
  char t[5];
  int tmp = temp * 100;
  for (int i =0 ; i<5; i++ )
    {h[i] = tmp%10 + 48;
     tmp = (tmp - tmp%10)/10;
    }
  mqtt.publish("Humi", h);
  tmp = humi * 100;
  for (int i =0 ; i<5; i++ )
    {t[i] = tmp%10 + 48;
     tmp = (tmp - tmp%10)/10;
    }
  mqtt.publish("Temp", t);
   delay(10);
  lcd.clear();
  lcd.print("Uploaded");
  delay(1000);
  S6 = true;
  enterSignal = 0;
}
void logOut(){
  lcd.clear();
  state = 0;
  enterSignal = 0;
  statuss = 0;
  S0 = true;
}
void readControlSignal(float &delayTime){
  float curTime = millis();
  if (curTime - delayTime >= delayReadbutton || checkReadbutton)
  {
    buttonValue = analogRead(A0);
    if ((buttonValue > 650 && buttonValue < 750) && prevButtonSelect == 0)
    {
      state = (state%8) + 1;
      if (state == 8) state = 1;
      prevButtonSelect = 1;
    }
    if ((buttonValue < 650 || buttonValue > 750) && prevButtonSelect == 1)
      prevButtonSelect = 0;
    if ((buttonValue > 250 && buttonValue < 450) && prevButtonEnter == 0)
    {
      enterSignal = 1 - enterSignal;
      prevButtonEnter = 1;
    }
    if ((buttonValue < 250 || buttonValue > 450) && prevButtonEnter == 1)
      prevButtonEnter = 0;
    delayTime = curTime;
    checkReadbutton = false;
    Serial.print(state);
    Serial.print(" ");
    Serial.println(enterSignal);
    
  }
}
void rfid_setup(){
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init(); 
}
void mqtt_setup(){
   setup_wifi();
   connect_mqtt();
}
void setup() {
  Serial.begin(9600);
  EEPROM.begin(1024);
  sim.begin(9600);
  pinMode(A0, OUTPUT);
  rfid_setup();
  lcd_setup();
  mqtt_setup();
}


void loop() {
  
  readControlSignal(timeReadbutton);
  //timeCheck = millis();
  if (S0) lcd_S0(timeCheck);
  if (statuss == 0){
   // enterSignal = 0;
    state = 0;
  }
  else if (statuss == 1){
    if (flag_RFID) {
      flag_RFID = false;
      state = 1;
     // enterSignal = 0;
    }
    S0 = false;
  }
  switch(state){
    case 1: 
      if (S1){
        lcd.clear();
        lcd_S1();
        S1 = false;
        S2 = true;
        S3 = true;
        S4 = true;
        S5 = true;
        S6 = true;
        S7 = true;
        flag = false;
      }
      if (enterSignal == 1)
      {
        if (!flag) lcd.clear();
        float curTime = millis();
        showData(timeShowdata, curTime);
        flag = true;
      }
    
      break;
     case 2:
      if (S2){
        lcd_S2();
        S2 = false;
        S1 = true;
        S3 = true;
        S4 = true;
        S5 = true;
        S6 = true;
        S7 = true;
      }
      if (enterSignal == 1) sendSMS();
      break;
     case 3:
      if (S3){
        lcd_S3();
        S3 = false;
        S2 = true;
        S1 = true;
        S4 = true;
        S5 = true;
        S6 = true;
        S7 = true;
        flag = false;
      }
       if (enterSignal == 1)
      {
        if (Master) {
        lcd.clear();
        lcd.print("Insert card");
        delay(200);
        add();
        }
        else {
          lcd.clear();
          lcd.print("MSCard required");
          delay(2000);
          enterSignal = 0;
          S3 = true;
        }
      }
      break;
     case 4:
      if (S4){
        lcd_S4();
        S4 = false;
        S2 = true;
        S3 = true;
        S1 = true;
        S5 = true;
        S6 = true;
        S7 = true;
        
      }
       if (enterSignal == 1)
      {
        if (Master) {
        lcd.clear();
        lcd.print("Insert card");
        delay(200);
        del();
        }
        else {
          lcd.clear();
          lcd.print("MSCard required");
          delay(2000);
          enterSignal = 0;
          S4 = true;
        }
      }
      break;
     case 5:
      if (S5){
        lcd_S5();
        S5 = false;
        S2 = true;
        S3 = true;
        S1 = true;
        S4 = true;
        S6 = true;
        S7 = true;
        
      }
       if (enterSignal == 1)
      {
        if (Master) {
        ClearUsers();
        }
        else {
          lcd.clear();
          lcd.print("MSCard required");
          delay(2000);
          enterSignal = 0;
          S5 = true;
        }
      }
      break; 
      case 6:
      if (S6){
        lcd_S6();
        S6 = false;
        S2 = true;
        S3 = true;
        S4 = true;
        S1 = true;
        S5 = true;
        S7 = true;
      }
      if (enterSignal == 1) updateData();
      break;
     case 7:
      if (S7){
        lcd_S7();
        S7 = false;
        S2 = true;
        S3 = true;
        S4 = true;
        S1 = true;
        S5 = true;
        S6 = true;
      }
      if (enterSignal == 1) logOut();
      break;
     default: break;
  }
}
