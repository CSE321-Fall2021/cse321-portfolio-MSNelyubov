/******************************************************************************
*   File Name:      cse321_project2_mnelyubo_main.cpp
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   10/10/2021
*   Last Modified:  10/15/2021
*   Purpose:        Extract input from a 4x4 keypad to act as the source of user 
*                     input to control a timer that will display its current
*                     mode behavior through an LCD. 
*
*   Functions:      
*               handleMatrixButtonEvent
*               handleInputKey
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
*   Outputs:        Serial output, LCD display (TBD), 2 LED channels (TBD)
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

//output to lCD
void populateLcdOutput();   //TODO: handle LED outputs relevant for the mode as well

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

int timerMode = InputMode;              //The timer mode defines what behavior will be undertaken due to a given keypad input. Begin the timer in Input Mode.

int bounceLockout = 0;      //set to bounceTimeoutWindow when a button is pressed down.  Ticks down once a millisecond.  Positive values indicate a button press is likely a duplicate.
int keypadVccRow = 0;       //the row currently being supplied a non-zero voltage to scan for user input
int logLine = 0;            //debugging utility to notify how many lines have been printed for understanding otherwise identical output

int outputChangesMade = true;      //flag to indicate if the output to the LCD needs to be refreshed.  Start at 1 to populate the initial display

int buttonPressed = false;      //boolean for if a keypad number that is currently live has been pressed down.  Used to halt row oscillation until it is opened.
char charPressed = '\0';    //The character on the input matrix keypad which is currently pressed down.  The variable defaults to '\0' when no key is pressed.
                            //system behavior is UNDEFINED when more than one key is pressed at the same time

// MatrixDim + 1 used as second dimension because of null terminator in each string
char keyValues[][MatrixDim + 1] = {"dcba","#963","0852","*741"};

//LCD output matrix. Access strings as modeLCDvalues[definedMode + LcdLineIndex]
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

InterruptIn rowLL(PC_0);    //declare the connection to pin PC_0 as a source of input interrupts, connected to the far left column of the matrix keypad
InterruptIn rowCL(PC_3);    //declare the connection to pin PC_3 as a source of input interrupts, connected to the center left column of the matrix keypad
InterruptIn rowCR(PC_1);    //declare the connection to pin PC_1 as a source of input interrupts, connected to the center right column of the matrix keypad
InterruptIn rowRR(PC_4);    //declare the connection to pin PC_4 as a source of input interrupts, connected to the far right column of the matrix keypad

Ticker countdownTicker;     //create a ticker that counts down the timer once per second
Ticker bounceHandlerTicker; //create a ticker that handles input lockout to mitigate bounce

int main() {
    RCC->AHB2ENR |= 0x4;    //enable RCC for GPIO C

    GPIOC->MODER |= 0x550000;       //configugure GPIO pins PC8,PC9,PC10,PC11
    GPIOC->MODER &= ~(0xAA0000);    //  as outputs

    rowLL.rise(&rising_isr_abc);   //assign interrupt handler for a rising edge event from the column containing buttons a,b,c,d
    rowCL.rise(&rising_isr_369);   //assign interrupt handler for a rising edge event from the column containing buttons 3,6,9,#
    rowCR.rise(&rising_isr_258);   //assign interrupt handler for a rising edge event from the column containing buttons 2,5,8,0
    rowRR.rise(&rising_isr_147);   //assign interrupt handler for a rising edge event from the column containing buttons 1,4,7,*

    rowLL.fall(&falling_isr_abc);   //assign interrupt handler for a falling edge event from the column containing buttons a,b,c,d
    rowCL.fall(&falling_isr_369);   //assign interrupt handler for a falling edge event from the column containing buttons 3,6,9,#
    rowCR.fall(&falling_isr_258);   //assign interrupt handler for a falling edge event from the column containing buttons 2,5,8,0
    rowRR.fall(&falling_isr_147);   //assign interrupt handler for a falling edge event from the column containing buttons 1,4,7,*

    lcdObject.begin();              //initialize LCD
    populateLcdOutput();            //populate initial LCD text

    countdownTicker.attach(&tickCountdownTimer, 1s);        //Attach the function that operates the Countdown mode to the ticket that triggers it once per second
    bounceHandlerTicker.attach(&tickBounceHandler, 1ms);    //Attach the function that ticks down milliseconds for the bounce lockout to eliminate duplicate events

    printf("\n\n== Initialized ==\n");

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
*  This function converts an input event type, row, and column received 
*    from an ISR handler into the character that is represented by that 
*    button press and stores that character to the global variable charPressed.   
*/
void handleMatrixButtonEvent(int isRisingEdgeInterrupt, int column, int row){
    buttonPressed = isRisingEdgeInterrupt;          //a rising edge interrupt occurs when a button is pressed.  
                                                    //These are two separate variables because buttonPressed is global to control input polling.

    if(isRisingEdgeInterrupt){

        //prevent duplicate event creation if another button press was just detected
        if(bounceLockout>0){
            printf("ll:%d Warning: potential duplicate input detected.  Ignoring.\n", logLine++);
            return;
        }


        bounceLockout = bounceTimeoutWindow;       //lock bounce 'mutex' after event is triggered

        charPressed = keyValues[column][row];               //fetch the char value associated with the index that was retrieved
        printf("ll:%d key pressed: %c.\n",logLine++, charPressed);
        handleInputKey(charPressed);
    }else{
        charPressed = '\0';                                 //reset the char value to '\0' as the key has been released
    }
}


/**  
*  This function handles the keypad input with respect to how that input 
*    affects the state of the system (as described in the design document) 
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
*  This function sets the Countdown timer to its initial value.
*  This function should be run when the A key is pressed.
*/
void switchToCountdownMode() {
    timerMode = CountdownMode;      //set the timer to countdown mode

    //push the input mode duration to the Countdown Mode string
    modeLCDvalues[CountdownMode + 1][CountdownMinutesIndex]   = modeLCDvalues[InputMode + 1][DurationInputMinutesIndex];   //inherit minutes from latest input
    modeLCDvalues[CountdownMode + 1][Countdown10SecondsIndex] = modeLCDvalues[InputMode + 1][DurationInput10SecondsIndex]; //inherit 10's of seconds from latest input
    modeLCDvalues[CountdownMode + 1][CountdownSecondsIndex]   = modeLCDvalues[InputMode + 1][DurationInputSecondsIndex];   //inherit seconds from latest input
}


/**  
*  This function handles the LCD output data flow based on the modeLCDvalues string array.
*  This function can't be called from an InterruptIn or basic Ticker context.
*/
void populateLcdOutput(){
    if(!outputChangesMade) return;      //minimize display signals by only refreshing when output changes have been made
    outputChangesMade = false;
    //refresh each line of the LCD display
    for(char line = 0; line < ROW; line++){
        char* printVal = modeLCDvalues[timerMode + line];

        printf("ll:%d Attempting to append [%s] to line %d\n", logLine++, printVal, line);
        lcdObject.setCursor(0, line);                       //reset cursor to position 0 of the current line
        lcdObject.print(printVal);                          //sent print request to configure the text of the line
    }
}

/**  
*  This function counts down the output string of the timer by 1 when the system
*    is in Countdown Mode
*/
void tickCountdownTimer(){
    if(timerMode != CountdownMode) return;
    outputChangesMade = true;

    //if the seconds digit is greater than 0, decrement it and exit the function
    if(modeLCDvalues[CountdownMode + 1][CountdownSecondsIndex] > '0'){
        modeLCDvalues[CountdownMode + 1][CountdownSecondsIndex]--;
        return;
    }

    //if the tens of seconds digit is greater than 0, decrement it, set seconds to 9, and exit the function
    if(modeLCDvalues[CountdownMode + 1][Countdown10SecondsIndex] > '0'){
        modeLCDvalues [CountdownMode + 1][Countdown10SecondsIndex]--;
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
*  This function regularly counts down the bounce lockout in order to prevent duplicate event entries
*/

void tickBounceHandler(){
    bounceLockout--;
}