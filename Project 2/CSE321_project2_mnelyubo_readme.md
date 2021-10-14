# About
Project Description:

This project tracks the design and development of a countdown timer which can be programmed by its user through a peripheral keypad and displays output text with an LCD display.  The project contains the following files:

- CSE321_project2_mnelyubo_main.cpp is the C++ file which implements the core functionality of the countdown timer
- CSE321_project2_stage2_part1_mnelyubo.pdf provides a high-level overview of the design of the microcontroller behavior
- 1802.cpp and 1802.h are the library files for interacting with the output LCD


Contribitor List:
- Misha Nelyubov (mnelyubo@buffalo.edu)
- Dr. Jennifer Winikus (jwinikus@buffalo.edu)


# Features
The main behavior of this code controls a Nucelo L4R5ZI to
- act as a countdown timer with set time, count down, alarm, and stop modes
- take timer configuration inputs from a 4x4 matrix keypad
- output the current state of the program to an external LCD monitor


# Required Materials
- Nucleo L4R5ZI
    - Hardware on which to execute code
- 4x4 Matrix keypad
    - input for microcontroller
- LCD (JHD1804 recommended)
    - output display for microcontroller
- LEDs (4)
    - Two LEDs are sufficient to repersent all necessary outputs, but four are recommended.
- USB 2.0 A to USB 2.0 Micro B cable
    - Interface between computer and Nucleo
- Mbed Studio (https://os.mbed.com/studio/)
    - IDE to develop and deploy code


# Resources and References
- https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

# Getting Started
TODO

# CSE321_project2_mnelyubo_main.cpp:
This program takes inputs from a 4x4 matrix keypad to control a timer.  The timer mode and input/remaining
time are output to a connected LCD (WIP).

The main function of the program initializes the system by configuring a set of four GPIO pins as
outputs to supply voltage to the keypad, assigning event handlers to the rising and falling edges 
of input interrupts that are connected to the keypad, and setting the initial timer mode to input.

After the initialization is complete, the main function cycles through the four output channels, 
providing power to one channel at a time and proceeding to the next input after a short time interval 
in which no input keystroke is detected.  This allows a fast response time to any of the 16 keys and 
eliminates the opportunity for duplicate inputs to be detected due to a single key press.


## Global Declarations

## API and Built-In Elements Used
- mbed.h
    - InterruptIn objects (4) used to detect button presses on the matrix keypad
- 1802.h
    - Interface to LCD
- cstdio
    - Used to printf from interrupts as a temporary output channel until the LCD is configured
- ctime
- string


## Custom Functions

- Interrupt Handlers:
    - The following set of functions are used as handlers for the rising and falling edge behaviors of keypad buttons.
        - rising_isr_abc
        - rising_isr_369
        - rising_isr_258
        - rising_isr_147
        - falling_isr_abc
        - falling_isr_369
        - falling_isr_258
        - falling_isr_147
    - Triggers:
        - The rising or falling edge of the respective column, corresponding to the name of the function.
    - Inputs:
        - None
    - Outputs:
        - None
    - Global variables accessed:
        - row
    - Global variables modified:
        - None
    - Functions called:
        - handleMatrixButtonEvent()
            - Input parameters to this function are whether or not the interrupt event was a rising or falling edge, the column of the event (interrupt handler function specific), and the row of the event (global variable)


- handleMatrixButtonEvent
    This function converts an input event type, row, and column received 
    from an ISR handler into the character that is represented by that 
    button press and stores that character to the global variable charPressed.
    - Inputs: 
        - int isRisingEdgeInterrupt - whether the button is pressed down (1) or released (0)
        - int column - the matrix keypad column in which the button press was detected
        - int row - the matrix keypad row which is currently being supplied power
    - Outputs: 
        - None
    - Global variables accessed:
        - row
        - buttonPressed
        - charPressed
    - Global variables modified:
        - buttonPressed is set to equal the input isRisingEdgeInterrupt
        - charPressed is set to equal the value of the pressed key if the event is a rising edge interrupt
    - Functions called:
        - handleInputKey()
            - Input parameters to this function are the key that was just input
            - This function is only called if the input isRisingEdgeInterrupt is true


- handleInputKey
    This function handles the keypad input with respect to how that input 
    handles the state of the system (as described in the design document) 
    - Inputs:
        - char inputKey
    - Outputs: 
        - None
    - Global variables accessed:
        - timerMode
        - inputString
    - Global variables modified:
        - timerMode may be modified due to certain input keystrokes depending on the present mode
        - inputString may be modified due to numeric inputs when in the input mode
    - Functions called:
        - TBD















*   Inputs:         4x4 matrix array input buttons
*
*   Outputs:        Serial output, LCD display (TBD)
*
*   References:     
*               
*
*
******************************************************************************/
#include
#include
#include
#include
#include

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
#define InputMode     0x1
#define CountdownMode 0x2
#define StoppedMode   0x4
#define AlarmMode     0x8







//injection point for the controller to handle the input with respect to the system state
void handleInputKey(char inputKey);

int timerMode = InputMode;  //begin execution with the timer in Input Mode

int inputModeIndex = 0;         //the position of the input character
char inputString[] = "m:ss";    //default input string, modified during the input mode
int countdownStartValue = -1;   //countdown start value: how long the timer should run for, in seconds, just afer starting up.  Default to -1 as "no input received" state

int row = 0;                //the row currently being supplied a non-zero voltage
int logLine = 0;            //debugging utility to notify how many lines have been printed for understanding otherwise identical output

int buttonPressed = 0;      //boolean for if a keypad number that is currently live has been pressed down.  Used to halt row oscillation until it is opened.
char charPressed = '\0';    //the character on the input matrix keypad which is currently pressed down.  Defaults to '\0' when no key is pressed.
                            //undefined behavior when more than one key is pressed at the same time

// MatrixDim + 1 used as second dimension because of null terminator in each string
char keyValues[][MatrixDim + 1] = {"dcba","#963","0852","*741"};

InterruptIn rowLL(PC_0);    //declare the connection to pin PC_0 as a source of input interrupts, connected to the far left column
InterruptIn rowCL(PC_3);    //declare the connection to pin PC_3 as a source of input interrupts, connected to the center left column
InterruptIn rowCR(PC_1);    //declare the connection to pin PC_1 as a source of input interrupts, connected to the center right column
InterruptIn rowRR(PC_4);    //declare the connection to pin PC_4 as a source of input interrupts, connected to the far right column
