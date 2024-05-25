#include "PressButton.h"

//Initializer for button. Required the pin value
PressButton::PressButton(int pin){
    _IoPin = pin;
    pinMode(_IoPin, INPUT_PULLUP);


}    

//returns the IO pin the button is configured to use
int PressButton::GetIOPin(){return _IoPin;}             

//does a debounced check to see if the specified button is in the down state
boolean PressButton::IsDown(){return digitalRead(_IoPin)== LOW && digitalRead(_IoPin) == LOW;}  

//does debounced check to see if specified button is in the up state
boolean PressButton::IsUp(){return digitalRead(_IoPin) == HIGH && digitalRead(_IoPin) == HIGH;}

// will set the WasDown flag true if was pressed when checked - also returns WasDown state at the same time 
boolean PressButton::CaptureDownState(){if(IsDown()){WasDown = true;} return WasDown;}

//clears the was down state, returns true if clear was done
boolean PressButton::CleasWasDown(){if(WasDown){WasDown = false; return true;} return false;} 

//provides a trigger action if the button was pressed but is now released > Clears WasDown
boolean PressButton::PressReleased(){if (WasDown && IsUp()){RepeatCnt = 0; WasDown = false; return true;} return false;}  

 //provides trigger for long press case 
boolean PressButton::LongPressed(){return(Repeated() && RepeatCnt == 2);}

//Provides and action trigger at a increasingly higher frequency for as long as a key is pressed
boolean PressButton::Repeated(){
    //snapshot the current time
    uint32_t currMs = millis();

    //check if repeat signal should be sent
    if (WasDown && (
                        RepeatCnt == 0 ||                                       
                        (RepeatCnt > 5 && currMs >= (LasRepeatMs + 200)) ||     
                        currMs >= (LasRepeatMs + 250 + (50 * (5- RepeatCnt)))
                    )){

        //increase the repeat count limiting to mac value 999 to avoid roll over
        if (RepeatCnt < 999 ){RepeatCnt += 1;}
        //clear the was down state to get the next repeat
        WasDown = false;
        //take note of the last repeat time to be used n next comparison & send back a positive trigger
        LasRepeatMs = currMs;
        //send back positive trigger
        return true;
    }
    //otherwise
    else{
        //has repeated and button is now up, then clear the repeat count and was down state
        if (RepeatCnt > 0  && IsUp()){RepeatCnt = 0; WasDown = false;}
        //send back a negative trigger
        return false;}
}         
