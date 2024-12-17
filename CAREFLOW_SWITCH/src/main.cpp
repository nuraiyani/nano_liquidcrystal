#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Pressbutton.h>

#define DISP_ITEM_ROWS 2        //number of rows usable in the display(depends on display size)
#define DISP_CHAR_WIDTH 16      //general info about the hiw many characters in single rows
#define PACING_MS 25            //minimum wait milisecond between executing code in menu loop
#define FLASH_RST_CNT 30        //number of loops between switching flash state
#define SETTING_CHKVAL 3647     //value used to manage versioning control
#define VIBRATION_MOTOR_PIN 11      // Vibration Motor (control pin for motor driver)
#define LED_GREEN 10

// ===========================================================
// ||                   DECLARATIONS                        ||
//============================================================

//i/o ports allocations-----------------------------------------------------------
const int BTN_OK = A0;
const int BTN_BACK = A2;
const int BTN_UP = A3;
const int BTN_DOWN = A1;
const int BTN_PLUS = 6;
const int BTN_MINUS = 7;

//BUTTONS --------------------------------------------------------------------------
PressButton btnOk(BTN_OK);
PressButton btnBack(BTN_BACK);
PressButton btnUp(BTN_UP);
PressButton btnDown(BTN_DOWN);
PressButton btnPlus(BTN_PLUS);
PressButton btnMinus(BTN_MINUS);

// Variables for button state, mode selection, and magnetic switch status
bool systemOn = false; // Indicates whether the system is currently active
unsigned long lastButtonPressTime = 0; // Stores the time of the last button press
int buttonPressCount = 0; // Counts the number of button presses
int currentMode = 0; // 0: Off, 1: Mode 1, 2: Mode 2
unsigned long modeEndTime = 0; // Stores the end time of the current mode
bool sensorActivated = false; // Indicates whether the magnetic sensor is activated
unsigned long sensorActivationTime = 0; // Stores the time of the last sensor activation
bool sterilizationPaused = false; // Indicates whether the sterilization process is paused
unsigned long remainingTime = 0; // Stores the remaining time when sterilization is paused

// Constants for mode durations and magnetic switch debounce
const unsigned long MODE1_DURATION = 60000; // 60 seconds / 1 min
const unsigned long MODE2_DURATION = 30000; // 20 seconds
const unsigned long DEMO_DURATION = 5000; // 5 seconds
const unsigned long DOUBLE_PRESS_INTERVAL = 500; // Maximum interval between button presses for double press (milliseconds)
const unsigned long SENSOR_DEBOUNCE_DELAY = 100; // Debounce delay for the magnetic sensor (milliseconds)

// MENU STRUCTURE ------------------------------------------------------------------- 

enum pageType{
        MENU_ROOT,
        MENU_SUB1,
        MENU_SUB1_A,
        //MENU_SUB1_B,
        MENU_SUB2,
        MENU_SUB2_A,
        //MENU_SUB2_B,
        MENU_SUB3,
        MENU_SUB3_A,
        MENU_SETTINGS,
};

enum pageType currPage = MENU_ROOT;
void page_MenuRoot();
void page_MenuSub1();
void page_MenuSub1_A();
//void page_MenuSub1_B();
void page_MenuSub2();
void page_MenuSub2_A();
//void page_MenuSub2_B();
void page_MenuSub3();
void page_MenuSub3_A();
void page_MenuSettings();


// MENU INTERNALS ----------------------------------------------------------------------

uint32_t loopStartMs;                                           //tracks when entered top of the lop
bool updateAllitems;                                            //flag for updating items list
bool updateItemvalue;                                        //position to update a specific items value
uint8_t itemCnt;                                                //number of items in the current menu  
uint8_t pntrPos;                                                // current pointer position
uint8_t dispOffset;                                             //display offset
uint8_t root_pntrPos = 1;
uint8_t root_dispOffSet = 0;
uint8_t flashCntr;                                              //flash counter
bool flashIsOn;                                                 //flash state
void initMenuPages(String title, uint8_t itemCount );           //sets all the common menu values and prints the menu title
void captureButtonDownState();                                  //captures the pressed down state for all buttons (set only)
void adjustBoolean(bool *v);                                    // adjusts a Boolean value depending on the button state  
void adjustUint8_t(uint8_t *v, uint8_t min, uint8_t max);       //adjusts a Boolean value depending on the button state
void doPointerNavigation();                                     //does the up/down point navigation
bool isFlashChanged();                                          //Returns true whenever the flash state changes (flash interval = PACING_)
void pacingWait();                                              //placed at the bottom of the loop to keep constant pacing of the intervals
bool menuItemPrintable(uint8_t xPos, uint8_t yPos);             //will return a positive state if the item can be display - it will also posing  

// PRINT TOOLS --------------------------------------------------------------------------------------

void printPointer();                                            //depending on flash state, prints or erases the current pointer in its 
void printOffsetArrows();                                       //print the arrows to indicate if the menu extends beyond current view
void printOnOff(boolean val);                                   //print either ON or OFF depending on the boolean state
void printUint32_tAtWidth(uint32_t value, uint8_t width, char c, boolean isRight); 

//FUNCTION FOR UVC LED ---------------------------------------------------------------------------------
void activateMode();
void deactivateSystem();
void displaySystemReady();
void displayPhysiotherapyComplete();
void check_system();

// SETTINGS -------------------------------------------------------------------------------------------

struct MySettings{
        boolean Test1_OnOff = false;
        uint8_t Test2_Num = 60 ;
        uint8_t Test3_Num = 255 ;
        uint8_t Test4_Num = 0 ;
        boolean Test5_OnnOff = true ;
        uint8_t Test6_Num = 197 ;
        uint16_t settingCheckValue = SETTING_CHKVAL; //settings check value to confirm are valud !! MUST BE AT END !!
};
MySettings settings;                                            //primary settings object
void sets_setDefaults();                                        //resets the settings object back to its default values 
void sets_Load();                                                //loads the settings from the EEPROM into the settings object.
void sets_Save();                                                 //save the values in the settings object into the EEPROM

// DISPLAY
LiquidCrystal_I2C lcd(0x27, 16, 2);     // set the LCD address to ex27 for a 16 chars and 2 line display
byte chrUp[] = {0b00000,                // Arrow up custom character data
                0b00100,
                0b00100,
                0b01110,
                0b01110,
                0b11111,
                0b11111,
                0b00000 };
byte chrDn[] = {0b00000,                // Arrow down custom character data
                0b11111,
                0b11111,
                0b01110,
                0b01110,
                0b00100,
                0b00000 };
byte chrF1[] = {0b10000,                // Menu title left side custom character data
                0b01000,
                0b10100,
                0b01010,
                0b10100,
                0b01000,
                0b10000,
                0b00000 };
byte chrF2[] = {                // Menu title right side custom character data
                0b01000,
                0b01100,
                0b01110,
                0b01111,
                0b01111,
                0b01110,
                0b01100,
                0b01000 };
byte chrAr[] = {0b00000,                // Selected item pointer custom character data
                0b01100,
                0b11110,
                0b10010,
                0b11110,
                0b01100,
                0b00000,
                0b00000 };

// ===========================================================
// ||                   SETUP                               ||
//============================================================
void setup() {

    lcd.init();
    lcd.createChar(1, chrUp);
    lcd.createChar(1, chrDn);
    lcd.createChar(1, chrF1);
    lcd.createChar(1, chrF2);
    lcd.createChar(1, chrAr);

    lcd.backlight();

    lcd.clear();
    sets_Load();
        
    // Pin Mode Configuration
    pinMode(VIBRATION_MOTOR_PIN, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(BTN_OK, INPUT_PULLUP); // Using internal pull-up resistor for the button

    // Initialize all outputs to LOW
    digitalWrite(VIBRATION_MOTOR_PIN, LOW);
    digitalWrite(LED_GREEN, LOW);
}
// ===========================================================
// ||                  MAIN LOOP                            ||
//============================================================
void loop() {

                switch(currPage){
                case MENU_ROOT : page_MenuRoot();break;
                case MENU_SUB1 : page_MenuSub1();break;
                case MENU_SUB1_A : page_MenuSub1_A();break;
                //case MENU_SUB1_B : page_MenuSub1_B();break;
                case MENU_SUB2 : page_MenuSub2();break;
                case MENU_SUB2_A : page_MenuSub2_A();break;
                //case MENU_SUB2_B : page_MenuSub2_B();break;
                case MENU_SUB3 : page_MenuSub3();break;
                case MENU_SUB3_A : page_MenuSub3_A();break;
                case MENU_SETTINGS : page_MenuSettings();break;
                }
                
  }

// ===========================================================
// ||                  MENU ROOT                            ||
//============================================================
void page_MenuRoot(){

        //initializes menu pages
        initMenuPages(F("MAIN MENU"), 5);

        // for the ROOT MENU we will recall last know position and off set of the display
        pntrPos = root_pntrPos; dispOffset = root_dispOffSet; 

        //inner loop
        while (true){

                //print the sipalay item when requested
                if(updateAllitems){

                        //print the visible item
                        if(menuItemPrintable(1,1)){lcd.print(F("MODE 1     "));}
                        if(menuItemPrintable(1,2)){lcd.print(F("MODE 2     "));}
                        if(menuItemPrintable(1,3)){lcd.print(F("DEMO MODE  "));}
                        if(menuItemPrintable(1,4)){lcd.print(F("SETTINGS   "));}

                }
        
                if(isFlashChanged()){printPointer();}

                //always clear update flags by this point
                updateAllitems = false;

                //capture the button down state
                captureButtonDownState();

                //check for the ok button
                if(btnOk.PressReleased()){

                        //for the ROOT MENU we will save last know position and offset of the display
                        root_pntrPos = pntrPos; root_dispOffSet = dispOffset; 
                        switch (pntrPos){
                                case 1: currPage = MENU_SUB1; return;
                                case 2: currPage = MENU_SUB2; return;
                                case 3: currPage = MENU_SUB3; return;
                                case 4: currPage = MENU_SETTINGS; return;
                        }
                }
                
                //otherwise check for pointer up or down button
                doPointerNavigation();

                //keep a specific pace
                pacingWait();
        }  
}
// ===========================================================
// ||                  MODE 1                               ||
//============================================================
void page_MenuSub1(){

        //initializes menu pages
        initMenuPages(F("CONTINUOUS "), 3 );
        
        //inner loop
        while (true){

                //print the sipalay item when requested
                if(updateAllitems){

                        //print the visible item
                        if(menuItemPrintable(1,1)){lcd.print(F("START "));}
                        if(menuItemPrintable(1,2)){lcd.print(F("BACK  "));}

                }
        
                if(isFlashChanged()){printPointer();}

                //always clear update flags by this point
                updateAllitems = false;

                //capture the button down state
                captureButtonDownState();

                //check for the ok button
                if(btnOk.PressReleased()){

                        switch (pntrPos){
                                case 1: currPage = MENU_SUB1_A; return;
                                case 2: currPage = MENU_ROOT; return;
                        }
                }
                else if(btnBack.PressReleased()){

                        switch (pntrPos){case 1: currPage = MENU_ROOT; return;
                        }
                }
                //otherwise check for pointer up or down button
                doPointerNavigation();

                //keep a specific pace
                pacingWait();
        }  
}
// ===========================================================
// ||                  SUB MODE 1                         ||
//============================================================
void page_MenuSub1_A(){

        //initializes menu pages
        initMenuPages(F("CONTINUOUS"), 2 );

        //inner loop
        while (true){

                //print the Display item when requested
                if(updateAllitems){

                        //print the visible item
                        if(menuItemPrintable(1,1)){
                                
                                if (systemOn) 
                                {
                                        // If system is on, stop sterilization
                                        sterilizationPaused = true;
                                        systemOn = false;
                                        remainingTime = modeEndTime - millis();
                                        lcd.clear();
                                        lcd.setCursor(0, 0);
                                        lcd.print(F("System Interrupt"));
                                        lcd.setCursor(1, 1);
                                        lcd.print(F(" Press Back >>"));                                        
                                        digitalWrite(VIBRATION_MOTOR_PIN, LOW);
                                        digitalWrite(LED_GREEN, LOW);
                                }
                                else {
                                        // If paused, resume sterilization
                                        if (sterilizationPaused) {
                                                modeEndTime = millis() + remainingTime;
                                                systemOn = true;
                                                sterilizationPaused = false;
                                                lcd.setCursor(1, 1);
                                                lcd.print(" 00 : ");
                                                lcd.print(remainingTime / 1000); // Print remaining seconds on LCD
                                                lcd.print(" : 00   ");
                                                digitalWrite(VIBRATION_MOTOR_PIN, HIGH); // Activate Vibration Motor
                                                digitalWrite(LED_GREEN, HIGH); //Activate Green LED
                                                } 
                                        else {
                                                currentMode = 1;// Mode 1 selected
                                                modeEndTime = millis() + MODE1_DURATION;
                                                activateMode();           
                                        }
                                }

                        }
                }

                //check system on and update countdown
                check_system();

                if(isFlashChanged()){printPointer();}

                //always clear update flags by this point
                updateAllitems = false;

                //capture the button down state
                captureButtonDownState();

                //check for the ok button
                if (btnBack.PressReleased()){currPage = MENU_SUB1; return;}
                //otherwise check for pointer up or down button
                doPointerNavigation();

                //keep a specific pace
                pacingWait();

  
        }
}
// ===========================================================
// ||                  MENU SUB#2                            ||
//============================================================

void page_MenuSub2(){
        
        //initializes menu pages
        initMenuPages(F("INTERMITTENT"), 3 );

        //inner loop
        while (true){

                //print the sipalay item when requested
                if(updateAllitems){

                        //print the visible item
                        if(menuItemPrintable(1,1)){lcd.print(F("START  "));}
                        if(menuItemPrintable(1,2)){lcd.print(F("BACK   "));}

                }
        
                if(isFlashChanged()){printPointer();}

                //alwats clear update flags by this point
                updateAllitems = false;

                //capture the button down state
                captureButtonDownState();

                //check for the ok button
                if(btnOk.PressReleased()){

                        switch (pntrPos){
                                case 1: currPage = MENU_SUB2_A; return;
                                case 2: currPage = MENU_ROOT; return;
                        }
                }
                else if(btnBack.PressReleased()){

                        switch (pntrPos){case 1: currPage = MENU_ROOT; return;
                        }
                }
                //otherwise check for pointer up or down button
                doPointerNavigation();

                //keep a specific pace
                pacingWait();
        }  
}
// ===========================================================
// ||                  SUB MODE 2                         ||
//============================================================
void page_MenuSub2_A(){

        //initializes menu pages
        initMenuPages(F("INTERMITTENT"), 2 );

        //inner loop
        while (true){

                //print the sipalay item when requested
                if(updateAllitems){

                        if(menuItemPrintable(1,1)){
                                
                                if (systemOn) {
                                        // If system is on, stop sterilization
                                        sterilizationPaused = true;
                                        systemOn = false;
                                        remainingTime = modeEndTime - millis();
                                        lcd.clear();
                                        lcd.setCursor(0, 0);
                                        lcd.print(F("System Interrupt"));
                                        lcd.setCursor(1, 1);
                                        lcd.print(F(" Press Back >>"));                                        
                                        digitalWrite(VIBRATION_MOTOR_PIN, LOW);
                                        digitalWrite(LED_GREEN, LOW);
                                }
                                else {
                                        // If paused, resume sterilization
                                        if (sterilizationPaused) {
                                                modeEndTime = millis() + remainingTime;
                                                systemOn = true;
                                                sterilizationPaused = false;
                                                lcd.setCursor(1, 1);
                                                lcd.print(" 00 : ");
                                                lcd.print(remainingTime / 1000); // Print remaining seconds on LCD
                                                lcd.print(" : 00   ");
                                                digitalWrite(VIBRATION_MOTOR_PIN, HIGH); // Activate Vibration Motor
                                                digitalWrite(LED_GREEN, HIGH); // Activate LED Green
                                                } 
                                        else {
                                                currentMode = 2;// Mode 2 selected
                                                modeEndTime = millis() + MODE2_DURATION;
                                                activateMode();           
                                                }
                                        }
                                }
                }

                //check system on and update countdown
                check_system();
        
                if(isFlashChanged()){printPointer();}

                //alwats clear update flags by this point
                updateAllitems = false;

                //capture the button down state
                captureButtonDownState();

                //check for the ok button
                if (btnBack.PressReleased()){currPage = MENU_SUB2; return;}
                //otherwise check for pointer up or down button
                doPointerNavigation();

                //keep a specific pace
                pacingWait();                 
        }
}

// ===========================================================
// ||                  DEMO MODE                            ||
//============================================================

void page_MenuSub3(){
        
       //initializes menu pages
        initMenuPages(F("DEMO MODE"), 3 );

        //inner loop
        while (true){

                //print the sipalay item when requested
                if(updateAllitems){

                        //print the visible item
                        if(menuItemPrintable(1,1)){lcd.print(F("START  "));}
                        if(menuItemPrintable(1,2)){lcd.print(F("BACK   "));}

                }
        
                if(isFlashChanged()){printPointer();}

                //alwats clear update flags by this point
                updateAllitems = false;

                //capture the button down state
                captureButtonDownState();

                                //check for the ok button
                if(btnOk.PressReleased()){

                        switch (pntrPos){
                                case 1: currPage = MENU_SUB3_A; return;
                                case 2: currPage = MENU_ROOT; return;
                        }
                }
                else if(btnBack.PressReleased()){currPage = MENU_ROOT; return;}
                
                //otherwise check for pointer up or down button
                doPointerNavigation();

                //keep a specific pace
                pacingWait();
        } 
}
// ===========================================================
// ||                  SUB DEMO MODE                          ||
//============================================================
void page_MenuSub3_A(){

        //initializes menu pages
        initMenuPages(F("DEMO MODE"), 2 );

        //inner loop
        while (true){

                //print the sipalay item when requested
                if(updateAllitems){

                        //print the visible item
                        if(menuItemPrintable(1,1)){
                                
                                if (systemOn) {
                                        // If system is on, stop sterilization
                                        sterilizationPaused = true;
                                        systemOn = false;
                                        remainingTime = modeEndTime - millis();
                                        lcd.clear();
                                        lcd.setCursor(0, 0);
                                        lcd.print(F("System Interrupt"));
                                        lcd.setCursor(1, 1);
                                        lcd.print(F(" Press Back >>"));                                        
                                        digitalWrite(VIBRATION_MOTOR_PIN, LOW);
                                        digitalWrite(LED_GREEN, LOW);
                                        }
                                else {
                                        // If paused, resume sterilization
                                        if (sterilizationPaused) {
                                                modeEndTime = millis() + remainingTime;
                                                systemOn = true;
                                                sterilizationPaused = false;
                                                lcd.setCursor(1, 1);
                                                lcd.print(" 00 : ");
                                                lcd.print(remainingTime / 1000); // Print remaining seconds on LCD
                                                lcd.print(" : 00   ");
                                                digitalWrite(VIBRATION_MOTOR_PIN, HIGH); // Activate Vibration Motor
                                                digitalWrite(LED_GREEN, HIGH); // Activate LED Green
                                                } 
                                        else {
                                                currentMode = 3;// Mode 1 selected
                                                modeEndTime = millis() + DEMO_DURATION;
                                                activateMode();           
                                        }
                                }

                        }
                }

                //check system on and update countdown
                check_system();

                if(isFlashChanged()){printPointer();}

                //always clear update flags by this point
                updateAllitems = false;

                //capture the button down state
                captureButtonDownState();

                //check for the ok button
                if (btnBack.PressReleased()){currPage = MENU_SUB3; return;}
                //otherwise check for pointer up or down button
                doPointerNavigation();

                //keep a specific pace
                pacingWait();
        }  
}

// ===========================================================
// ||                  MENU settings                            ||
//============================================================
void page_MenuSettings(){
        //initializes menu pages
        initMenuPages(F("SETTINGS"), 7 );

        //inner loop
        while (true){

                //print the sipalay item when requested
                if(updateAllitems){

                        //print the visible item
                        if(menuItemPrintable(1,1)){lcd.print(F("Setting 1 = "));}
                        if(menuItemPrintable(1,2)){lcd.print(F("Setting 2 = "));}
                        if(menuItemPrintable(1,3)){lcd.print(F("Setting 3 = "));}
                        if(menuItemPrintable(1,4)){lcd.print(F("Setting 4 = "));}
                        if(menuItemPrintable(1,5)){lcd.print(F("Setting 5 = "));}
                        if(menuItemPrintable(1,6)){lcd.print(F("Setting 6 = "));}

                }
                if(updateAllitems || updateItemvalue){

                        if(menuItemPrintable(13,1)){printOnOff(settings.Test1_OnOff);}
                        if(menuItemPrintable(13,2)){printUint32_tAtWidth(settings.Test2_Num, 3, ' ', false);}
                        if(menuItemPrintable(13,3)){printUint32_tAtWidth(settings.Test3_Num, 3, ' ', false);}
                        if(menuItemPrintable(13,4)){printUint32_tAtWidth(settings.Test4_Num, 3, ' ', false);}
                        if(menuItemPrintable(13,5)){printOnOff(settings.Test5_OnnOff);}
                        if(menuItemPrintable(13,6)){printUint32_tAtWidth(settings.Test6_Num, 3, ' ', false);}
                }
                if(isFlashChanged()){printPointer();}

                //always clear update flags by this point
                updateAllitems = false;
                updateItemvalue = false;

                //capture the button down state
                captureButtonDownState();

                //go back to menu layer
                if(btnBack.PressReleased()){currPage = MENU_ROOT; sets_Save(); return;}
                
                //otherwise check for pointer up or down button
                doPointerNavigation();

                //editing action based on selected item > MUST BE AFTER NAVIGATION TO ENSURE CORRECT SCREEN UPDATE!!
                switch (pntrPos)
                {
                        case 1: adjustBoolean(&settings.Test1_OnOff); break;
                        case 2: adjustUint8_t(&settings.Test2_Num, 0, 255); break;
                        case 3: adjustUint8_t(&settings.Test3_Num, 0, 255); break;
                        case 4: adjustUint8_t(&settings.Test4_Num, 0, 255); break;
                        case 5: adjustBoolean(&settings.Test5_OnnOff); break;
                        case 6: adjustUint8_t(&settings.Test6_Num, 0, 255); break;
                
                }

                //if(btnOk.LongPressed()){ sets_SetDefaults(); updateAllitems = true ;}
                //keep a specific pace
                pacingWait();
        }  
}

// ===========================================================
// ||                  MENU UVC                            ||
//============================================================

// Function to activate the selected mode
void activateMode() {

        systemOn = true; // Turn on the system
        sterilizationPaused = false;
        
        lcd.setCursor(1, 1);
        lcd.print("MODE ");
        lcd.print(currentMode);
        lcd.print(": ");
        lcd.print((modeEndTime - millis()) / 1000); // Print initial remaining seconds on LCD
        lcd.print("s   ");
        digitalWrite(VIBRATION_MOTOR_PIN, HIGH); // Activate UV-C lights
        digitalWrite(LED_GREEN, HIGH); //Activate LED Green
}

// Function to display "Sterilization Complete" on the LCD
void displayPhysiotherapyComplete() {
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("Physiotherapy");
        lcd.setCursor(4, 1);
        lcd.print("Complete");
        delay(1000);
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("Press Back >>");
}
// Function to deactivate the system
void deactivateSystem() {
        systemOn = false; // Turn off the system
        currentMode = 0; // Reset mode to Off
        digitalWrite(VIBRATION_MOTOR_PIN, LOW);
        digitalWrite(LED_GREEN, LOW);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("________________");
        delay(1000);
}
void check_system(){

        // Update the countdown display for Mode 1
        if (systemOn && (currentMode == 1 || currentMode == 2 || currentMode == 3) ) {
        unsigned long currentTime = millis();
                if (currentTime >= modeEndTime) {
                deactivateSystem();
                displayPhysiotherapyComplete();
        } 
        else {
                lcd.setCursor(1, 1);
                lcd.print(" 00 : ");
                //lcd.print(currentMode);
                //lcd.print(": ");
                lcd.print((modeEndTime - currentTime) / 1000); // Print remaining seconds on LCD
                lcd.print(" : 00   ");
                }
        }
}

// ===========================================================
// ||               TOOLS - MENU INTERVALS                  ||
//============================================================

void initMenuPages(String title, uint8_t itemCount ){

        lcd.clear();

        lcd.setCursor(0,0);

        uint8_t fillCnt = (DISP_CHAR_WIDTH - title.length()) / 2;
        if(fillCnt > 0){for(uint8_t i = 0; i< fillCnt; i++){lcd.print(F("\03"));}}
        lcd.print(title);
        if ((title.length() % 2 ) == 1){fillCnt++;}
        if(fillCnt > 0){for(uint8_t i = 0; i < fillCnt; i++){lcd.print(F("\04"));}}

        //clear all button states
        btnUp.ClearWasDown();
        btnDown.ClearWasDown();
        btnOk.ClearWasDown();
        btnBack.ClearWasDown();
        btnPlus.ClearWasDown();
        btnMinus.ClearWasDown();

        //set the menu item count
        itemCnt = itemCount;

        //current pointer positiom
        pntrPos = 1;

        //display offset
        dispOffset = 0;

        //flash counter  > force immediate draw at startup
        flashCntr = 0;

        //flash state > will switch to being on at startup
        flashIsOn = false;

        // force a full update
        updateAllitems = true;

        //capture start time
        loopStartMs = millis();
}           
void captureButtonDownState(){

        btnUp.CaptureDownState();
        btnDown.CaptureDownState();
        btnOk.CaptureDownState();
        btnBack.CaptureDownState();
        btnMinus.CaptureDownState();
        btnPlus.CaptureDownState();
}                                  
void adjustBoolean(boolean *v){
        if(btnPlus.PressReleased() || btnMinus.PressReleased()){*v = !*v; updateItemvalue = true;}
}                                    
void adjustUint8_t(uint8_t *v, uint8_t min, uint8_t max){
        
        if (btnPlus.RepeatCnt == 0 && btnMinus.Repeated()){if(*v > min){*v = *v - 1; updateItemvalue = true;}}

        if (btnMinus.RepeatCnt == 0 && btnPlus.Repeated()){if(*v < max){*v = *v + 1; updateItemvalue = true;}}
}  
void doPointerNavigation(){

        if(btnUp.PressReleased() && pntrPos > 1){
        
                flashIsOn = false;
                flashCntr = 0;
                printPointer();

                if(pntrPos - dispOffset == 1){ updateAllitems = true; dispOffset--;}

                //move the pointer
                pntrPos--;
        }

        else if (btnDown.PressReleased() && pntrPos < itemCnt)
        {
                flashIsOn = false;
                flashCntr = 0;
                printPointer();

                if(pntrPos - dispOffset == DISP_ITEM_ROWS){ updateAllitems = true; dispOffset++;}
                
                //move the pointer
                pntrPos++;
        }       
}

bool isFlashChanged(){

        //check if counter expired
        if(flashCntr == 0){
                
                //flip the pointer state
                flashIsOn = !flashIsOn;

                //reset the pointer counter
                flashCntr = FLASH_RST_CNT;

                //indicate it is time to flash
                return true;
        }
        
        //decrease the flash counter and send a negative response
        else {flashCntr--;return false;}
}                                          
void pacingWait(){

        //do the pacing wait
        while(millis() - loopStartMs < PACING_MS){delay(1);}

        //capture start time
        loopStartMs = millis();
}
                                              
bool menuItemPrintable(uint8_t xPos, uint8_t yPos){

        //basic check to see if the basic conditions to allow showing this item
        if(!(updateAllitems|| (updateItemvalue && pntrPos == yPos))){return false;}

        // make a value use later to check if there is enough offset to make the value visible
        uint8_t yMaxOffset = 0;

        //this case means that the value is typically beyond the offset display, so we remove the visible row count
        if(yPos > DISP_ITEM_ROWS) {yMaxOffset = yPos - DISP_ITEM_ROWS;}

        //taking into account any offset , check if the item position is currently visible -> position cursor and return true if so
        if(dispOffset <= (yPos-1) && dispOffset >= yMaxOffset){lcd.setCursor(xPos,yPos - dispOffset); return true;}

        //otherwise just return false
        return false;
}     

// ===========================================================
// ||               TOOLS - DISPLAY                          ||
//============================================================

void printPointer(){

        // dont allow printing pointer if less than 2 items
        if(itemCnt < 2 ){return;}

        //move the cursor 
        lcd.setCursor(0, pntrPos- dispOffset);

        //show the pointer if set
        if(flashIsOn){lcd.print(F("\01"));}

        //otherwise hide the pointer
        else{lcd.print(F(" "));}
}                                            
void printOffsetArrows(){

        lcd.setCursor(DISP_CHAR_WIDTH - 1, 1);
        if(dispOffset > 0 ){ lcd.print(F("\01"));} else{lcd.print(F(" "));}

        lcd.setCursor(DISP_CHAR_WIDTH - 1, DISP_ITEM_ROWS);
        if(itemCnt > DISP_ITEM_ROWS && itemCnt - DISP_ITEM_ROWS > dispOffset){lcd.print(F("\02"));} else{lcd.print(F(" "));}
}                                       
void printOnOff(bool val){

        if(val){lcd.print(F("ON "));}
        else{lcd.print(F("OFF"));}
}             

void printChars(uint8_t cnt, char c){

        //only bother if char count is more than zero
        if(cnt > 0 ){

                // build the character array with the single character ready for printing
                char cc[] = " "; cc[0] = c;

                //print "cnt" number of the specified character
                for(uint8_t i = 1; i<= cnt; i++){lcd.print(cc);}
        }
}
uint8_t getUint32_tCharCnt(uint32_t value){

        //value id zero then return 1
        if(value == 0){return 1;}

        //setup the working variables
        uint32_t tensCalc = 10;uint8_t cnt = 1;

        //keep increasing the tensCalc value and counting the digits for as long as the tensCalc value is lower than the value
        while(tensCalc <= value && cnt < 20){tensCalc *=10; cnt += 1;}

        //return the result!
        return cnt;
}
void printUint32_tAtWidth(uint32_t value, uint8_t width, char c, boolean isRight){

        uint8_t numChars = getUint32_tCharCnt(value);

        if(isRight){printChars(width- numChars, c);}

        lcd.print(value);

        if(!isRight){printChars(width-numChars,c);}
} 

// ===========================================================
// ||               TOOLS - SETTINGS                          ||
//============================================================

// resets the settings object back to its default values
void sets_SetDefaults() {
        MySettings tempSets;
        memcpy(&settings, &tempSets, sizeof settings);
}

// loads the settings from the EEPROM into the settings object. If settings no good then default values will be returned
void sets_Load(){

        // load the values
        EEPROM.get(0, settings);
// if the check value does not match then load the defaults
        if (settings.settingCheckValue != SETTING_CHKVAL) {sets_SetDefaults();}
}
// save the values in the settings object into the EEPROM
void sets_Save(){
    EEPROM.put(0, settings);
}