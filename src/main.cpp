#include <Wire.h>
#include <Arduino.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <BleMouse.h>

#define I2C_SDA 14
#define I2C_SCL 15
#define L_BTN_PIN 12
#define R_BTN_PIN 13

uint8_t data[6];
int16_t gyroX, gyroZ;

// if I increase sensitivity, the mouse will move slower 
int SensitivityX = 600;
int SensitivityY = 400;
int delayi = 20;

int16_t calibrationGyroX = 0;
int16_t calibrationGyroZ = 0;
bool isCalibrated = false;

BleMouse bleMouse;

uint32_t timer;
uint8_t i2cData[14];

uint32_t leftButtonPressTime = 0;
bool leftButtonHeld = false;
bool bothButtonsPressed = false;

const uint8_t IMUAddress = 0x68;
const uint16_t I2C_TIMEOUT = 1000;

uint8_t i2cWrite(uint8_t registerAddress, uint8_t *data, uint8_t length, bool sendStop)
{
 Wire.beginTransmission(IMUAddress);
 Wire.write(registerAddress);
 Wire.write(data, length);
 return Wire.endTransmission(sendStop); // Returns 0 on success
}

uint8_t i2cWrite2(uint8_t registerAddress, uint8_t data, bool sendStop)
{
 return i2cWrite(registerAddress, &data, 1, sendStop); // Returns 0 on success
}

uint8_t i2cRead(uint8_t registerAddress, uint8_t *data, uint8_t nbytes)
{
 uint32_t timeOutTimer;
 Wire.beginTransmission(IMUAddress);
 Wire.write(registerAddress);
 if (Wire.endTransmission(false))
    return 1;
 Wire.requestFrom(IMUAddress, nbytes, (uint8_t) true);
 for (uint8_t i = 0; i < nbytes; i++)
 {
    if (Wire.available())
      data[i] = Wire.read();
    else
    {
      timeOutTimer = micros();
      while (((micros() - timeOutTimer) < I2C_TIMEOUT) && !Wire.available())
        ;
      if (Wire.available())
        data[i] = Wire.read();
      else
        return 2;
    }
 }
 return 0;
}

void setup()
{
 Wire.begin(I2C_SDA, I2C_SCL);
 pinMode(L_BTN_PIN, INPUT);
 pinMode(R_BTN_PIN, INPUT);

 i2cData[0] = 7;
 i2cData[1] = 0x00;
 i2cData[3] = 0x00;

 while (i2cWrite(0x19, i2cData, 4, false))
    ;
 while (i2cWrite2(0x6B, 0x01, true))
    ;
 while (i2cRead(0x75, i2cData, 1))
    ;
 while (i2cRead(0x3B, i2cData, 6))
    ;

 timer = micros();
 Serial.begin(115200);
 bleMouse.begin();
 delay(100);
}

void loop()
{
 while (i2cRead(0x3B, i2cData, 14));
 gyroX = ((i2cData[8] << 8) | i2cData[9]);
 gyroZ = ((i2cData[12] << 8) | i2cData[13]);
  Serial.print("X(raw): ");
  Serial.print(gyroX);
  Serial.print(" Y(raw): ");
  Serial.print(gyroZ);
  Serial.print("\r\n");


 gyroX = gyroX / SensitivityX / 1.1 * -1;
 gyroZ = gyroZ / SensitivityY * -1;

 if (bleMouse.isConnected())
 {
    // Gestione del tasto sinistro
    if (digitalRead(L_BTN_PIN) == HIGH && bothButtonsPressed == false)
    {
      if (!leftButtonHeld)
      {
        leftButtonPressTime = micros();
        leftButtonHeld = true;
        bleMouse.press(MOUSE_LEFT); // Invia il comando di pressione
      }
    }
    else if (leftButtonHeld)
    {
      if (micros() - leftButtonPressTime > 200000)
      { // 0.2 secondi in microsecondi
        Serial.println("Left click long press");
        bleMouse.press(MOUSE_LEFT);
      }
      else
      {
        Serial.println("Left click");
      }
      bleMouse.release(MOUSE_LEFT); // Invia il comando di rilascio
      leftButtonHeld = false;
    }

    // Gestione del tasto destro
    if (digitalRead(R_BTN_PIN) == HIGH)
    {
      if (leftButtonHeld)
      { // Assicurati che il tasto sinistro non sia premuto
        bothButtonsPressed = true;
      }
    }
    else
    {
      bothButtonsPressed = false;
    }

    if (bothButtonsPressed)
    {
      // Esegui l'azione di scorrimento in base alla direzione del giroscopio
      if (gyroZ < 0)
      {
        Serial.println("Scroll up");
        bleMouse.move(0, 0, 1);
      }
      else if (gyroZ > 0)
      {
        Serial.println("Scroll down");
        bleMouse.move(0, 0, -1);
      }
      // Aggiungi qui altre direzioni se necessario
    }

    if (digitalRead(R_BTN_PIN) == HIGH && bothButtonsPressed == false)
    {
      delay(150);
      Serial.println("Right click");
      bleMouse.click(MOUSE_RIGHT);
    }

    Serial.print("X(elaborated): ");
    Serial.print(gyroX);
    Serial.print("Y(elaborated): ");
    Serial.print(gyroZ);
    Serial.print("\r\n");
    if (bothButtonsPressed == false)
      bleMouse.move(gyroZ, -gyroX);
    delay(delayi);
  }
}
