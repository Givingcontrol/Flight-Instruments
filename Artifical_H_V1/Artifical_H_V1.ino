/* V0
 *  ported to ST7789 TFT - modify user setup in TFT_eSPI
 *  does not fill whole screen - come back to !!!!!!!!!!!
 *  
 * V1 
 *  remove most mavlink code
 *  add MPU6050
 *  add MPUlight Libary to read pitch, roll, yaw
 *  resize display to fit 240 x 240 = should be autofitted! 
 *    compiles and loads
 *    
 *  
 *  MAVLInk_DroneLights
 *  by Juan Pedro López
 *  
 * This program was developed to connect an Arduino board with a Pixhawk via MAVLink 
 *   with the objective of controlling a group of WS2812B LED lights on board of a quad
 */


// Demo code for artifical horizon display
// Written by Bodmer for a 160 x 128 TFT display
// 15/8/16


// Code templates joined by vierfuffzig feb 2021
// https://github.com/vierfuffzig/MAVLinkEFIS
//************************************************
//V1
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <MPU6050_light.h>
//ESP32:: SCL = GPIO22 SDA= GPIO21
Adafruit_MPU6050 mpu;
MPU6050 mpu_light(Wire);


//MPU variables

int16_t pitch, roll, yaw; //from mpu6050light
int16_t Pitch, Roll, Yaw; //from mpu6050light

// Mavlink carry over variables
int16_t heading = 0;
float alt = 0;
//float volt = 0;
float curr = 0;
int16_t rssi = 0;
//float climb = 0;
float aspd = 0;
int16_t mode = 0;

static float volt = 12.3;
static char voltstr[15];

static float climb = 12.3;
static char climbstr[15];

// String modestr = "MODE";
char modestr[15] = "MODE";






////////////////////////////////////////// TFT Setup ///////////////////////////////////////////////////////////

#include <SPI.h>

// Use ONE of these three highly optimised libraries, comment out other two!

// For S6D02A1 based TFT displays
//#include <TFT_S6D02A1.h>         // Bodmer's graphics and font library for S6D02A1 driver chip
//TFT_S6D02A1 tft = TFT_S6D02A1(); // Invoke library, pins defined in User_Setup.h
//                                    https://github.com/Bodmer/TFT_S6D02A1

// For ST7735 based TFT displays
#include <TFT_eSPI.h>
//#include <..\TFT_eSPI\User_Setups\Setup205_ST7789_ESP32.h>
TFT_eSPI tft = TFT_eSPI();
//#include <TFT_ST7735.h>          // Bodmer's graphics and font library for ST7735 driver chip
//TFT_ST7735 tft = TFT_ST7735();   // Invoke library, pins defined in User_Setup.h
//                                    https://github.com/Bodmer/TFT_ST7735

// For ILI9341 based TFT displays (note sketch is currently setup for a 160 x 128 display)
//#include <TFT_ILI9341.h>         // Bodmer's graphics and font library for ILI9341 driver chip
//TFT_ILI9341 tft = TFT_ILI9341(); // Invoke library, pins defined in User_Setup.h
//                                    https://github.com/Bodmer/TFT_ILI9341

#define REDRAW_DELAY 16 // minimum delay in milliseconds between display updates

#define HOR 120    // Horizon circle outside radius (205 is corner to corner

#define BROWN      0x5140 //0x5960
#define SKY_BLUE   0x02B5 //0x0318 //0x039B //0x34BF
#define DARK_RED   0x8000
#define DARK_GREY  0x39C7

#define XC 120 // x coord of centre of horizon
#define YC 120 // y coord of centre of horizon

#define ANGLE_INC 1 // Angle increment for arc segments, 1 will give finer resolution, 2 or more gives faster rotation

#define DEG2RAD 0.0174532925

//int roll = 3;
//int pitch;

int roll_angle = 180; // These must be initialed to 180 so updateHorizon(0); in setup() draws

int last_roll = 0; // the whole horizon graphic
int last_pitch = 0;

int roll_delta = 90;  // This is used to set arc drawing direction, must be set to 90 here


// Variables for test only
//int test_angle = 0;
//int delta = ANGLE_INC;

unsigned long redrawTime = 0;



////////////////////////////////////////////// end of tft setup ////////////////////////////////////////////////////////


void setup() {
  
  Serial.begin(115200);


Wire.begin();
   mpu_light.begin();
   
   
///////////////// TFT setup ///////////////////////////////////////////////////

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);            // Centre middle justified
  tft.drawString("Gyro Offsets", 120, 120, 1);
  mpu_light.calcGyroOffsets(); 
   delay(2000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  
//Original
 //tft.fillRect(0,  0, 128, 80, SKY_BLUE); 
 // tft.fillRect(0, 80, 128, 80, BROWN);

//240 x 240 display
  tft.fillRect(0,  0, 240, 120, SKY_BLUE);
  tft.fillRect(0, 120, 240, 120, BROWN);

  // Draw the horizon graphic
  drawHorizon(0, 0);
  drawInfo();
  delay(2000); // Wait to permit visual check

  // Test roll and pitch
  // testRoll();
  // testPitch();

  
}

void loop() {
  
  unsigned long currentMillis = millis();
  int i=0;

  


  

////////////////////////////////////// TFT display //////////////////////////

  if (millis() > redrawTime) {
    get_MPU_Angle_Readings();
    redrawTime = millis() + REDRAW_DELAY;
    updateHorizon(roll, pitch);

  }
   
}

//////////////////////////////////////MPU functions
void get_MPU_Angle_Readings(){
//mpu6050light  
   mpu_light.update();
  Pitch =(mpu_light.getAngleX());
  pitch = Pitch;
  alt = Pitch;  // getting values to display
  roll =(mpu_light.getAngleY());
  climb = roll;
  Yaw = (mpu_light.getAngleZ());

}

////////////////////////////////////// telemetry functions /////////////////////////////////////////////////////////////////////////


// #########################################################################
// Update the horizon with a new angle (angle in range -180 to +180)
// #########################################################################
void updateHorizon(int roll, int pitch)
{
  bool draw = 1;
  int delta_pitch = 0;
  int pitch_error = 0;
  int delta_roll  = 0;
  while ((last_pitch != pitch) || (last_roll != roll))
  {
    delta_pitch = 0;
    delta_roll  = 0;

    if (last_pitch < pitch) {
      delta_pitch = 1;
      pitch_error = pitch - last_pitch;
    }
    
    if (last_pitch > pitch) {
      delta_pitch = -1;
      pitch_error = last_pitch - pitch;
    }
    
    if (last_roll < roll) delta_roll  = 1;
    if (last_roll > roll) delta_roll  = -1;
    
    if (delta_roll == 0) {
      if (pitch_error > 1) delta_pitch *= 2;
    }
    
    drawHorizon(last_roll + delta_roll, last_pitch + delta_pitch);
    drawInfo();
  }
}

void drawHorizon(int roll, int pitch)
{
  //roll = uav_roll;
  // Fudge factor adjustment for this sketch (so horizon is horizontal when start angle is 0)
  // This is needed as we draw extra pixels to avoid leaving plotting artifacts

  // Calculate coordinates for line start
  float sx = cos(roll * DEG2RAD);
  float sy = sin(roll * DEG2RAD);

  int16_t x0 = sx * HOR;
  int16_t y0 = sy * HOR;

  if ((roll != last_roll) && ((abs(roll) > 35)  || (pitch != last_pitch)))
  {
    tft.drawLine(XC - x0, YC - y0 - 3 + pitch, XC + x0, YC + y0 - 3 + pitch, SKY_BLUE);
    tft.drawLine(XC - x0, YC - y0 + 3 + pitch, XC + x0, YC + y0 + 3 + pitch, BROWN);
    tft.drawLine(XC - x0, YC - y0 - 4 + pitch, XC + x0, YC + y0 - 4 + pitch, SKY_BLUE);
    tft.drawLine(XC - x0, YC - y0 + 4 + pitch, XC + x0, YC + y0 + 4 + pitch, BROWN);
  }

  tft.drawLine(XC - x0, YC - y0 - 2 + pitch, XC + x0, YC + y0 - 2 + pitch, SKY_BLUE);
  tft.drawLine(XC - x0, YC - y0 + 2 + pitch, XC + x0, YC + y0 + 2 + pitch, BROWN);

  tft.drawLine(XC - x0, YC - y0 - 1 + pitch, XC + x0, YC + y0 - 1 + pitch, SKY_BLUE);
  tft.drawLine(XC - x0, YC - y0 + 1 + pitch, XC + x0, YC + y0 + 1 + pitch, BROWN);

  tft.drawLine(XC - x0, YC - y0 + pitch,   XC + x0, YC + y0 + pitch,   TFT_WHITE);

  last_roll = roll;
  last_pitch = pitch;

}


// #########################################################################
// Draw the information
// #########################################################################

void drawInfo(void)
{
  // Update things near middle of screen first (most likely to get obscured)

  // Level wings graphic
  tft.fillRect(120 - 1, 120 - 1, 3, 3, TFT_RED);
  tft.drawFastHLine(120 - 30,   120, 24, TFT_RED);
  tft.drawFastHLine(120 + 30 - 24, 120, 24, TFT_RED);
  tft.drawFastVLine(120 - 30 + 24, 120, 3, TFT_RED);
  tft.drawFastVLine(120 + 30 - 24, 120, 3, TFT_RED);

  tft.drawFastHLine(120 - 12,   120 - 40, 24, TFT_WHITE);
  tft.drawFastHLine(120 -  6,   120 - 30, 12, TFT_WHITE);
  tft.drawFastHLine(120 - 60,   120 - 20, 120, TFT_WHITE);
  tft.drawFastHLine(120 -  6,   120 - 10, 12, TFT_WHITE);

  tft.drawFastHLine(120 -  6,   120 + 10, 12, TFT_WHITE);
  tft.drawFastHLine(120 - 12,   120 + 20, 24, TFT_WHITE);
  tft.drawFastHLine(120 -  6,   120 + 30, 12, TFT_WHITE);
  tft.drawFastHLine(120 - 12,   120 + 40, 24, TFT_WHITE);


  tft.setTextColor(TFT_WHITE);
  tft.setCursor(120 - 60 - 20, 120 - 20 - 3);
  tft.print("10");
  tft.setCursor(120 + 60 + 1, 120 - 20 - 3);
  tft.print("10");
  tft.setCursor(120 - 12 - 13, 120 + 20 - 3);
  tft.print("10");
  tft.setCursor(120 + 12 + 1, 120 + 20 - 3);
  tft.print("10");

  tft.setCursor(120 - 12 - 13, 120 - 40 - 3);
  tft.print("20");
  tft.setCursor(120 + 12 + 1, 120 - 40 - 3);
  tft.print("20");
  tft.setCursor(120 - 12 - 13, 120 + 40 - 3);
  tft.print("20");
  tft.setCursor(120 + 12 + 1, 120 + 40 - 3);
  tft.print("20");


/*****flight data*******
 * 
 *   int16_t heading = 0;
 *   float alt = 0;
 *   float volt = 0;
 *   float curr = 0;
 *   int16_t rssi = 0;
 *   float climb = 0;
 *   float aspd = 0;
 *   int16_t mode = 0;
 */

  // Display flight data
  // Top Data
  tft.setTextColor(TFT_YELLOW, SKY_BLUE);
  tft.setTextDatum(TC_DATUM);
  
  tft.setTextPadding(88); // Flightmode requires full line padding
  tft.drawString(modestr, 120, 1, 1);
  
  tft.setTextPadding(24); // reset to value only padding
  tft.drawNumber(alt, 129, 20, 1);
 // tft.drawString(climbstr, 74, 19, 1);
  //tft.drawString("12.3", 74, 19, 1);
   tft.drawNumber(climb, 129, 40, 1);

  // Bottom data
  tft.setTextColor(TFT_YELLOW, BROWN); // Text with background
  tft.setTextDatum(MC_DATUM);            // Centre middle justified
  tft.setTextPadding(24);                // Padding width to wipe previous number
 
  tft.drawNumber(heading, 120, 135, 1);
  tft.drawNumber(rssi, 120, 144, 1);
  tft.drawString(voltstr, 128, 153, 1);
  //tft.drawNumber(curr, 74, 151, 1);
  //tft.drawNumber(alt, 64, 151, 1);

  // Draw fixed text
  tft.setTextColor(TFT_YELLOW);
  tft.setTextDatum(TC_DATUM);            // Centre middle justified
  
  tft.drawString("Alt", 80, 10, 1);
  tft.drawString("m", 160, 10, 1);
  tft.drawString("Climb", 80, 19, 1);
  tft.drawString("m/s", 160, 19, 1);
  tft.drawString("Hdg", 80, 180, 1);
  tft.drawString("deg", 120, 180, 1);
  tft.drawString("Rssi", 80, 200, 1);
  tft.drawString("%", 160, 200, 1);
  tft.drawString("Bat", 80, 220, 1);
  tft.drawString("V", 160, 220, 1);


}
