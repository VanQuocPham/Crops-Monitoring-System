#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include<DHT.h>
#include<Keypad.h>
#include<stdio.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define FIREBASE_HOST "readtemphumi-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "qhMBENlso11IhSldZAvefbmN5gqqW5moVpqigphX"

#define WIFI_SSID "Khuong"
#define WIFI_PASSWORD "0704887188"

#define DHTPIN 15
#define DHTTYPE DHT11
#define relayLight 2
#define relayFan 0
#define soilSsPin 34

DHT dht(DHTPIN, DHTTYPE);

FirebaseData firebaseData ;

//Ban Phim Ma Tran
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {12, 14, 27, 26}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {25, 33, 32, 19}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad keypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);
typedef enum {
  AUTOMATIC = 0,
  HANDWORK = 1,
  DEFAUT = 2
} ModeActive;

String fan = "";
String light = "";
unsigned long timeWaitRead = 0;
ModeActive modeActive = DEFAUT;
char key ;
bool resetLCDFlag = false;
bool turnOffRelayFlag = false;
char keyHoldTurnOff ;
void setup() {
  Serial.begin(9600);
  delay(1000);

  //DHT
  dht.begin();

  //lcd
  Wire.begin(21, 22); // Initialize I2C interface
  lcd.init();   // Initialize the LCD screen
  lcd.backlight(); // Turn on backlight

  //ConnectWifi
  connectWiFi();

  //Connect Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  timeWaitRead = millis();
  Serial.print("timeWaitRead: ");
  Serial.print(timeWaitRead);
  Serial.print("\n");
  pinMode(relayLight, OUTPUT);
  pinMode(relayFan, OUTPUT);
  pinMode(soilSsPin, INPUT);

  setData("LIGHT", "OFF");
  setData("FAN", "OFF");
  keypad.addEventListener(keypadEvent);
  lcd.setCursor(0, 0);
  lcd.print("Nhan A=AUTOMATIC");
  lcd.setCursor(0, 1);
  lcd.print("Nhan B=HANDWORK");
}

void loop() {
  key = keypad.getKey();
  selectModeActive();

  if (resetLCDFlag) {
    resetLCD();
    resetLCDFlag = false;
  }
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                                      //try to connect with wifi
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP Address is : ");
  Serial.println(WiFi.localIP());
}

float* readTempHumi() {
  static float result[2];
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read data from DHT sensor");
    result[0] = NAN;
    result[1] = NAN;
  } else {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%, Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");

    Firebase.setFloat(firebaseData, "/humidity", humidity);
    Firebase.setFloat(firebaseData, "/temperature", temperature);

    result[0] = humidity;
    result[1] = temperature;
  }
  return result;
}

int readSoilMoisture() {
  int value = 0;
  int realValue = 0;
  for (int i = 0; i <= 3000; i++) {
    value = + analogRead(soilSsPin);
  }
  realValue = value / 3000;
  int percent = map(value, 0, 4095, 0, 100);
  int realPercentage = 100 - percent;
  Serial.print("Do Am Dat: ");
  Serial.print(realPercentage);
  Serial.print(" %\n");
  return realPercentage;
}
// set data
void setData(const char* field, const char* value) {
  if (Firebase.setString(firebaseData, field, value)) {
    Serial.println("Set field success");
  } else {
    Serial.println("Set field failure");
  }
}
//get Data
String getData(const char*field) {
  Firebase.getString(firebaseData, field);
  return firebaseData.stringData();
}

// control relay
void controlRelay(String field, int pin, String name) {
  if (field == "ON ") {                         // compare the input of led status received from firebase
    Serial.print(name);
    Serial.println(" turned ON");
    digitalWrite(pin, HIGH);                                                         // make output ON
  }

  else if (field == "OFF") {              // compare the input of led status received from firebase
    Serial.print(name);
    Serial.println(" turned OFF");
    digitalWrite(pin, LOW);                                                         // make output OFF
  }
  else {
    Serial.println("Wrong Credential! Please send ON/OFF");
  }
}
void handleTempHumi() {
  float *tempHumi = readTempHumi();
  float humidity = tempHumi[0];
  float temperature = tempHumi[1];
  fan = getData("FAN");
  controlRelay(fan, relayFan, "QUAT" );

  light = getData("LIGHT");
  controlRelay(light, relayLight, "DEN" );
  if (temperature >= 35) {
    setData("FAN", "ON ");
  }
  else if ( temperature <= 10) {
    setData("LIGHT", "ON ");
  }
  else {
    setData("FAN", "OFF");
    setData("LIGHT", "OFF");
  }
}

void runModeAutomatic() {
  if ((unsigned long)millis() - timeWaitRead >= 5000) {
    int percentSoilCoisture = readSoilMoisture();
    handleTempHumi();
    timeWaitRead = millis();
  }
  lcd.setCursor(0, 0);
  lcd.print("QUAT: ");
  lcd.setCursor(5, 0);
  lcd.print(fan);
  lcd.setCursor(0, 1);
  lcd.print("DEN : ");
  lcd.setCursor(5, 1);
  lcd.print(light);
}

void runModeHandwork() {
  turnOnRelayHandwork();
  turnOffRelayHandwork();
  fan = getData("FAN");
  light = getData("LIGHT");
  controlRelay(fan, relayFan, "QUAT");
  controlRelay(light, relayLight, "DEN");
  //display LCD
  lcd.setCursor(0, 0);
  lcd.print("QUAT: ");
  lcd.setCursor(5, 0);
  lcd.print(fan);
  lcd.setCursor(0, 1);
  lcd.print("DEN : ");
  lcd.setCursor(5, 1);
  lcd.print(light);
}

void turnOnRelayHandwork() {
  char keyTurnOn = keypad.getKey();
  if (keyTurnOn == '1') {
    setData("FAN", "ON ");
  }
  else if (keyTurnOn == '2') {
    setData("LIGHT", "ON ");
  }
  else if (keyTurnOn == '3') {
    //    setData("....", "ON");
    Serial.print("PhunSuong ON\n");
  }
  else if (keyTurnOn == '4') {
    //    setData("...."."ON ");
    Serial.print("Rem ON\n");
  }
}

bool turnOffRelayHandwork() {
  if (turnOffRelayFlag) {
    if (keyHoldTurnOff == '1') {
      setData("FAN", "OFF");
    }
    else if (keyHoldTurnOff == '2') {
      setData("LIGHT", "OFF");
    }
    else if (keyHoldTurnOff == '3') {
      //    setData("....", "OFF");
      Serial.print("PhunSuong OFF\n");
    }
    else if (keyHoldTurnOff == '4') {
      //    setData("...."."OFF");
      Serial.print("Rem OFF\n");
    }
    keyHoldTurnOff = 'r';
    turnOffRelayFlag = false;
    return true;
  }
}
void keypadEvent(KeypadEvent key) {
  switch (keypad.getState()) {
    case PRESSED:
      lcd.clear();
      if (key == 'A') {
        modeActive = AUTOMATIC;
      }
      else if (key == 'B') {
        modeActive = HANDWORK;
      }
      break;
    case HOLD:
      lcd.clear();
      if (key == '0') {
        resetLCDFlag = true;
      }
      else if (key == '1' || key == '2' || key == '3' || key == '4') {
        keyHoldTurnOff = key;
        turnOffRelayFlag = true;
      }
  }
}

void selectModeActive() {
  if (modeActive == AUTOMATIC) {
    runModeAutomatic();
  }
  else if (modeActive == HANDWORK) {
    Serial.print("HANDWORK !!!\n");
    runModeHandwork();
  }
  else {
    Serial.print("\Please select mode !!!\n");
  }
}

void resetLCD() {
  lcd.begin(16, 2); // Cài đặt lại độ phân giải và số dòng
  lcd.clear(); // Xóa toàn bộ màn hình
  lcd.setCursor(0, 0); // Đặt con trỏ về vị trí đầu tiên trên màn hình
  lcd.print("LCD RESET"); // In ra thông báo LCD đã được reset
  lcd.clear(); // Xóa toàn bộ màn hình
}
void hello(){}
