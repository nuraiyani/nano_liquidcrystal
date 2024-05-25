#include <Arduino.h>

/* |
* @brief function and debounce button  
*/

class PressButton {

private: 

    int _IoPin;                //Internal value - IO Pin 

public:

    PressButton(int pin);       //Initializer for button. Required the pin value
    int GetIOPin();             //returns the IO pin the button is configured to use
    boolean WasDown = false;    //store was down states as set by CaptureDownState()
    uint32_t RepeatCnt = 0;     //maintains a count number of times repeat trigger sent
    uint32_t LasRepeatMs = 0;   //keeps a records of when last repeatr trigger was sent
    boolean IsDown();           //does a debounced check to see if the specified button is in the down state
    boolean IsUp();             //does debounced check to see if specified button is in the up state
    boolean CaptureDownState(); // will set the WasDown dflag true if was pressed when checked - also returns WasDown
    boolean CleasWasDown();     //clears the was down state, returns true if clear was done
    boolean PressReleased();    //provides a trigger action if the button was pressed but is now released > Clears WasDown
    boolean LongPressed();      //provides trigger for long press case
    boolean Repeated();         //Provides and action trigger at a increasingly higher frequency for as long as a key is pressed

};




