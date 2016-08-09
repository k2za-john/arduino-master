/*********************************************************************
This is an example sketch for our Monochrome Nokia 5110 LCD Displays

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/338

These displays use SPI to communicate, 4 or 5 pins are required to
interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution

Note this effort by N6QW builds on the work of DuWayne KV4QB
The changes by N6QW include generating the frequency at 4X the operating frequency and then dividing the result by 4
adding selectable USB or LSB, a TUNE function and an S Meter. 12/31/2014.

1/30/15 Changed for LBS Project --no offsets but the Fout is at 4X --you must follow the Si5351 output with a /4 via 74AC74
The final LBS SSB Transceiver will use a 4.9152 MHz IF Filter. It isan easy transition to making this into a 17M transceiver

********************************************************************/



#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "Rotary.h"
#include <toneAC.h>
#define NOTE_B5  988

// Software SPI (slower updates, more flexible pin options):
// pin 8 - Serial clock out (SCLK)
// pin 7 - Serial data out (DIN)
// pin 6 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 5 - LCD reset (RST)
// note there are different pin configurations on some display modules


//Adafruit_PCD8544 display = Adafruit_PCD8544(8, 7, 6, 4, 5);


//test antenna analyzer baord
Adafruit_PCD8544 display = Adafruit_PCD8544(8, 7, 6, 4, 5);

#define ENCODER_B    2                      // Encoder pin A
#define ENCODER_A    3                     // Encoder pin B
#define ENCODER_BTN  A3


#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
const int Backlight = 9; // Analog output pin that the LED is attached to
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };
  
  
  
// code for si5351 clock generator module from Adafruit
// it is controlled via i2c buss on pins a4 and a5


#include "si5351.h"
#include "Wire.h"

  Si5351 si5351;

  long int frq;
int_fast32_t rx = 28800000L; // Now just direct conversion 4 X Later Starting frequency of VFO = 4X times the operating frequency + Offset at 4.9137 = 48460800L
int_fast32_t rx2=1; // variable to hold the updated frequency
int_fast32_t increment = 10; // starting VFO update increment in HZ.
int_fast32_t bfo = 0L; // Now 0 because direct conversion default offset 4.9152 MHz  - 1.5 KHz = 4913700 this will be used in the SSB version
String hertz = " 10";
byte ones,tens,hundreds,thousands,tenthousands,hundredthousands,millions ;  //Placeholders
int buttonstate = 0;
int buttonstate2 = 0;

const int tonepin = 10;
const int SW = A1; //selects upper or lower sideband
const int SW1 = A2; // provides the TUNE fucntion

int i = 0;
const int SENSOR = 0;  // Analog Pin A0 for the S Meter function
int val = 0;
int val1 = 0;

int buttonState = 0;
int lastButtonState = 0;

Rotary r = Rotary(2,3); // sets the pins the rotary encoder uses.  Must be interrupt pins.
void setup() {
  
     Serial.begin(115200);                    // connect to the serial port
   
     PCICR |= (1 << PCIE2);
     PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
     sei();
     display.begin();  // init display 
     
     pinMode(SW, INPUT);   // Selects either USB or LSB````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````[
     digitalWrite(SW,HIGH);
     pinMode(SW1, INPUT); //Tune
     digitalWrite(SW1,HIGH);
 
     pinMode(A3,INPUT); // Connect to a button that goes to GND on push
     pinMode(A1,INPUT); // IF sense **********************************************
   
     digitalWrite(A3,HIGH);
     digitalWrite(A2,HIGH);
     digitalWrite(A1,HIGH); // init done
     // set backlight level
     analogWrite(Backlight,90 ); 
     // you can change the contrast around to adapt the display
     // for the best viewing!
     display.setContrast(60);

     display.display(); // show splashscreen
     delay(200);
     display.clearDisplay();   // clears the screen and buffer
     
    
     // text display tests
     
     
      //  initialize the Si5351
  
      si5351.init(SI5351_CRYSTAL_LOAD_8PF);
      si5351.set_correction(230);
      // Set CLK0 to output 4 times 7.2 MHz with a fixed PLL frequency --later a divide by 4 ~ phase noise improvement
      si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
      
     si5351.set_freq(rx , SI5351_PLL_FIXED, SI5351_CLK0);

     
      si5351.set_freq(bfo, 0, SI5351_CLK2);
      
      si5351.drive_strength(SI5351_CLK0,SI5351_DRIVE_8MA); 
      si5351.drive_strength(SI5351_CLK2,SI5351_DRIVE_8MA);
  
     

} 


void setincrement(){
    if (increment == 10){increment = 100;  hertz = "100";}
    else if (increment == 100){increment = 1000; hertz="  1K";}
    else if (increment == 1000){increment = 10000; hertz=" 10K"; }
    else if (increment == 10000){increment = 100000; hertz="100K";}
    else if (increment == 100000){increment = 1000000; hertz="  1M";}  
    else{increment = 10; hertz = " 10";};  

     delay(125); // Adjust this delay to speed up/slow down the button menu scroll speed.
};
void showFreq(){
      millions = int(((rx/4)-bfo)/1000000);
      hundredthousands = ((((rx/4)-bfo )/100000)%10);
      tenthousands = ((((rx/4)-bfo )/10000)%10);
      thousands = ((((rx/4)-bfo )/1000)%10);
      hundreds = ((((rx/4)-bfo )/100)%10);
      tens = ((((rx/4)-bfo )/10)%10);
      ones = ((((rx/4)-bfo )/1)%10);
     // display.setCursor(0,0);
     // display.print("                ");
     display.setTextSize(2);
      if (millions > 9){display.setCursor(0,0);}
          else{display .setCursor(12,0);}
      display.print(millions);
     // display.print(".");
      display.print(hundredthousands);
      display.print(tenthousands);
      display.print(thousands);
       display.setTextSize(1);
     
     
      display.print(hundreds);
      display.print(tens);
      display.print(ones);

      display.display();
};
ISR(PCINT2_vect) {
  unsigned char result = r.process();
  if (result) {    
    if (result == DIR_CW){rx=rx+4*increment;} // account that rx = 4 times the frequency so the increment must be 4*
    else {rx = rx-4*increment;};       
      if (rx >=29200000){rx=rx2;}; // UPPER VFO LIMIT = 4 X the opearting frequency + offset = 48866800 for the ssb with 4.9152 IF
      if (rx <=26654800){rx=rx2;}; // LOWER VFO LIMIT = 4 X the opearting frequency + Offset = 47654800
      }
}
void loop()
{
      
      checkMode(); //********Moved void checkBand out of the loop
      
      
      val = analogRead(SENSOR);
      delay(750);
      val1 = 10*log(abs(350 - val));
      display.fillRect(16, 40, val1, 3, WHITE);
      display.display();
      
  
     display.setTextSize(1);
     display.setTextColor(BLACK);
     display.setCursor (58, 18);
     display.print("K2ZA");
     display.display(); 
     
     
    
     
     
     
    if(digitalRead(SW)){  //********If SW is true do the following.    Not used until the SSB transceiver
       
       
     /*  bfo = 4913700L;
       si5351.set_freq( bfo, 0, SI5351_CLK2);*/  // remove the */ for the SSB Transceiver
       display.setTextSize(1);
       display.setTextColor(WHITE);
       display.setCursor(2,18);  // 2nd line of display
       display.println("USB");
       display.setTextSize(1);
       display.setTextColor(BLACK);
       display.setCursor(2,18);  // 2nd line of display
       display.println("LSB");
       display.display();
       
       
  }
  else{                //**********if not, do this.
      
      
      
     
    /* bfo = 4916700L;
      si5351.set_freq( bfo, 0, SI5351_CLK2);*/  // Remove */ for SSB Transceiver
      
      display.setTextSize(1);
       display.setTextColor(WHITE);
       display.setCursor(2,18);  // 2nd line of display
       display.println("LSB");
      
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(2, 18);  // 2nd line of display
      display.println("USB");
      display.display();
      
  }     
  
    if (rx != rx2){  
        display.clearDisplay();   // clears the screen and buffer  
        showFreq();
        si5351.set_freq(rx  , SI5351_PLL_FIXED, SI5351_CLK0);
        
        si5351.set_freq(bfo  , SI5351_PLL_FIXED, SI5351_CLK2);
        rx2 = rx;
        
  
        display.setCursor(60,8);
        display.print(hertz);
        display.display();
      } 
      
    buttonstate = digitalRead(A3);
    if(buttonstate == LOW) {
        setincrement(); 
        display.clearDisplay ();
        display.setCursor(60,8);
        display.print(hertz);
        display.display();
        showFreq();
    };
    
    delay(25);
      
      display.fillRect(1, 35, 83, 12, WHITE); // Makes S Mtr Background & tick marks for scaling the S Meter  S1, S3, S5, S9, 30/9
      display.fillRect(1, 35, 83, 12, BLACK);
      display.fillRect(40, 37, 2, 2, WHITE);
      display.fillRect(30,37,2,2, WHITE);
      display.fillRect(20,37,2,2, WHITE);
      display.fillRect(55,37,2,2,WHITE);
      display.fillRect(75,37,2,2,WHITE);
      
      display.display();
      
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(3,38);
      display.print("S=");
      
    
     display.setTextSize(1);
     display.setTextColor(BLACK);
     display.setCursor (2, 26);
     display.print("  LBS Part I ");
     display.display();
     
}
void checkMode(){
    buttonState = digitalRead(SW1); // creates a 10 second tuning pulse trani 50% duty cycle and makes TUNE appear on the screen
   if(buttonState != lastButtonState){
    if(buttonState == LOW){
     display.setTextSize(1);
     display.setTextColor(BLACK);
     display.setCursor(26, 18);  // 2nd line of display
     display.print("TUNE");
     display.display();
     delay(10);
     for(int i = 0; i < 100; i++) {
     tone(10, NOTE_B5);
     delay(50);
     noTone(10);
     delay(50);
     }
     
    } 
   else{
  
     display.setTextSize(1);    // This prints a white TUNE over the black TUNE and makes it disappear from the scereen
     display.setTextColor(WHITE);
     display.setCursor(26, 18);
     display.print("TUNE");
     display.display();
     noTone(10);
}
lastButtonState = buttonState;
}
    Serial.println("si5351 VFO");
    Serial.println(rx);
    Serial.println(bfo);
    Serial.println((rx/4)-bfo);
    Serial.println(val);
    Serial.println(val1);
    Serial.println(NOTE_B5);
    Serial.println("  ");
    Serial.println("  ");
    
    
}
