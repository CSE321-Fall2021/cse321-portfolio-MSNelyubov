/******************************************************************************
*   File Name:      cse321_project2_mnelyubo_main.cpp
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   10/10/2021
*   Last Modified:  10/27/2021
*   Purpose:        
*               This program takes inputs from a 4x4 matrix keypad to control a timer.
*               Timer mode-based text and input/remaining time are output to a connected LCD.
*               
*               The main function of the program initializes the system by configuring a set of four GPIO pins as
*                 outputs to supply voltage to the keypad, assigning event handlers to the rising and falling edges 
*                 of input interrupts that are connected to the keypad, and setting the initial timer mode to input.

*               After the initialization is complete, the main function cycles through the four output channels, 
*                 providing power to one channel at a time and proceeding to the next input after a short time interval 
*                 in which no input keystroke is detected.  This allows a fast response time to any of the 16 keys and 
*                 eliminates the opportunity for duplicate inputs to be detected due to a single key press.
*               
*   Functions:      
*               populateLcdOutput
*               handleMatrixButtonEvent
*               handleInputKey
*               tickCountdownTimer
*               tickBounceHandler
*               switchToCountdownMode
*               
*               The following set of functions are used as handlers for the 
*                 rising and falling edge behaviors of keypad buttons:
*                 * rising_isr_abc
*                 * rising_isr_369
*                 * rising_isr_258
*                 * rising_isr_147
*                 * falling_isr_abc
*                 * falling_isr_369
*                 * falling_isr_258
*                 * falling_isr_147
*               
*   Assignment:     CSE321 Project 2
*
*   Inputs:         4x4 matrix array input buttons
*
*   Outputs:        LCD display, 2 LED channels
*
*   Constraints:    in order to avoid undefined behavior, no more than one
*                     keypad button should be pressed at any point in time
*
*   References:     
*               https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
*
*
******************************************************************************/
#include "mbed.h"
#include "1802.h"
#include <cstdio>
#include <ctime>
#include <string>

#define COL 16
#define ROW 2

#define RisingEdgeInterrupt 1
#define FallingEdgeInterrupt 0

//Associated column values with their respective character key value index in the matrix
#define ColABC 0
#define Col369 1
#define Col258 2
#define Col147 3

//dimension (row and column) of the Matrix keypad
#define MatrixDim 4

//Timer modes
#define InputMode     0x0
#define CountdownMode 0x2
#define StoppedMode   0x4
#define AlarmMode     0x6

#define ModeCount 4

//Input Duration string index values correspoinding to indexes in the string array modeLCDvalues
#define DurationInputMinutesIndex 11
#define DurationInput10SecondsIndex 13
#define DurationInputSecondsIndex 14

//Countdown Duration string index values corresponding to indexes in the string array modeLCDvalues
#define CountdownMinutesIndex 6
#define Countdown10SecondsIndex 8
#define CountdownSecondsIndex 9

//System Time Rates
#define KeyPadHighInputCycleTime 5
#define KeyPadFallingEdgeBufferTime 1
#define bounceTimeoutWindow 100

/****************************
  *  Function Declarations  *
  ***************************/

////declare individual interrupt handler events for each column
//declare rising edge interrupt handler events
void rising_isr_abc(void);
void rising_isr_369(void);
void rising_isr_258(void);
void rising_isr_147(void);

//declare falling edge interrupt handler events
void falling_isr_abc(void);
void falling_isr_369(void);
void falling_isr_258(void);
void falling_isr_147(void);

//output to LCD and configure LED states based on the mode
void populateLcdOutput();

//declare general handler for Matrix keypad input events
void handleMatrixButtonEvent(int isRisingEdgeInterrupt, int column, int row);

//injection point for the controller to handle the input with respect to the system state
void handleInputKey(char inputKey);

//use a 1 Hz ticker to count down the remaining seconds
void tickCountdownTimer();

//tick down bounce handler
void tickBounceHandler();

//switch the system to Countdown Mode
void switchToCountdownMode();

/****************************
  *    Global  Variables    *
  ***************************/

//This value controls the mode of the timer.
//This value is used to determine which behaviors to perform when a key is pressed.
int timerMode = InputMode;

// This value is set to a defined (bounceTimeoutWindow) quantity of milliseconds when a button is pressed down.
// This value is ticked down every millisecond by the function tickBounceHandler.
// If the variable is accessed by handleMatrixButtonEvent while the value is positive, this indicates that a button press is most likely a duplicate.
int bounceLockout = 0;

//This value indicates the only row that is to be supplied power.
//This value is used to determine which keypad input was pressed in the function handleMatrixButtonEvent().
int keypadVccRow = 0;

//Boolean flag to indicate if the output to the LCD needs to be refreshed.
//The initial value is 1 to populate the display during startup.
int outputChangesMade = true;

//This (boolean) variable indicates if any button is currently pressed.
//A value of 1 means some button is pressed and 0 means no button is pressed.
//The polling of rows will be halted as long as this value is true.
int buttonPressed = false;

//The character on the input matrix keypad which is currently pressed down.  
//The variable defaults to '\0' when no key is pressed.
//system behavior is UNDEFINED when more than one key is pressed at the same time.
char charPressed = '\0';    

//The matrix of characters maps keypad rows/columns to their associated character
// MatrixDim + 1 used as second dimension because of null terminator in each string
char keyValues[][MatrixDim + 1] = {"dcba","#963","0852","*741"};

//The LCD output matrix. Access strings as modeLCDvalues[definedMode + LcdLineIndex]
//Each string must be exactly COL characters in length so that populateLcdOutput() 
//  will cleanly update the entire screen without needing to spend clock cycles on a clear() call.
char modeLCDvalues[ROW * ModeCount][COL] = {
    " Input Duration"," of timer: 0:00",      //Input Mode     LCD Output String
    " Time Remaining","      0:00     ",      //Countdown Mode LCD Output String
    " TIMER  STOPPED","               ",      //Stopped Mode   LCD Output String
    "               ","    Times up   "       //Alarm Mode     LCD Output String
};

/****************************
  *   Global API Objects    *
  ***************************/

CSE321_LCD lcdObject(COL,ROW);  //create interface to control the output LCD

InterruptIn colLL(PC_0);    //declare the connection to pin PC_0 as a source of input interrupts, connected to the far left column of the matrix keypad
InterruptIn colCL(PC_3);    //declare the connection to pin PC_3 as a source of input interrupts, connected to the center left column of the matrix keypad
InterruptIn colCR(PC_1);    //declare the connection to pin PC_1 as a source of input interrupts, connected to the center right column of the matrix keypad
InterruptIn colRR(PC_4);    //declare the connection to pin PC_4 as a source of input interrupts, connected to the far right column of the matrix keypad

Ticker countdownTicker;     //create a ticker that counts down the timer once per second
Ticker bounceHandlerTicker; //create a ticker that handles input lockout to mitigate bounce

int main() {
    RCC->AHB2ENR |= 0x6;    //enable RCC for GPIO ports B and C

    GPIOC->MODER |= 0x550000;       //configugure GPIO pins PC8,PC9,PC10,PC11
    GPIOC->MODER &= ~(0xAA0000);    //  as outputs

    GPIOB->MODER |= 0x500000;       //configure GPIO pins PB10,PB11 as outputs
    GPIOB->MODER &= ~(0xA00000);    //these will be used to control input/alarm indicator LEDs

    colLL.rise(&rising_isr_abc);   //assign interrupt handler for a rising edge event from the column containing buttons a,b,c,d
    colCL.rise(&rising_isr_369);   //assign interrupt handler for a rising edge event from the column containing buttons 3,6,9,#
    colCR.rise(&rising_isr_258);   //assign interrupt handler for a rising edge event from the column containing buttons 2,5,8,0
    colRR.rise(&rising_isr_147);   //assign interrupt handler for a rising edge event from the column containing buttons 1,4,7,*

    colLL.fall(&falling_isr_abc);   //assign interrupt handler for a falling edge event from the column containing buttons a,b,c,d
    colCL.fall(&falling_isr_369);   //assign interrupt handler for a falling edge event from the column containing buttons 3,6,9,#
    colCR.fall(&falling_isr_258);   //assign interrupt handler for a falling edge event from the column containing buttons 2,5,8,0
    colRR.fall(&falling_isr_147);   //assign interrupt handler for a falling edge event from the column containing buttons 1,4,7,*

    lcdObject.begin();              //initialize LCD
    populateLcdOutput();            //populate initial LCD text
    
    bounceHandlerTicker.attach(&tickBounceHandler, 1ms);    //Attach the function that ticks down milliseconds for the bounce lockout to eliminate duplicate events

    while (1) {

        //supply voltage to one output row at a time
        switch (keypadVccRow){
            case 0:
                GPIOC->ODR |= 0x100;        //supply voltage to the row of keypad buttons with labels *0#D
                break;
            case 1:
                GPIOC->ODR |= 0x200;        //supply voltage to the row of keypad buttons with labels 789C
                break;
            case 2:
                GPIOC->ODR |= 0x400;        //supply voltage to the row of keypad buttons with labels 456B
                break;
            case 3:
                GPIOC->ODR |= 0x800;        //supply voltage to the row of keypad buttons with labels 123A
                break;
        }

        thread_sleep_for(KeyPadHighInputCycleTime);   //maintain power to the row for a brief period of time to account for bounce

        //proceed to scanning the next input if and only if there is 
        //  no closed loop in the current scan set
        if(!buttonPressed){
            GPIOC->ODR &= ~(0xF00);         //reset voltage to output 0 on all pins

            thread_sleep_for(KeyPadFallingEdgeBufferTime);   //wait to give any falling edge triggers a chance to resolve before proceeding

            keypadVccRow++;                          //update the row target to poll the next row
            keypadVccRow%=MatrixDim;
        }

        populateLcdOutput();                //refresh the LCD output at the same rate as inputs are polled
    }

    return 0;
}


//The following set of functions are used as handlers for the rising and falling edge behaviors of keypad buttons.
//All of the following eight functions serve to configure the inputs for handleMatrixButtonevent().
void rising_isr_abc(void) {handleMatrixButtonEvent(RisingEdgeInterrupt,  ColABC, keypadVccRow);}
void falling_isr_abc(void){handleMatrixButtonEvent(FallingEdgeInterrupt, ColABC, keypadVccRow);}
void rising_isr_369(void) {handleMatrixButtonEvent(RisingEdgeInterrupt,  Col369, keypadVccRow);}
void falling_isr_369(void){handleMatrixButtonEvent(FallingEdgeInterrupt, Col369, keypadVccRow);}
void rising_isr_258(void) {handleMatrixButtonEvent(RisingEdgeInterrupt,  Col258, keypadVccRow);}
void falling_isr_258(void){handleMatrixButtonEvent(FallingEdgeInterrupt, Col258, keypadVccRow);}
void rising_isr_147(void) {handleMatrixButtonEvent(RisingEdgeInterrupt,  Col147, keypadVccRow);}
void falling_isr_147(void){handleMatrixButtonEvent(FallingEdgeInterrupt, Col147, keypadVccRow);}



/**
* void handleMatrixButtonEvent
* 
* Summary of the function:
*    This function converts an input event type, row, and column received 
*    from an ISR handler into the character that is represented by that 
*    button press, filters for duplicate inputs, and calls to handle the key press if appropriate.
*
* Parameters:   
*   - int isRisingEdgeInterrupt - 1 indicates that the button that triggered the event was pressed down, 0 indicates that the button was released
*   - int column - 0-3 value indicating which column the event was detected on 
*   - int row    - 0-3 value indicating which row the event occured in, based on which row is currently being supplied a voltage
*
* Return value: None
*
* Outputs:      The input detection LED is turned on and off based on whether the incoming interrupt is riding or falling edge, displaying whether or not there is currently an input detected.
*
* Description:  
*   This function will only update the state of the key press if there is no other key press currently detected.
*   Two key presses of the same key in short sequence will be ignored to prevent duplicate event handlers from being launched.
*   If a single non-conflicting key press or release is detected, the global variables charPressed and buttonPressed are set appropriately, and handleInputKey is called if the button was pressed.
*/
void handleMatrixButtonEvent(int isRisingEdgeInterrupt, int column, int row){
    char detectedKey = keyValues[column][row];      //fetch the char value associated with the index that was detected
    if(isRisingEdgeInterrupt){

        //prevent duplicate event creation if another button press was just detected
        if(bounceLockout > 0){
            return;
        }

        //enforce only one key being pressed down at a time
        if(charPressed != '\0'){
            return;
        }
        
        buttonPressed = isRisingEdgeInterrupt;     //a rising edge interrupt occurs when a button is pressed.  Set to 1 to prevent further polling until key is released
        bounceLockout = bounceTimeoutWindow;       //lock bounce timeout window after event is triggered to ignore duplicate events

        GPIOB->ODR |= 0x400;                       //send signal High to pin PB10 to indicate that a button press is detected
        charPressed = detectedKey;                 //set the global variable for the currently pressed character to the detected character
        handleInputKey(charPressed);
    }else{
        if(detectedKey != charPressed) return;        //Ignore any key release inputs that are not the pressed key
        GPIOB->ODR &= ~(0x400);                       //send signal Low to pin PB10 to indicate that a button release is detected
        charPressed = '\0';                           //reset the char value to '\0' as the key has been released
        buttonPressed = isRisingEdgeInterrupt;        //once the appropriate key has been verified, update button pressed state to resume polling
    }
}



/**
* void handleInputKey
* 
* Summary of the function:
*   This function modifies the mode or duration of the timer based on the specified input to the system.
*   The action taken as the result of any input depends upon the present mode of the system.
*
* Parameters:   
*   - char inputKey - the rising edge button press that was detected
*
* Return value: None
*
* Outputs:      The timer mode may be modified as a consequence of this function.
*
* Description:  
*   This function is broken down into handling the input to the system based on the mode of the system.
*   The branch corresponding with the present mode is selected, and then the case corresponding to the input key is executed, if one exists.
*/
void handleInputKey(char inputKey){
    outputChangesMade = true;       //assume true for any input even if no changes were actually made.  Requests LCD to be refreshed.

    if(timerMode == InputMode){     //handle button behaviors when the system is in input mode
        switch (inputKey) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                    //In Input Mode, any numeric inputs configure the timer duration.
                    //mode LCD values access:
                    //  Shift all existing numeric values one position to the left (lose most significant digit)
                    //  insert the most recently input value in lowest index
                    modeLCDvalues[InputMode + 1][DurationInputMinutesIndex]   = modeLCDvalues[InputMode + 1][DurationInput10SecondsIndex];      //shift value from 10s of seconds to minutes
                    modeLCDvalues[InputMode + 1][DurationInput10SecondsIndex] = modeLCDvalues[InputMode + 1][DurationInputSecondsIndex];        //shift value from seconds to 10s of seconds
                    modeLCDvalues[InputMode + 1][DurationInputSecondsIndex]   = inputKey;                                                       //set seconds value to the latest input

                    //handle the edge case where times are greater than 9:59 to reduce the times back down to the defined upper limit of 9:59
                    if(modeLCDvalues[InputMode + 1][DurationInputMinutesIndex] == '9' && modeLCDvalues[InputMode + 1][DurationInput10SecondsIndex] > '5'){
                        modeLCDvalues[InputMode + 1][DurationInput10SecondsIndex] = '5';    //9:5x
                        modeLCDvalues[InputMode + 1][DurationInputSecondsIndex]   = '9';    //9:59
                    }
                    break;
                
            case 'a':           //button press A is defined to be the trigger that switches the timer to Countdown Mode
                switchToCountdownMode();
                break;
        }
        return;     //return to avoid jumping into any other mode handler
    }

    if(timerMode == CountdownMode){
        switch (inputKey) {
            case 'b':           //button press B is defined to be the trigger that switches the tier to Stopped Mode
                timerMode = StoppedMode;
                break;
        }
        return;     //return to avoid jumping into any other mode handler
    }

    if(timerMode == StoppedMode){
        switch (inputKey) {
            case 'a':           //button press A is defined to be the trigger that switches the timer to Countdown Mode
                switchToCountdownMode();
                break;
            case 'd':           //button press D is defined to be the trigger that switches the timer to Input Mode
                timerMode = InputMode;
                break;
        }
        return;     //return to avoid jumping into any other mode handler
    }

    if(timerMode == AlarmMode){
        switch (inputKey) {
            case 'a':           //button press A is defined to be the trigger that switches the timer to Countdown Mode
                switchToCountdownMode();
                break;
            case 'b':           //button press B is defined to be the trigger that switches the tier to Stopped Mode
                timerMode = StoppedMode;
                break;
            case 'd':           //button press D is defined to be the trigger that switches the timer to Input Mode
                timerMode = InputMode;
                break;
        }
        return;     //return to avoid jumping into any other mode handler
    }
}


/**
* void switchToCountdownMode
* 
* Summary of the function:
*  This function sets the Countdown timer to its initial value.
*  This function should be run when the A key is pressed.
*
* Parameters:   
*
* Return value: None
*
* Outputs:    The LCD output text is configured, but not instructed to display.
*
* Description:  
*   The function first switches the timer mode to countdown mode.
*   The function then sets the timer string of coutdown mode to equal the timer string of input mode
*   The function finally reinitializes the countdown ticker to synchronize it with when countdown mode is entered
*/
void switchToCountdownMode() {
    timerMode = CountdownMode;      //set the timer to countdown mode

    //push the input mode duration to the Countdown Mode string
    modeLCDvalues[CountdownMode + 1][CountdownMinutesIndex]   = modeLCDvalues[InputMode + 1][DurationInputMinutesIndex];   //inherit minutes from latest input
    modeLCDvalues[CountdownMode + 1][Countdown10SecondsIndex] = modeLCDvalues[InputMode + 1][DurationInput10SecondsIndex]; //inherit 10's of seconds from latest input
    modeLCDvalues[CountdownMode + 1][CountdownSecondsIndex]   = modeLCDvalues[InputMode + 1][DurationInputSecondsIndex];   //inherit seconds from latest input

    //Attach the function that decrements remaining time in the Countdown mode to the ticker that triggers it once per second.
    //Reattachment here resets the duration of the ticker to allow a full second before the first tick down.
    countdownTicker.attach(&tickCountdownTimer, 1s);
}


/**
* void populateLcdOutput
* 
* Summary of the function:
*  This function handles the LCD output data flow based on the modeLCDvalues string array.
*  This function controls the output state of the alarm LED signal.
*  This function can't be successfully executed from an InterruptIn or basic Ticker call.
*
* Parameters:   None
*
* Return value: None
*
* Outputs:    
*   The LCD output is set based on the current timer Mode and (if in countdown mode) elapsed time.
*   Pin 11 from port B will be set to high if the timer is currently in alarm mode. Otherwise, it will be turned off.
*
* Description:  
*   This funciton will only attempt to send a request to modify the LCD if the global variable outputChangesMade is true.
*   This function will send a digital high signal to pin (PB11) if and only if the timer is currently in Alarm Mode.  Otherwise, it will turn off the signal.
*   This function will iterate over each line of the LCD output display and pull the corresponding string that is assigned to be displayed in the present state of the timer mode, as defined by the modeLCDvalues matrix.
*/
void populateLcdOutput(){
    if(!outputChangesMade) return;      //minimize display signals by only refreshing when output changes have been made
    outputChangesMade = false;

    //update output LED state for alarm mode condition:
    if(timerMode == AlarmMode){
        GPIOB->ODR |= 0x800;                       //send signal High to pin PB11 to indicate that the alarm is going off
    }else{
        GPIOB->ODR &= ~(0x800);                       //send signal Low  to pin PB11 to indicate that the alarm is off
    }

    //refresh each line of the LCD display
    for(char line = 0; line < ROW; line++){
        char* printVal = modeLCDvalues[timerMode + line];   //retrieve the string associated with the current line of the LCD
        lcdObject.setCursor(0, line);                       //reset cursor to position 0 of the line to be written to
        lcdObject.print(printVal);                          //send a print request to configure the text of the line
    }
}


/**
* void tickCountdownTimer
* 
* Summary of the function:
*  This function counts down the output string of the timer by 1 when the system is in Countdown Mode
*
* Parameters:   None
*
* Return value: None
*
* Outputs:      None
*
* Description:  
*   This function is called by the countdownTicker.  It will only modify the remaining time in Countdown Mode.  
*   If called outside of Countdown Mode, the function will instead disable the ticker that is responsible for calling it in order to save resources.
*   This function will create changes that will require a new execution of populateLcdOutput in order to be populate to the LCD.
*   This function will attempt to decrement the seconds, then tens of seconds, and finally minutes until it finds a character with a value greater than '0'.
*   When decrementing higher significance digits, all less significant digits will be reset to their maximum value.
*   This function will switch the timer mode to Alarm Mode if and only if the countdown mode remaining time is 0:00
*/
void tickCountdownTimer(){
    //only make any changes if the timer is in countdown mode.  If the timer is not in countdown mode, 
    //disable the ticker responsible for calling this function
    if(timerMode != CountdownMode) {
        countdownTicker.detach();
        return;
    }
    outputChangesMade = true;

    //if the seconds digit is greater than 0, decrement it and exit the function
    if(modeLCDvalues[CountdownMode + 1][CountdownSecondsIndex] > '0'){
        modeLCDvalues[CountdownMode + 1][CountdownSecondsIndex]--;
        return;
    }

    //if the tens of seconds digit is greater than 0, decrement it, set seconds to 9, and exit the function
    if(modeLCDvalues[CountdownMode + 1][Countdown10SecondsIndex] > '0'){
        modeLCDvalues[CountdownMode + 1][Countdown10SecondsIndex]--;
        modeLCDvalues[CountdownMode + 1][CountdownSecondsIndex] = '9';
        return;
    }

    //if the minutes digit is greater than 0, decrement it,  set the seconds count to 59, and exit the function
    if(modeLCDvalues[CountdownMode + 1][CountdownMinutesIndex] > '0'){
        modeLCDvalues[CountdownMode + 1][CountdownMinutesIndex]--;
        modeLCDvalues[CountdownMode + 1][Countdown10SecondsIndex] = '5';
        modeLCDvalues[CountdownMode + 1][CountdownSecondsIndex]   = '9';
        return;
    }

    //if none of the above conditions caused the function to exit, the time must be 0:00
    //switch the timer to the Alarm Mode
    timerMode = AlarmMode;
}

/**
* void tickBounceHandler
* 
* Summary of the function:
*  This function counts down the bounce lockout in order to prevent duplicate events from being generated due to a single button press.
*
* Parameters:   None
*
* Return value: None
*
* Outputs:      None
*
* Description:  
*   The global variable bounceLockout will cause button presses to be ignored when it is set to a value greater than 0
*   bounceLockout will not be decremented for values less than zero to avoid an accidental lockout caused by an integer underflow 
*/
void tickBounceHandler(){
    if(bounceLockout > 0)
        bounceLockout--;
}
