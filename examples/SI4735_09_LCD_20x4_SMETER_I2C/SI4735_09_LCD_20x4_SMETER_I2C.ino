/*
  SS4735 Arduino Library example with LCD 20x4 I2C.
  It is a example that shows how to set up a LCD and SI4735 on the same I2C bus.
  This sketch uses the Rotary Encoder Class implementation from Ben Buxton (the source code is included
  together with this sketch) and LiquidCrystal I2C Library by Frank de Brabander.
  Look for LiquidCrystal I2C on Manager Libraries.

  By Ricardo Lima Caratti, Nov 2019.
*/

#include <SI4735.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>    // https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SH1106.h> // https://github.com/wonho-maker/Adafruit_SH1106
#include "Rotary.h"
#include "smeter.h"

#define OLED_RESET 4

#define AM_FUNCTION 1
#define FM_FUNCTION 0

#define RESET_PIN 12

// Enconder PINs
#define ENCODER_PIN_A 3
#define ENCODER_PIN_B 2

// Buttons controllers
#define AM_FM_BUTTON 5 // Next Band
#define SEEK_BUTTON 6  // Previous Band
#define VOL_UP 7       // Volume Volume Up
#define VOL_DOWN 8     // Volume Down
#define SEEK 9         // Seek Function

#define MIN_ELAPSED_TIME 100

long elapsedButton = millis();
long elapsedRSSI = millis();

// Encoder control variables
volatile int encoderCount = 0;

// Some variables to check the SI4735 status
unsigned currentFrequency;
unsigned previousFrequency;
byte rssi = 0;
byte stereo = 1;
byte volume = 0;

// Devices class declarations
Rotary encoder = Rotary(ENCODER_PIN_A, ENCODER_PIN_B);
LiquidCrystal_I2C lcd(0x27, 20, 4);
Adafruit_SH1106 display(OLED_RESET);
SI4735 si4735;

void setup()
{
  // Encoder pins
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  pinMode(AM_FM_BUTTON, INPUT_PULLUP);
  pinMode(SEEK_BUTTON, INPUT_PULLUP);
  pinMode(VOL_UP, INPUT_PULLUP);
  pinMode(VOL_DOWN, INPUT_PULLUP);


  display.begin(SH1106_SWITCHCAPVCC, 0x3C);                // needed for SH1106 display
  display.clearDisplay();                                  // clears display from any library info displayed
  display.display();

  lcd.init();

  lcd.backlight();
  lcd.setCursor(6, 0);
  lcd.print("SI4735");
  lcd.setCursor(2, 1);
  lcd.print("Arduino Library");
  lcd.setCursor(0, 3);
  lcd.print("By PU2CLR - Ricardo");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("FM and AM (MW & SW)");
  lcd.setCursor(4, 2);
  lcd.print("Tuning Test");

  lcd.clear();

  // Encoder interrupt
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);

  delay(500);

  si4735.setup(RESET_PIN, FM_FUNCTION);

  // Starts defaul radio function and band (FM; from 84 to 108 MHz; 103.9 MHz; step 100KHz)
  si4735.setFM(8400, 10800, 10390, 10);

  delay(500);

  currentFrequency = previousFrequency = si4735.getFrequency();
  si4735.setVolume(45);
  showStatus();
}

// Use Rotary.h and  Rotary.cpp implementation to process encoder via interrupt
void rotaryEncoder()
{ // rotary encoder events
  uint8_t encoderStatus = encoder.process();
  if (encoderStatus)
  {
    if (encoderStatus == DIR_CW)
    {
      encoderCount = 1;
    }
    else
    {
      encoderCount = -1;
    }
  }
}

// Show current frequency
void showStatus()
{
  // Clear just the frequency information space.
  lcd.setCursor(7, 0);
  lcd.print("        ");

  lcd.setCursor(0, 0);
  if (si4735.isCurrentTuneFM())
  {
    lcd.print("FM ");
    lcd.setCursor(7, 0);
    lcd.print(String(currentFrequency / 100.0, 2));
    lcd.setCursor(17, 0);
    lcd.print("MHz ");
    showStereo();
  }
  else
  {
    lcd.print("AM ");
    lcd.setCursor(8, 0);
    lcd.print(currentFrequency);
    lcd.setCursor(17, 0);
    lcd.print("KHz");
    lcd.setCursor(0, 3);
    lcd.print("       ");
  }
}

/* *******************************
   Shows RSSI status
*/
void showRSSI()
{
  int blk;
  lcd.setCursor(8, 3);
  lcd.print("S:");

  blk = rssi / 10 + 1;

  for (int i = 0; i < 10; i++)
  {
    lcd.setCursor(10 + i, 3);
    lcd.print((i < blk) ? ">" : " ");
  }
}

/* *****************************
   Shows Stereo or Mono status
*/
void showStereo()
{

  lcd.setCursor(0, 3);
  lcd.print((si4735.getCurrentPilot()) ? "STEREO" : "MONO  ");
}

/* ***************************
   Shows the volume level on LCD
*/
void showVolume()
{
  lcd.setCursor(8, 2);
  lcd.print("VOLUME    ");
  lcd.setCursor(15, 2);
  lcd.print(volume);
}

  
void showSmeter(unsigned signalLevel)
{
  static unsigned long startMillis = millis(); // start of signalLevel window
  static unsigned int PeaktoPeak = 0;          // peak-to-peak level
  static unsigned int maxSignal = 0;
  static unsigned int minSignal = 2014;
  static int hMeter = 65; // horizontal center for needle animation
  static int vMeter = 85; // vertical center for needle animation (outside of dislay limits)
  static int rMeter = 80;
  const int sampleWindow = 50; // sample window width in mS (50 mS = 20Hz)

  // PeaktoPeak = maxSignal - minSignal;         // max - min = peak-peak amplitude
  PeaktoPeak = signalLevel;
  float MeterValue = PeaktoPeak * 330 / 1024; // convert volts to arrow information

  MeterValue = MeterValue - 34;                            // shifts needle to zero position
  display.clearDisplay();                                  // refresh display for next step
  display.drawBitmap(0, 0, S_Meter, 128, 64, WHITE);       // draws background
  int a1 = (hMeter + (sin(MeterValue / 57.296) * rMeter)); // meter needle horizontal coordinate
  int a2 = (vMeter - (cos(MeterValue / 57.296) * rMeter)); // meter needle vertical coordinate
  display.drawLine(a1, a2, hMeter, vMeter, WHITE);         // draws needle
  display.display();
  delay(10);
}


/*
   Main
*/
void loop()
{

  // Check if the encoder has moved.
  if (encoderCount != 0)
  {

    if (encoderCount == 1)
      si4735.frequencyUp();
    else
      si4735.frequencyDown();

    encoderCount = 0;
    showSmeter(0);
  }

  // Check button commands
  if ((millis() - elapsedButton) > MIN_ELAPSED_TIME)
  {
    // check if some button is pressed
    if (digitalRead(AM_FM_BUTTON) == LOW)
    {
      // Switch AM to FM and vice-versa
      if (si4735.isCurrentTuneFM())
        si4735.setAM(570, 1710, 810, 10);
      else
        si4735.setFM(8600, 10800, 10390, 10);
    }
    else if (digitalRead(SEEK_BUTTON) == LOW)
      si4735.seekStationUp();
    else if (digitalRead(VOL_UP) == LOW)
      si4735.volumeUp();
    else if (digitalRead(VOL_DOWN) == LOW)
      si4735.volumeDown();

    elapsedButton = millis();
  }

  // Show the current frequency only if it has changed
  currentFrequency = si4735.getFrequency();
  if (currentFrequency != previousFrequency)
  {
    previousFrequency = currentFrequency;
    showStatus();
  }

  // Show RSSI status only if this condition has changed
  if ( (millis() - elapsedRSSI) > MIN_ELAPSED_TIME * 2) {
    si4735.getCurrentReceivedSignalQuality();
    if (rssi != si4735.getCurrentRSSI())
    {
      rssi = si4735.getCurrentRSSI();
      // showRSSI();
      showSmeter(rssi * 10);
    }
    elapsedRSSI = millis();
  }

  // Show stereo status only if this condition has changed
  if (si4735.isCurrentTuneFM())
  {
    // Show stereo status only if this condition has changed
    if (stereo != si4735.getCurrentPilot())
    {
      stereo = si4735.getCurrentPilot();
      showStereo();
    }
  }

  // Show volume level only if this condition has changed
  if (si4735.getCurrentVolume() != volume)
  {
    volume = si4735.getCurrentVolume();
    showVolume();
  }

  delay(50);
}