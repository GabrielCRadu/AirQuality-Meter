#include <Wire.h>
#include "FT62XXTouchScreen.h"
#include "Free_Fonts.h"
#include <SPI.h>
#include <TFT_eSPI.h>  // Hardware-specific library
int buzzer = 25;
bool BuzzerOn;

//BME280*//
#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"
Adafruit_BME280 bme;  //foloseste I2C
float Temperatura,Umiditate,Presiune,tmax,umax,pmax;
float tmin=100,umin=100,pmin=1500;

//*Senzor MQ2 - Metan//
int MQ2 = 34;  //pinul la care este legat senzorul
int ConcentratieGaz;
//https://github.com/miguel5612/MQSensorsLib/blob/master/examples/MQ-2/MQ-2.ino


//Includem fisierul cu imagini
#include "Imagini.h"  // convertor imagine to hex  http://rinkydinkelectronics.com/t_imageconverter565.php

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

//ButoaneFizice//
int ButonSus = 17;
int ButonJos = 2;

int TFT_BL = 23;  // Backlight

#define DISPLAY_WIDTH 480   //Latimea Ecranului
#define DISPLAY_HEIGHT 320  //Inaltimea ecranului

int XMiddle = DISPLAY_WIDTH / 2;   //coordonata pe X din mijlocul ecranului
int YMiddle = DISPLAY_HEIGHT / 2;  //coordonata pe Y din mijlocul ecranului

FT62XXTouchScreen touchScreen = FT62XXTouchScreen(DISPLAY_HEIGHT, 18, 19);  //(DISPLAY_HEIGHT, PIN_SDA, PIN_SCL);

int16_t h;
int16_t w;

bool MeniuPrincipal;
int Menu = 1;

//Variabile pt refresh valori ecran/
unsigned long TimpAnterior = 0;
unsigned long TimpActual = 0;
unsigned int TimpRefresh = 1000;  // 1 secunda


// Invoke the TFT_eSPI button class and create all the button objects
TFT_eSPI_Button key[16];

/*****/

void setup() {
  pinMode(ButonSus, INPUT_PULLUP);
  pinMode(ButonJos, INPUT_PULLUP);
  pinMode(MQ2, INPUT);      //definim pinul la care este legat senzorul MQ2 ca intrare
  pinMode(TFT_BL, OUTPUT);  // Backlight
  digitalWrite(TFT_BL, 128);
  pinMode(buzzer, OUTPUT);
  Serial.begin(115200);

  tft.init();
  touchScreen.begin();

  h = tft.height();
  w = tft.width();

  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(FS18);

  //key[5].initButton(&tft, 45, 295,  70, 20, TFT_RED, TFT_BLACK, TFT_ORANGE, "<<", 1);// x, y, w, h, culoare rama,culoare buton, culoare text.

  // Newer function instead accepts upper-left corner & size
  //key[1].initButtonUL(&tft,, int16_t x1, int16_t y1, uint16_t w, uint16_t h, uint16_t outline, uint16_t fill, uint16_t textcolor, char *label, uint8_t textsize)
  key[1].initButtonUL(&tft, 10, 230, 60, 80, TFT_WHITE, TFT_WHITE, TFT_BLACK, "<", 1);  // x, y, w, h, culoare rama,culoare buton, culoare text.
  key[1].drawButton();

  key[2].initButtonUL(&tft, 410, 230, 60, 80, TFT_WHITE, TFT_WHITE, TFT_BLACK, ">", 1);  // x, y, w, h, culoare rama,culoare buton, culoare text.
  key[2].drawButton();

  key[3].initButtonUL(&tft, 430, 10, 40, 40, TFT_DARKGREY, TFT_LIGHTGREY, TFT_BLACK, "Setari", 1);  // x, y, w, h, culoare rama,culoare buton, culoare text.
  tft.pushImage(430, 10, 40, 40, CogWheel);                                                         //Afisare iconita Setari

  //BME280*//
  bool status;

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin();
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    //while (1);
  }
  //BME280*//

  MeniuPrincipal = true;
  Meniu1();
}


/***VOID LOOP***/
void loop() {
  TouchPoint touchPos = touchScreen.read();

  if (touchPos.touched) {
    //Serial.printf("X: %d, Y: %d\n", touchPos.xPos, touchPos.yPos);


    // / Check if any key coordinate boxes contain the touch coordinates
    for (uint8_t b = 0; b < 15; b++) {
      // if (pressed && key[b].contains(touchPos.xPos, touchPos.yPos)) {
      if (key[b].contains(touchPos.xPos, touchPos.yPos)) {
        key[b].press(true);  // tell the button it is pressed
      } else {
        key[b].press(false);  // tell the button it is NOT pressed
      }
    }
    Butoane();  //decide ce buton afisat este apasat
  }
  ButoaneFizice();  //verifica apasarea butoanelor fizice

  TimpActual = millis();
  if (TimpActual - TimpAnterior >= TimpRefresh) {

    SenzorMQ2();     //Cheama bucla SenzorMQ2
    SenzorBME280();  //Cheama bucla BME280
    if (BuzzerOn) BuzzerOn = false;
    else BuzzerOn = true;

    if (Menu == 1) {

      tft.setTextColor(TFT_WHITE, TFT_BLACK);

      if (Umiditate>=65 || Temperatura>=30 && BuzzerOn) {   
        digitalWrite(25, HIGH);
      } 
      else {
        digitalWrite(25, LOW);
      }
       if (Temperatura>=30) tft.setTextColor(TFT_RED, TFT_BLACK);
       else tft.setTextColor(TFT_WHITE, TFT_BLACK);

        tft.drawString("Temperatura", 10, 80, 4);   
        tft.drawFloat(Temperatura, 2, 240, 80, 1);  
        tft.drawString("*C", 320, 85, 4);

        if (Umiditate>=65) tft.setTextColor(TFT_RED, TFT_BLACK);
        else tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("Umiditate", 10, 120, 4);   
        tft.drawFloat(Umiditate, 2, 240, 120, 1);  
        tft.drawString("%", 320, 125, 4);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);


        tft.drawString("Presiune", 10, 160, 4);   
        tft.drawFloat(Presiune, 2, 240, 160, 1);  
        tft.drawString("hPa", 340, 165, 4);

        tft.drawString("Concentratie Gaz", 10, 200, 4);  
        tft.drawFloat(ConcentratieGaz, 0, 240, 200, 1);  

      TimpAnterior = millis();
    }
    delay(20);  

          if(Menu==2){     
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        if(tmax<Temperatura)tmax=Temperatura;
        tft.drawString("Temperatura Maxima", 10, 80, 4);   
        tft.drawFloat(tmax, 2, 280, 80, 1);  
        tft.drawString("*C", 370, 85, 4); 
        
        if(umax<Umiditate)umax=Umiditate;
        tft.drawString("Umiditate Maxima", 10, 120, 4);   
        tft.drawFloat(umax, 2, 280, 120, 1);  
        tft.drawString("%", 370, 125, 4);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);

        if(pmax<Presiune)pmax=Presiune;
        tft.drawString("Presiune Maxima", 10, 160, 4);   
        tft.drawFloat(pmax, 2, 280, 160, 1);  
        tft.drawString("hPa", 380, 165, 4);

   } 
          if(Menu==3){
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        if(Temperatura<tmin)tmin=Temperatura;
        tft.drawString("Temperatura Minima", 10, 80, 4);   
        tft.drawFloat(tmin, 2, 280, 80, 1);  
        tft.drawString("*C", 370, 85, 4); 
        
        if(Umiditate<umin)umin=Umiditate;
        tft.drawString("Umiditate Minima", 10, 120, 4);   
        tft.drawFloat(umin, 2, 280, 120, 1);  
        tft.drawString("%", 370, 125, 4);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);

        if(Presiune<pmin)pmin=Presiune;
        tft.drawString("Presiune Minima", 10, 160, 4);   
        tft.drawFloat(pmin, 2, 280, 160, 1);  
        tft.drawString("hPa", 380, 165, 4);
  }
 }   
}

      


  //SENZORI//

  void SenzorBME280() {

    Temperatura = bme.readTemperature();
    Serial.print("Temperature = ");
    Serial.print(Temperatura);
    Serial.println(" *C");

    Presiune = bme.readPressure() / 100.0F;  //transformare din HPa in Pa
    Serial.print("Pressure = ");
    Serial.print(Presiune);
    Serial.println(" hPa");

    Umiditate = bme.readHumidity();
    Serial.print("Humidity = ");
    Serial.print(Umiditate);
    Serial.println(" %");

  }

  void SenzorMQ2() {
    ConcentratieGaz = analogRead(MQ2);
    Serial.println(ConcentratieGaz);  //se printeaza pe serial valoarea analogica a pinului MQ2
    //SA SE CALCULEZE VALOAREA PARTICULELOR DUPA FORMULA DE MAI JOS

    // How to Detect Concentration of Gas by Using MQ2 Sensor
    //https://www.instructables.com/How-to-Detect-Concentration-of-Gas-by-Using-MQ2-Se/


    //COMPARARE NIVEL DEPASIT
  }


  //BUTOANE//

  void ButoaneFizice() {
    bool ButonSusApasat = !digitalRead(ButonSus);
    bool ButonJosApasat = !digitalRead(ButonJos);

    if (ButonJosApasat) {
      Menu--;
      if (Menu < 1) Menu = 3;
      Serial.println(Menu);
      delay(400);  //Button debouncing
      Meniuri();
    }


    if (ButonSusApasat) {
      Menu++;
      if (Menu > 3) Menu = 1;
      Serial.println(Menu);
      delay(400);  // Button debouncing
      Meniuri();
    }
  }

  void Butoane() {



    // else {
    //  for (uint8_t b = 0; b < 15; b++) {
    //   key[b].press(false);  // tell the button it is NOT pressed
    // }
    //}

    if (MeniuPrincipal == true) {                //-MENIURI PRINCIPALE
      tft.pushImage(430, 10, 40, 40, CogWheel);  //Afisare iconita Setari
      if (key[1].justPressed()) {
        // < //

              key[1].press(false);
        Menu--;
        if (Menu < 1) Menu = 3;

        Serial.println(Menu);
        delay(200);  // UI debouncing
        Meniuri();
      }



      if (key[2].justPressed()) {
        // > //

              key[2].press(false);
        Menu++;
        if (Menu > 3) Menu = 1;

        Serial.println(Menu);
        delay(200);  // UI debouncing
        Meniuri();
      }

      if (key[3].justPressed()) {
        // SETARI //

          key[3].press(false);
        Setari();
        MeniuPrincipal = false;
      }
    }  //-MENIURI PRINCIPALE


    if (MeniuPrincipal == false) {  //-MENIURI SECUNDARE

      if (key[1].justPressed()) {
        // Iesire // key[1].press(false);
        MeniuPrincipal = true;
        tft.fillScreen(TFT_BLACK);
        tft.pushImage(430, 10, 40, 40, CogWheel);  //Afisare iconita Setari
        Meniuri();
      }
    }








    // Check if any key has changed state
    //for (uint8_t b = 0; b < 15; b++) {

    // if (key[b].justPressed()) {
    //   key[b].drawButton(true);  // draw invert

    //   delay(100);  // UI debouncing
    // }


    //if (key[1].justReleased())key[1].press(false);  // tell the button it is NOT pressed
    // if (key[b].justReleased()) key[b].drawButton();  // draw normal




    // }
  }


  //MENIURI//
  void Meniuri() {
    tft.fillScreen(TFT_BLACK);
    if (Menu == 1) Meniu1();
    else if (Menu == 2)
      Meniu2();
    else if (Menu == 3)
      Meniu3();
      
  }

  void Meniu1() {
    //tft.setTextSize(1);
    //tft.fillScreen(TFT_BLACK);
    key[1].drawButton();                               //Afiseaza butonul"<"
    key[2].drawButton();                               //Afiseaza butonul">"
    tft.fillRoundRect(5, 10, 100, 40, 10, TFT_WHITE);  //x, y, w, h, r, color
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString("Meniu1", 15, 20, 4);
  }

  void Meniu2() {
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);
    key[1].drawButton();                               //Afiseaza butonul"<"
    key[2].drawButton();                               //Afiseaza butonul">"
    tft.fillRoundRect(5, 10, 100, 40, 10, TFT_WHITE);  //x, y, w, h, r, color
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString("Meniu2", 15, 20, 4);
  }

  void Meniu3() {
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);
    key[1].drawButton();                               //Afiseaza butonul"<"
    key[2].drawButton();                               //Afiseaza butonul">"
    tft.fillRoundRect(5, 10, 100, 40, 10, TFT_WHITE);  //x, y, w, h, r, color
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString("Meniu3", 15, 20, 4);
  }

  void Setari() {

    tft.fillScreen(TFT_BLACK);
    key[1].drawButton();                               //Afiseaza butonul Inapoi (Folosesc Butonul "Stanga <" pentru a iesi din meniu)
    tft.fillRoundRect(5, 10, 100, 40, 10, TFT_WHITE);  //x, y, w, h, r, color
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString("Setari", 15, 20, 4);
  }