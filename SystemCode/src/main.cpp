#include <main.h>

#define FIREBASE_HOST "readtemphumi-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "qhMBENlso11IhSldZAvefbmN5gqqW5moVpqigphX"

// #define WIFI_SSID "Lính Thuỷ Đánh Bạc"
// #define WIFI_PASSWORD "07091995"

#define WIFI_SSID "Tang 3.1"
#define WIFI_PASSWORD "14220auco"
// #define WIFI_SSID "tang 3.2"
// #define WIFI_PASSWORD "giangthang789"

#define I2C_SDA 21
#define I2C_SCL 22
#define DHTPIN 15
#define DHTTYPE DHT11
#define relayLight 5
#define relayFan 18
#define relayMist 16
#define relayHeating 4
#define relayPump 17
#define soilSsPin 39
#define PIN_12V 34
#define PIN_5V 36
#define PIN_SOLAR 35
#define PIN_VSOLAR 32
#define PIN_AUTOMATIC 25
#define PIN_MANUAL 26

// DHT11
DHT dht(DHTPIN, DHTTYPE);

FirebaseData firebaseData;

// Ban Phim Ma Tran
const byte ROWS = 4;  // four rows
const byte COLS = 4;  // four columns
// define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {12, 14, 27, 26};  // connect to the row pinouts of the keypad
byte colPins[COLS] = {25, 33, 32, 19};  // connect to the column pinouts of the keypad

// initialize an instance of class NewKeypad
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);

// BH1750
BH1750 lightMeter;
// Lọc Nhiễu
SimpleKalmanFilter filter(2, 2, 0.01);

typedef enum {
    AUTOMATIC = 0,
    HANDWORK = 1,
    DEFAUT = 2
} ModeActive;

struct ParamSystem {
    int amountLight;
    int percentSM;
    float temperature;
    float humidity;
    float currentOfLoad;
    float powerOfLoad;
    float currentOfSolar;
    float powerOfSolar;
};
String fan = "";
String light = "";
String mist = "";
String heating = "";
String pump = "";
String modeAtv = "";
String resetlcd = "";
String flagChangeStateRl = "";
unsigned long timeWaitRead = 0;
ModeActive modeActive = DEFAUT;
ParamSystem param;
char key;
bool resetLCDFlag = false;
bool turnOffRelayFlag = false;
bool flagAutomatic = false;
bool flagGetParam = false;
bool checkModeAtm = false;  // Biến kiểm tra câu lệnh đã được thực hiện hay chưa
bool checkModeHw = false;  // Biến kiểm tra câu lệnh đã được thực hiện hay chưa
char keyHoldTurnOff;
float R1 = 30000.00;
float R2 = 7500.00;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // lcd
    Wire.begin(I2C_SDA, I2C_SCL);  // Initialize I2C interface
    lcd.init();                    // Initialize the LCD screen
    lcd.backlight();               // Turn on backlight

    // ConnectWifi
    initWiFi();

    // Connect Firebase
    // Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

    connectFirebase();

    // DHT
    dht.begin();

    // BH1750
    lightMeter.begin();

    pinMode(relayLight, OUTPUT);
    pinMode(relayFan, OUTPUT);
    pinMode(relayMist, OUTPUT);
    pinMode(relayHeating, OUTPUT);
    pinMode(relayPump, OUTPUT);
    pinMode(soilSsPin, INPUT);
    pinMode(PIN_12V, INPUT);
    pinMode(PIN_5V, INPUT);
    pinMode(PIN_VSOLAR, INPUT);
    pinMode(PIN_AUTOMATIC,OUTPUT);
    pinMode(PIN_MANUAL, OUTPUT);
    setData("/ControlDevice/LIGHT", "OFF");
    setData("/ControlDevice/FAN", "OFF");
    setData("/ControlDevice/MIST", "OFF");
    setData("/ControlDevice/HEATING", "OFF");
    setData("/ControlDevice/PUMP", "OFF");
    setData("/AModeActive", "DEFAUT");
    setData("/ResetLCD", "NO");
    setData("/ChangeStateRelay", "YES");
    timeWaitRead = (unsigned long)millis();
    initDisplay();
}

void loop() {
    // check WiFi Connection
    if (WiFi.status() != WL_CONNECTED) {
        reconnectWiFi();
    }
    unsigned long currentMillis = millis();
    if (currentMillis - timeWaitRead >= 10000) {
        flagAutomatic = true;
        flagGetParam = true;
        timeWaitRead = currentMillis;
    }
    if (flagGetParam == true) {
        getParamSensors();
        flagGetParam = false;
    }
    selectModeActive();
    resetlcd = getData("/ResetLCD");
    if (resetLCDFlag == true || resetlcd == "YES") {
        Serial.print("Reset LCD");
        resetLCD();
        resetLCDFlag = false;
        setData("/ResetLCD", "NO");
    }
}

void initWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // try to connect with wifi
    Serial.print("Connecting to ");
    Serial.print(WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        for (int i = 0; i < 5; i++) {
            lcd.setCursor(i, 0);
            lcd.print(".");
            if (i == 4) {
                reconnectWiFi();
            }
        }
        delay(500);
    }
    lcd.clear();
    Serial.println();
    Serial.print("Connected to ");
    Serial.println(WIFI_SSID);
    Serial.print("IP Address is : ");
    Serial.println(WiFi.localIP());
}
void connectFirebase() {
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    while (!Firebase.ready()) {
        Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
        Serial.print("Connecting With Firebase...!!\n");
    }
    Serial.print("Connected With Firebase...!!\n");
}
void reconnectWiFi() {
    // Loop until we're reconnected
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("Connected to AP");
    }
}

float* readTempHumi() {
    float* result = (float*)malloc(2 * sizeof(float));
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

        // srand(time(0));
        // float randomNumber = (round((0.05 + (float)rand() / ((float)RAND_MAX / (0.1 - 0.05))) * 100)) / 100;
        result[0] = humidity;
        result[1] = temperature;
    }
    return result;
}

int readSoilMoisture() {
    int value = 0;
    int realValue = 0;
    for (int i = 0; i <= 1000; i++) {
        value = +analogRead(soilSsPin);
    }
    realValue = value / 1000;
    int percent = map(value, 0, 4095, 0, 100);
    int realPercentage = 100 - percent;
    Serial.printf("Do Am Dat: %d%%\n", realPercentage);
    return realPercentage;
}

long readLightIntensity() {
    long lux = (long)lightMeter.readLightLevel();
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");
    return lux;
}

void getParamSensors() {
    float* tempHumi = readTempHumi();
    param.humidity = tempHumi[0];
    param.temperature = tempHumi[1];
    param.percentSM = readSoilMoisture();
    param.amountLight = readLightIntensity();
    // updateCurrentPower();
    // readVoltage(PIN_VSOLAR);
    float* currentPowerLoad12V = readCurrent(PIN_12V);
    float* currentPowerLoad5V = readCurrent(PIN_5V);
    float* currentPowerSolar = readCurrent(PIN_SOLAR);
    param.currentOfLoad = ((float)((int)((currentPowerLoad12V[0] + currentPowerLoad5V[0]) * 100 + 0.5))) / 100.0;
    param.powerOfLoad = ((float)((int)((currentPowerLoad12V[1] + currentPowerLoad5V[1]) * 100 + 0.5))) / 100.0;
    param.currentOfSolar = ((int)(currentPowerSolar[0] * 100 + 0.5)) / 100.0;
    param.powerOfSolar = ((int)(currentPowerSolar[1] * 100 + 0.5)) / 100.0;
    // printf("currentOfLoad: %f\t\npowerOfLoad %f\t\ncurrentOfSolar%f\t\npowerOfSolar%f\t\n", param.currentOfLoad, param.powerOfLoad, param.currentOfSolar, param.powerOfSolar);

    Firebase.setFloat(firebaseData, "/Sensor/DHT11/Humidity", param.humidity);
    Firebase.setFloat(firebaseData, "/Sensor/DHT11/Temperature", param.temperature);
    Firebase.setFloat(firebaseData, "/Sensor/LightIntensity", param.amountLight);
    Firebase.setInt(firebaseData, "/Sensor/SoilMoisture", param.percentSM);
    Firebase.setFloat(firebaseData, "/Energy/Load/Current", param.currentOfLoad);
    Firebase.setFloat(firebaseData, "/Energy/Load/Power", param.powerOfLoad);
    Firebase.setFloat(firebaseData, "/Energy/Solar/Current", param.currentOfSolar);
    Firebase.setFloat(firebaseData, "/Energy/Solar/Power", param.powerOfSolar);
}
// set data
void setData(const char* field, const char* value) {
    if (Firebase.setString(firebaseData, field, value)) {
        Serial.println("Set field success");
    } else {
        Serial.println("Set field failure");
    }
}
// get Data
String getData(const char* field) {
    Firebase.getString(firebaseData, field);
    return firebaseData.stringData();
}

// control relay
void controlRelay(String device, int pin, String name) {
    if (device == "ON ") {  // compare the input of led status received from firebase
        Serial.print(name);
        Serial.println(" turned ON");
        digitalWrite(pin, HIGH);  // make output ON
    }

    else if (device == "OFF") {  // compare the input of led status received from firebase
        Serial.print(name);
        Serial.println(" turned OFF");
        digitalWrite(pin, LOW);  // make output OFF
    } else {
        Serial.println("Wrong Credential! Please send ON/OFF");
    }
}

void controlDevice(int pin, bool statusRelay, const char* device) {
    digitalWrite(pin, statusRelay);
    if (digitalRead(pin) == HIGH) {
        setData(device, "ON ");
    } else {
        setData(device, "OFF");
    }
}

void handleModeAutomatic() {
    // if (param.temperature >= 30 || param.humidity > 70) {
    //     controlDevice(relayFan, HIGH, "/ControlDevice/FAN");
    //     controlDevice(relayHeating, LOW, "/ControlDevice/HEATING");

    // } else if (param.temperature <= 20) {
    //     controlDevice(relayHeating, HIGH, "/ControlDevice/HEATING");
    //     controlDevice(relayFan, LOW, "/ControlDevice/FAN");
    // } else {
    //     controlDevice(relayFan, LOW, "/ControlDevice/FAN");
    //     controlDevice(relayHeating, LOW, "/ControlDevice/HEATING");
    // }
    // if (param.humidity >= 50 && param.humidity <= 70) {
    //     controlDevice(relayMist, LOW, "/ControlDevice/MIST");

    // } else if (param.humidity < 50) {
    //     controlDevice(relayMist, HIGH, "/ControlDevice/MIST");
    // }
    // if (param.percentSM >= 45 && param.percentSM <= 100) {
    //     controlDevice(relayPump, LOW, "/ControlDevice/PUMP");

    // } else {
    //     controlDevice(relayPump, HIGH, "/ControlDevice/PUMP");
    // }
    // if (param.amountLight < 2000) {
    //     controlDevice(relayLight, HIGH, "/ControlDevice/LIGHT");
    // } else if (param.amountLight >= 2000 && param.amountLight <= 3000) {
    //     controlDevice(relayLight, LOW, "/ControlDevice/LIGHT");
    // } else {
    //     controlDevice(relayLight, LOW, "/ControlDevice/LIGHT");
    // }
    // fan = getData("/ControlDevice/FAN");
    // light = getData("/ControlDevice/LIGHT");
    // mist = getData("/ControlDevice/MIST");
    // heating = getData("/ControlDevice/HEATING");
    // pump = getData("/ControlDevice/PUMP");
    bool fanState;
    bool lightState;
    bool mistState;
    bool pumpState;
    bool heatingState;
    if (param.temperature >= 30 || param.humidity > 70) {
    fanState = HIGH;
    heatingState = LOW;
} else if (param.temperature <= 20) {
    heatingState = HIGH;
    fanState = LOW;
} else {
    fanState = LOW;
    heatingState = LOW;
}

if (param.humidity >= 50 && param.humidity <= 70) {
    mistState = LOW;
} else if (param.humidity < 50) {
    mistState = HIGH;
}

if (param.percentSM >= 45 && param.percentSM <= 100) {
    pumpState = LOW;
} else {
    pumpState = HIGH;
}

if (param.amountLight < 2000) {
    lightState = HIGH;
} else if (param.amountLight >= 2000 && param.amountLight <= 3000) {
    lightState = LOW;
} else {
    lightState = LOW;
}

controlDevice(relayFan, fanState, "/ControlDevice/FAN");
controlDevice(relayHeating, heatingState, "/ControlDevice/HEATING");
controlDevice(relayMist, mistState, "/ControlDevice/MIST");
controlDevice(relayPump, pumpState, "/ControlDevice/PUMP");
controlDevice(relayLight, lightState, "/ControlDevice/LIGHT");

// Lấy trạng thái của các thiết bị từ hệ thống
fan = getData("/ControlDevice/FAN");
light = getData("/ControlDevice/LIGHT");
mist = getData("/ControlDevice/MIST");
heating = getData("/ControlDevice/HEATING");
pump = getData("/ControlDevice/PUMP");
}

void runModeAutomatic() {
    if (flagAutomatic) {
        handleModeAutomatic();
        flagAutomatic = false;
    }
    displayParamSystem();
}

void runModeHandwork() {
    flagChangeStateRl = getData("/ChangeStateRelay");
    fan = getData("/ControlDevice/FAN");
    light = getData("/ControlDevice/LIGHT");
    mist = getData("/ControlDevice/MIST");
    heating = getData("/ControlDevice/HEATING");
    pump = getData("/ControlDevice/PUMP");
    if (flagChangeStateRl == "YES") {
        controlRelay(fan, relayFan, "QUAT");
        controlRelay(light, relayLight, "DEN");
        controlRelay(mist, relayMist, "PS");
        controlRelay(heating, relayHeating, "DENSuoi");
        controlRelay(pump, relayPump, "BOM");
        setData("/ChangeStateRelay", "NO");
    }
    displayParamSystem();
}

void turnOnRelayHandwork() {
    if (key == '1') {
        setData("/ControlDevice/LIGHT", "ON ");
    } else if (key == '2') {
        setData("/ControlDevice/PUMP", "ON ");
    } else if (key == '3') {
        setData("/ControlDevice/FAN", "ON ");
        //    Serial.print("PhunSuong ON\n");
    } else if (key == '4') {
        setData("/ControlDevice/MIST", "ON ");
        //    Serial.print("Rem ON\n");
    } else if (key == '5') {
        setData("/ControlDevice/CURTAIN", "ON ");
    }
}

bool turnOffRelayHandwork() {
    //  if (turnOffRelayFlag) {
    if (key == '6') {
        setData("/  ControlDevice/LIGHT", "OFF");
    } else if (key == '7') {
        setData("/ControlDevice/PUMP", "OFF");
    } else if (key == '8') {
        setData("/ControlDevice/FAN", "OFF");
        //    Serial.print("PhunSuong OFF\n");
    } else if (key == '9') {
        setData("/ControlDevice/MIST", "OFF");
    }
    turnOffRelayFlag = false;
    return true;
    //  }
}
void keypadEvent(KeypadEvent key) {
    switch (keypad.getState()) {
    case PRESSED:
        //      lcd.clear();
        if (key == 'A') {
            lcd.clear();
            modeActive = AUTOMATIC;
            setData("AModeActive", "AUTOMATIC");
        } else if (key == 'B') {
            lcd.clear();
            modeActive = HANDWORK;
            setData("AModeActive", "HANDWORK");
        }
        break;
    case HOLD:
        //      lcd.clear();
        if (key == '*') {
            resetLCDFlag = true;
        }
        //      else if (key == '1' || key == '2' || key == '3' || key == '4') {
        //        keyHoldTurnOff = key;
        //        turnOffRelayFlag = true;
        //      }
        break;
    }
}

void selectModeActive() {
    modeAtv = getData("/AModeActive");
    if (modeAtv == "AUTOMATIC") {
        if (!checkModeAtm ) {
            checkModeAtm = true;
            checkModeHw = false;
            digitalWrite(PIN_AUTOMATIC, HIGH);
            digitalWrite(PIN_MANUAL, LOW);
            Serial.printf("TC: %d\nTD:%d\n", digitalRead(PIN_MANUAL),digitalRead(PIN_AUTOMATIC));
        }
        runModeAutomatic();
    } else if (modeAtv == "HANDWORK") {
        if (!checkModeHw) {
            checkModeHw = true;
            checkModeAtm = false;
            digitalWrite(PIN_AUTOMATIC, LOW);
            digitalWrite(PIN_MANUAL, HIGH);
            Serial.printf("TC: %d\nTD:%d\n", digitalRead(PIN_MANUAL),digitalRead(PIN_AUTOMATIC));
        }
        runModeHandwork();
    } else {
        //    initDisplay();
        Serial.print("Please select mode !!!\n");
    }
}

void resetLCD() {
    lcd.begin(20, 4);     // Cài đặt lại độ phân giải và số dòng
    lcd.clear();          // Xóa toàn bộ màn hình
    lcd.setCursor(0, 0);  // Đặt con trỏ về vị trí đầu tiên trên màn hình
                          //  lcd.print("LCD RESET"); // In ra thông báo LCD đã được reset
                          //  lcd.clear(); // Xóa toàn bộ màn hình
}

void initDisplay() {
    lcd.setCursor(4, 0);
    lcd.print("Chon Che Do");
    lcd.setCursor(0, 1);
    lcd.print("Nhan: TU DONG");
    lcd.setCursor(0, 2);
    lcd.print("Nhan: THU CONG");
}
void displayParamSystem() {
    char buff[5] = {0};
    if (param.amountLight < 10) {
        sprintf(buff, "   %d", param.amountLight);
    } else if (param.amountLight < 100) {
        sprintf(buff, "  %d", param.amountLight);
    } else if (param.amountLight < 1000) {
        sprintf(buff, " %d", param.amountLight);
    } else if (param.amountLight < 10000) {
        sprintf(buff, "%d", param.amountLight);
        Serial.print(buff);
    }
    // NhietDo
    lcd.setCursor(1, 0);
    lcd.print("T");
    lcd.setCursor(0, 1);
    lcd.print((int)param.temperature);
    lcd.setCursor(2, 1);
    lcd.print("C");
    // Do Am Khong Khi
    lcd.setCursor(5, 0);
    lcd.print("H");
    lcd.setCursor(4, 1);
    lcd.print((int)param.humidity);
    lcd.setCursor(6, 1);
    lcd.print("%");
    // Anh sang
    lcd.setCursor(10, 0);
    lcd.print("AS");
    lcd.setCursor(8, 1);
    lcd.print(buff); 
    lcd.setCursor(12, 1);
    lcd.print("lx");
    // Do Am Dat
    lcd.setCursor(15, 0);
    lcd.print("DD");
    lcd.setCursor(15, 1);
    lcd.print(param.percentSM);
    lcd.setCursor(18, 1);
    lcd.print("%");
    // Den
    lcd.setCursor(0, 2);
    lcd.print("DEN");
    lcd.setCursor(0, 3);
    lcd.print(light);
    // Bom
    lcd.setCursor(4, 2);
    lcd.print("BOM");
    lcd.setCursor(4, 3);
    lcd.print(pump);
    // Quat
    lcd.setCursor(8, 2);
    lcd.print("QUAT");
    lcd.setCursor(8, 3);
    lcd.print(fan);
    // Phun Suong
    lcd.setCursor(13, 2);
    lcd.print("PS");
    lcd.setCursor(13, 3);
    lcd.print(mist);
    // Rem
    lcd.setCursor(17, 2);
    lcd.print("DS");
    lcd.setCursor(17, 3);
    lcd.print(heating);
    // }
}

float* readCurrent(float pin) {
    float* result = (float*)malloc(2 * sizeof(float));
    float sum = 0;
    float current = 0, power = 0;
    float sumavg = 0;
    if (pin == PIN_12V) {
        for (int i = 0; i < 100; i++) {
            float raw = analogRead(pin);
            float valueFiltered = filter.updateEstimate(raw);
            float value = (raw * 5.00 / 4095.0) * 0.707;  // 0.70 0.707
            sum += value;
        }
        sumavg = sum / 100;
        current = (sumavg - 5.00 * 0.5) / 0.066;  // 0.066
        if (current < 0.1) {
            current = 0;
        }
        power = 12.00 * current;
    } else if (pin == PIN_SOLAR) {
        for (int i = 0; i < 100; i++) {
            float raw = analogRead(pin);
            // float valueFiltered = filter.updateEstimate(raw);
            float value = (raw * 5.00 / 4095.0) * 0.715;  // 0.95; 0.83  0.805 0.827 0.687 0.703
            sum += value;
        }
        sumavg = sum / 100;
        current = (sumavg - 5.00 * 0.5) / 0.185;  // 0185 0.066
        if (current < 0) {
            current = 0;
        }
        power = 18.00 * current;
    } else if (pin == PIN_5V) {
        for (int i = 0; i < 100; i++) {
            float raw = analogRead(pin);
            float valueFiltered = filter.updateEstimate(raw);
            float value = (raw * 5.00 / 4095.0) * 0.705;  //  0.70 0.71
            sum += value;
        }
        sumavg = sum / 100;
        current = (sumavg - 5.00 * 0.5) / 0.066;  // 0.066
        if (current < 0) {
            current = 0;
        }
        power = 5.00 * current;
    }
    Serial.println("\n\t ------------------------------");
    Serial.printf("Voltage: %0.2f\n", sumavg);
    Serial.printf("Current: %0.2f\n", current);
    Serial.printf("Power: %0.2f\n", power);

    result[0] = current;
    result[1] = power;
    return result;
}
float readVoltage(float pin) {
    float Vin = 0;
    float sumVolt = 0;
    for (int i = 0; i < 300; i++) {
        int adc = analogRead(pin);
        // float vout = (adc * 5.00) / 4095;
        // sumVolt = sumVolt + vout;
        sumVolt = sumVolt + adc;
    }
    float sum = sumVolt / 300;
    // float sumVoltAvg = sumVolt / 300;
    // Vin = sumVoltAvg / (R2 / (R1 + R2));
    float voltage = mapp(sum, 0, 4095, 0.00, 24.00);
    Serial.printf("adc = %0.2f\n", sum);
    Serial.printf("Vin = %0.2f\n", voltage);
    return Vin;
}

float mapp(long x, long in_min, long in_max, long out_min, long out_max) {
    const long run = in_max - in_min;
    if (run == 0) {
        log_e("map(): Invalid input range, min == max");
        return -1;  // AVR returns -1, SAM returns 0
    }
    const long rise = out_max - out_min;
    const long delta = x - in_min;
    float result = ((float)(delta * rise) / run + out_min) * 0.7;
    return result;
}