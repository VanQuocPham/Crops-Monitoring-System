#include <string>
#include <ArduinoJson.h>
#include <BH1750.h>
#include <DHT.h>
#include <FirebaseESP32.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <SimpleKalmanFilter.h>
#include <WiFi.h>
#include <Wire.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
bool turnOffRelayHandwork();
void initWiFi();
void setData(const char* field, const char* value);
// void keypadEvent(KeypadEvent key);
void resetLCD();
void selectModeActive();
void reconnectWiFi();
void displayParamSystem();
void turnOnRelayHandwork();
void initDisplay();
String getData(const char* field);
float* readCurrent(float pin);
void getParamSensors();
void updateCurrentPower();
float readVoltage(float pin);
float mapp(long x, long in_min, long in_max, long out_min, long out_max);
void connectFirebase();