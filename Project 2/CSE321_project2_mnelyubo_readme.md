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
    - microcontroller on which to execute code and control inputs/outputs

- 4x4 Matrix keypad
    - input for microcontroller

- LCD (JHD1804 recommended)
    - output display for microcontroller

- LEDs (4)
    - Two LEDs are sufficient to repersent all necessary outputs, but four are recommended.

- Jumper Wires and breadboard
    - used to connect microcontroller with inputs and outputs

- USB 2.0 A to USB 2.0 Micro B cable
    - Interface between computer and Nucleo

- Mbed Studio (https://os.mbed.com/studio/)
    - IDE to develop and deploy code


# Resources and References
- https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf


# Getting Started
1. Clone the git repository locally
2. Open the repository with Mbed Studio
3. Select "Project 2" as the Active program in Mbed studio
4. Connect the Nucleo-L4R5ZI to your computer via USB cable
5. Select Nucleo-L4R5ZI as the Target in Mbed studio
6. (TODO: create schematic of all connections) Connect the Nucelo to the input and output peripherals through the breadboard
7. Click on the play button labeled "Run Program" to begin executing the code on the Nucleo


# CSE321_project2_mnelyubo_main.cpp:
This program takes inputs from a 4x4 matrix keypad to control a timer.  The timer mode and input/remaining
time are output to a connected LCD (WIP).

The timer is configured to have the following four modes:
- Input Mode
    - Entered by pressing the D key and during system startup.
    - The user can press numeric inputs on the number pad to configure the duration of the timer up to 9 minute and 59 seconds.
    - The LCD will display the currently input time.
    - For time durations less than 9 minutes and 59 seconds, inputs of more than 60 seconds will be accepted (e.g. 0:77 will function the same as 1:17).

- Countdown Mode
    - Started by pressing the A key.
    - The LCD will display the remaining timer duration.

- Stopped Mode
    - Entered by pressing the B key during the Countdown or Alarm modes.
    - Clears the value of an ongoing countdown and shuts down alarm notifications.
    - The LCD will display "Timer stopped"

- Alarm Mode
    - The system will automatically switch to this mode from the Countdown mode when the countdown timer reaches 0 seconds remaining.
    - The LCD will dysplay "Times up".
    - Multiple LEDs will be turned on.
    - Mode can be exited by pressing A, B, or D keys to switch to their corresponding mode, ending the alarm.

The main function of the program initializes the system by configuring a set of four GPIO pins as
outputs to supply voltage to the keypad, assigning event handlers to the rising and falling edges 
of input interrupts that are connected to the keypad, and setting the initial timer mode to input.

After the initialization is complete, the main function cycles through the four output channels, 
providing power to one channel at a time and proceeding to the next input after a short time interval 
in which no input keystroke is detected.  This allows a fast response time to any of the 16 keys and 
eliminates the opportunity for duplicate inputs to be detected due to a single key press.


## Global Declarations

- int timerMode
    - This value controls the mode of the timer and is used to determine which behaviors to perform when a key is pressed.

- int inputModeIndex
    - This value controls the position at which the next number of a duration will be stored in memory

- char[] inputString (TBD)
    - This value holds the current value of the user-input duration

- int countdownStartValue (TBD)
    - This value is the converted equivalent of the inputString into an integer quantity of seconds

- int row
    - This value indicates the only row that is to be supplied power.  This is used to determine which keypad input was pressed in the function handleMatrixButtonEvent().

- int logLine
    - This value is used with each serial print statement to distinguish identical outputs in the event of duplicate outputs.  It must be displayed and  incremented with each printf call.

- int buttonPressed
    - This variable indicates if any button is currently pressed.  A value of 1 means some button is pressed and 0 means no button is pressed.
    - This is not guaranteed to be correct if multiple buttons are pressed at the same time.

- char charPressed
    - This variable contains the ASCII character representation of the button that is currently being pressed.  If no button is being pressed, it will contain an ASCII value of '\0', the null terminator of a string.  
    - This is not guaranteed to be correct if multiple buttons are pressed at the same time.

- char[][] keyValues
    - This two-dimensional array contains the ASCII character values associated with each key in the matrix.  It can be accessed with the index of a triggered input column and currently powered row to determine the ASCII value associated with that button.


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


- handleInputKey (WIP)
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
