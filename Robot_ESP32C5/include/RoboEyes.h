#ifndef _FLUXGARAGE_ROBOEYES_H
#define _FLUXGARAGE_ROBOEYES_H

#include <Arduino.h>
#include <math.h>
#include <Adafruit_GFX.h>
#include "Config.h"

template<typename AdafruitDisplay>
class RoboEyes {
public:
    AdafruitDisplay *display;
    GFXcanvas16 *canvas;

    //Thong so co ban
    uint16_t BGCOLOR = 0; 
    uint16_t MAINCOLOR = 1;
    uint16_t BLUSHCOLOR = 0xFB76; //Mau hong
    int screenWidth = 128; 
    int screenHeight = 64; 
    int frameInterval = 20; 
    unsigned long fpsTimer = 0; 

    //Co trang thai
    bool dizzy = 0; bool shy = 0; bool happy = 0; bool crying = 0; bool yawning = 0; bool confused = 0;
    bool curious = 0; bool cyclops = 0; bool eyeL_open = 0; bool eyeR_open = 0; 
    bool clapping = 0; unsigned long clapStartTime = 0; bool prevClapping = 0;

    //Mat trai
    int eyeLwidthDefault = 36; int eyeLheightDefault = 36;
    int eyeLwidthCurrent = eyeLwidthDefault; int eyeLheightCurrent = 1; 
    int eyeLwidthNext = eyeLwidthDefault; int eyeLheightNext = eyeLheightDefault;
    int eyeLheightOffset = 0;
    byte eyeLborderRadiusDefault = 8;
    byte eyeLborderRadiusCurrent = eyeLborderRadiusDefault;
    byte eyeLborderRadiusNext = eyeLborderRadiusDefault;

    //Mat phai
    int eyeRwidthDefault = eyeLwidthDefault; int eyeRheightDefault = eyeLheightDefault;
    int eyeRwidthCurrent = eyeRwidthDefault; int eyeRheightCurrent = 1; 
    int eyeRwidthNext = eyeRwidthDefault; int eyeRheightNext = eyeRheightDefault;
    int eyeRheightOffset = 0;
    byte eyeRborderRadiusDefault = 8;
    byte eyeRborderRadiusCurrent = eyeRborderRadiusDefault;
    byte eyeRborderRadiusNext = eyeRborderRadiusDefault;

    //Toa do va khoang cach 2 mat
    int spaceBetweenDefault = 10;
    int spaceBetweenCurrent = spaceBetweenDefault;
    int spaceBetweenNext = 10;
    int eyeLift = 30;
    int faceY() { return (screenHeight - eyeLheightCurrent) / 2 - eyeLift; } //Do cao cho mat

    int eyeLxDefault = ((screenWidth)-(eyeLwidthDefault+spaceBetweenDefault+eyeRwidthDefault))/2;
    int eyeLyDefault = ((screenHeight-eyeLheightDefault)/2);
    int eyeLx = eyeLxDefault; int eyeLy = eyeLyDefault;
    int eyeLxNext = eyeLx; int eyeLyNext = eyeLy;

    int eyeRxDefault = eyeLx+eyeLwidthCurrent+spaceBetweenDefault;
    int eyeRyDefault = eyeLy;
    int eyeRx = eyeRxDefault; int eyeRy = eyeRyDefault;
    int eyeRxNext = eyeRx; int eyeRyNext = eyeRy;

    //Bien hieu ung
    bool hFlicker = 0; bool hFlickerAlternate = 0; byte hFlickerAmplitude = 2;
    bool vFlicker = 0; bool vFlickerAlternate = 0; byte vFlickerAmplitude = 10;
    bool autoblinker = 0; int blinkInterval = 1; int blinkIntervalVariation = 4; unsigned long blinktimer = 0; 
    bool idle = 0; int idleInterval = 1; int idleIntervalVariation = 3; unsigned long idleAnimationTimer = 0;
    bool laugh = 0; unsigned long laughAnimationTimer = 0; int laughAnimationDuration = 500; bool laughToggle = 1;
    bool sweat = 0; byte sweatBorderradius = 3;
    int sweat1XPosInitial = 2; int sweat1XPos; float sweat1YPos = 2; int sweat1YPosMax; float sweat1Height = 2; float sweat1Width = 1;
    int sweat2XPosInitial = 2; int sweat2XPos; float sweat2YPos = 2; int sweat2YPosMax; float sweat2Height = 2; float sweat2Width = 1;
    int sweat3XPosInitial = 2; int sweat3XPos; float sweat3YPos = 2; int sweat3YPosMax; float sweat3Height = 2; float sweat3Width = 1;

    //Khoi tao
    RoboEyes(AdafruitDisplay &disp) : display(&disp) {};

    void setFramerate(byte fps) { frameInterval = 1000/fps; }
    void setDisplayColors(uint16_t background, uint16_t main) { BGCOLOR = background; MAINCOLOR = main; }
    void setWidth(byte leftEye, byte rightEye) { eyeLwidthNext = leftEye; eyeRwidthNext = rightEye; eyeLwidthDefault = leftEye; eyeRwidthDefault = rightEye; }
    void setHeight(byte leftEye, byte rightEye) { eyeLheightNext = leftEye; eyeRheightNext = rightEye; eyeLheightDefault = leftEye; eyeRheightDefault = rightEye; }
    void setBorderradius(byte leftEye, byte rightEye) { eyeLborderRadiusNext = leftEye; eyeRborderRadiusNext = rightEye; eyeLborderRadiusDefault = leftEye; eyeRborderRadiusDefault = rightEye; }
    void setSpacebetween(int space) { spaceBetweenNext = space; spaceBetweenDefault = space; }
    
    void centerEyes() {
        eyeLwidthCurrent = eyeLwidthDefault;
        eyeRwidthCurrent = eyeRwidthDefault;
        spaceBetweenCurrent = spaceBetweenDefault;

        eyeLx = (screenWidth - (eyeLwidthDefault + spaceBetweenDefault + eyeRwidthDefault)) / 2;
        eyeLy = (screenHeight - eyeLheightDefault) / 2 - eyeLift;
        eyeRx = eyeLx + eyeLwidthDefault + spaceBetweenDefault;
        eyeRy = eyeLy;

        eyeLxNext = eyeLx; eyeLyNext = eyeLy;
        eyeRxNext = eyeRx; eyeRyNext = eyeRy;
        eyeLxDefault = eyeLx; eyeLyDefault = eyeLy;
        eyeRxDefault = eyeRx; eyeRyDefault = eyeRy;
    }

    void setMood(unsigned char mood) {
        dizzy = 0; shy = 0; happy = 0; crying = 0; yawning = 0; confused = 0; clapping = 0;
        eyeLxNext = eyeLxDefault; eyeLyNext = eyeLyDefault; //Dua mat ve vi tri mac dinh
        switch (mood) {
            case DIZZY: dizzy = 1; break;
            case SHY: shy = 1; break;
            case HAPPY: happy = 1; break;
            case CRYING: crying = 1; break;
            case YAWNING: yawning = 1; break;
            case CONFUSED: confused = 1; break;
            case CLAPPING: clapping = 1; break;
            default: break;
        }
    }

    void setPosition(unsigned char position) {
        switch (position) {
            case N: eyeLxNext = getScreenConstraint_X()/2; eyeLyNext = 0; break;
            case NE: eyeLxNext = getScreenConstraint_X(); eyeLyNext = 0; break;
            case E: eyeLxNext = getScreenConstraint_X(); eyeLyNext = getScreenConstraint_Y()/2; break;
            case SE: eyeLxNext = getScreenConstraint_X(); eyeLyNext = getScreenConstraint_Y(); break;
            case S: eyeLxNext = getScreenConstraint_X()/2; eyeLyNext = getScreenConstraint_Y(); break;
            case SW: eyeLxNext = 0; eyeLyNext = getScreenConstraint_Y(); break;
            case W: eyeLxNext = 0; eyeLyNext = getScreenConstraint_Y()/2; break;
            case NW: eyeLxNext = 0; eyeLyNext = 0; break;
            default: eyeLxNext = getScreenConstraint_X()/2; eyeLyNext = getScreenConstraint_Y()/2; break;
        }
    }

    void setAutoblinker(bool active, int interval, int variation) { autoblinker = active; blinkInterval = interval; blinkIntervalVariation = variation; blinktimer = millis() + (interval * 1000) + (random(variation) * 1000); }
    void setAutoblinker(bool active) { autoblinker = active; }
    void setIdleMode(bool active, int interval, int variation) { idle = active; idleInterval = interval; idleIntervalVariation = variation; }
    void setIdleMode(bool active) { idle = active; }
    void setCuriosity(bool curiousBit) { curious = curiousBit; }
    void setCyclops(bool cyclopsBit) { cyclops = cyclopsBit; }
    void setHFlicker (bool flickerBit, byte Amplitude) { hFlicker = flickerBit; hFlickerAmplitude = Amplitude; }
    void setHFlicker (bool flickerBit) { hFlicker = flickerBit; }
    void setVFlicker (bool flickerBit, byte Amplitude) { vFlicker = flickerBit; vFlickerAmplitude = Amplitude; }
    void setVFlicker (bool flickerBit) { vFlicker = flickerBit; }
    void setSweat (bool sweatBit) { sweat = sweatBit; }
    
    int getScreenConstraint_X() { return screenWidth-eyeLwidthCurrent-spaceBetweenCurrent-eyeRwidthCurrent; } 
    int getScreenConstraint_Y() { return screenHeight-eyeLheightDefault; }

    void close() { eyeLheightNext = 1; eyeRheightNext = 1; eyeL_open = 0; eyeR_open = 0; }
    void open() { eyeL_open = 1; eyeR_open = 1; }
    void blink() { close(); open(); }
    void close(bool left, bool right) { if(left){ eyeLheightNext = 1; eyeL_open = 0; } if(right){ eyeRheightNext = 1; eyeR_open = 0; } }
    void open(bool left, bool right) { if(left){ eyeL_open = 1; } if(right){ eyeR_open = 1; } }
    void blink(bool left, bool right) { close(left, right); open(left, right); }
    void anim_laugh() { laugh = 1; }
    
    //CHUYEN TRANG THAI
    bool eyesClosed() { return eyeLheightCurrent <= 2 && (cyclops || eyeRheightCurrent <= 2); }

    void snapOpen() {
        eyeLheightCurrent = eyeLheightNext = eyeLheightDefault;
        eyeRheightCurrent = eyeRheightNext = eyeRheightDefault;
        eyeL_open = 1; eyeR_open = 1;
        eyeLx = eyeLxDefault; eyeLy = eyeLyDefault;
        eyeRx = eyeRxDefault; eyeRy = eyeRyDefault;
    }

    void snapClosed() {
        eyeLheightCurrent = 1; eyeRheightCurrent = 1;
        eyeLx = eyeLxDefault; eyeLy = eyeLyDefault;
        eyeRx = eyeRxDefault; eyeRy = eyeRyDefault;
        blink();
    }

    void begin(int width, int height, byte frameRate);
    void update();
    void drawEyes();
};

#include "RoboEyes.hpp"

#endif