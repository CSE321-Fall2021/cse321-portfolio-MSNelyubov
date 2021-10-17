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
#define CountdownMode 0x1
#define StoppedMode   0x2
#define AlarmMode     0x3

//Input Duration string index values correspoinding to indexes in the string array modeLCDvalues
#define DurationInputMinutesIndex 11
#define DurationInput10SecondsIndex 13
#define DurationInputSecondsIndex 14

//TODO: create interface to output LCD
//CSE321_LCD lcdObject(COL,ROW);


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

//declare general handler for Matrix keypad input events
void handleMatrixButtonEvent(int isRisingEdgeInterrupt,int column, int row);

//injection point for the controller to handle the input with respect to the system state
void handleInputKey(char inputKey);


/****************************
  *    Global  Variables    *
  ***************************/

int timerMode;              //The timer mode defines what behavior will be undertaken due to a given keypad input

int inputModeIndex = 0;         //the position of the input character
char inputString[] = "m:ss";    //default input string, modified during the input mode
int countdownStartValue = -1;   //countdown start value: how long the timer should run for, in seconds, just afer starting up.  Default to -1 as "no input received" state

int row = 0;                //the row currently being supplied a non-zero voltage
int logLine = 0;            //debugging utility to notify how many lines have been printed for understanding otherwise identical output


int focusInputPositions[] = {
    DurationInputSecondsIndex, 
    DurationInput10SecondsIndex, 
    DurationInputMinutesIndex
};
int focusInputPositionsIdx = 0;  //the position of the input value index that is currently being modified (if in Input Mode)

int buttonPressed = 0;      //boolean for if a keypad number that is currently live has been pressed down.  Used to halt row oscillation until it is opened.
char charPressed = '\0';    //the character on the input matrix keypad which is currently pressed down.  Defaults to '\0' when no key is pressed.
                            //undefined behavior when more than one key is pressed at the same time

// MatrixDim + 1 used as second dimension because of null terminator in each string
char keyValues[][MatrixDim + 1] = {"dcba","#963","0852","*741"};

//LCD output matrix. Access strings as modeLCDvalues[definedMode + LcdLineIndex]
char modeLCDvalues[8][COL] = {
    " Input Duration"," of timer: 1:11",      //Input Mode     LCD Output String
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


int main() {
    RCC->AHB2ENR |= 0x4;    //enable RCC for GPIO C

    GPIOC->MODER |= 0x550000;       //configugure GPIO pins PC8,PC9,PC10,PC11
    GPIOC->MODER &= ~(0xAA0000);    //as outputs

    rowLL.rise(&rising_isr_abc);   //assign interrupt handler for a rising edge event from the column containing buttons a,b,c,d
    rowCL.rise(&rising_isr_369);   //assign interrupt handler for a rising edge event from the column containing buttons 3,6,9,#
    rowCR.rise(&rising_isr_258);   //assign interrupt handler for a rising edge event from the column containing buttons 2,5,8,0
    rowRR.rise(&rising_isr_147);   //assign interrupt handler for a rising edge event from the column containing buttons 1,4,7,*

    rowLL.fall(&falling_isr_abc);   //assign interrupt handler for a falling edge event from the column containing buttons a,b,c,d
    rowCL.fall(&falling_isr_369);   //assign interrupt handler for a falling edge event from the column containing buttons 3,6,9,#
    rowCR.fall(&falling_isr_258);   //assign interrupt handler for a falling edge event from the column containing buttons 2,5,8,0
    rowRR.fall(&falling_isr_147);   //assign interrupt handler for a falling edge event from the column containing buttons 1,4,7,*


    timerMode = InputMode;          //begin execution with the timer in Input Mode

    printf("\n\n== Initialized ==\n");

    while (1) {

        //supply voltage to one output row at a time
        switch (row){
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

        thread_sleep_for(5);              //maintain power to the row for a brief period of time to account for bounce

        //proceed to scanning the next input if and only if there is no closed loop in the current scan set
        if(!buttonPressed){
            GPIOC->ODR &= ~(0xF00);         //reset voltage to output 0 on all pins

            thread_sleep_for(1);           //wait to give any falling edge triggers a chance to resolve before proceeding

            row++;                          //update the row target to poll the next row
            row%=4;
        }
    }

    return 0;
}


//The following set of functions are used as handlers for the rising and falling edge behaviors of keypad buttons.
//All of the following eight functions serve to configure the inputs for handleMatrixButtonevent().
void rising_isr_abc(void) {handleMatrixButtonEvent(RisingEdgeInterrupt,  ColABC, row);}
void falling_isr_abc(void){handleMatrixButtonEvent(FallingEdgeInterrupt, ColABC, row);}
void rising_isr_369(void) {handleMatrixButtonEvent(RisingEdgeInterrupt,  Col369, row);}
void falling_isr_369(void){handleMatrixButtonEvent(FallingEdgeInterrupt, Col369, row);}
void rising_isr_258(void) {handleMatrixButtonEvent(RisingEdgeInterrupt,  Col258, row);}
void falling_isr_258(void){handleMatrixButtonEvent(FallingEdgeInterrupt, Col258, row);}
void rising_isr_147(void) {handleMatrixButtonEvent(RisingEdgeInterrupt,  Col147, row);}
void falling_isr_147(void){handleMatrixButtonEvent(FallingEdgeInterrupt, Col147, row);}


/**
*  This function converts an input event type, row, and column received 
*    from an ISR handler into the character that is represented by that 
*    button press and stores that character to the global variable charPressed.   
*/
void handleMatrixButtonEvent(int isRisingEdgeInterrupt,int column, int row){
    buttonPressed = isRisingEdgeInterrupt;          //a rising edge interrupt occurs when a button is pressed.  
                                                    //These are two separate variables because buttonPressed is global to control input polling.

    if(isRisingEdgeInterrupt){
        charPressed = keyValues[column][row];               //fetch the char value associated with the index that was retrieved
        printf("key pressed: %c. log line: %d\n",charPressed, logLine++);
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

    if(timerMode == InputMode){     //handle button behaviors when the system is in input mode

        return;
    }

    if(timerMode == CountdownMode){

        return;
    }

    if(timerMode == StoppedMode){

        return;
    }

    if(timerMode == AlarmMode){

        return;
    }
}
