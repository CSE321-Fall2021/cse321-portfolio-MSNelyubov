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
The main behavior of this code controls a Nucleo L4R5ZI to
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
6. Connect the Nucleo to the input and output peripherals through the breadboard
    - connect power supply lanes
        - connect a Nucleo pin labeled GND to the - bus of the breadboard
        - connect a Nucleo pin labeled 5V  to the + bus of the breadboard
    - connect input source pins
        - connect the Nucleo pin labeled A1(PC0) to pin 1 of the keypad via jumper wire.
        - connect the Nucleo pin labeled A2(PC3) to pin 2 of the keypad via jumper wire.
        - connect the Nucleo pin labeled A3(PC1) to pin 3 of the keypad via jumper wire.
        - connect the Nucleo pin labeled A4(PC4) to pin 4 of the keypad via jumper wire.

        - connect a 1k resistor in between the connection from pin A1 to keypad 1 and the ground bus.
            - this serves to provide a reference to the common ground for the signal
            - add one 1k resistor for each of the four pins

        - connect the Nucleo pin labeled PC8  in the pin diagram in sector CN8 to pin 5 of the keypad via jumper wire.
        - connect the Nucleo pin labeled PC9  in the pin diagram in sector CN8 to pin 6 of the keypad via jumper wire.
        - connect the Nucleo pin labeled PC10 in the pin diagram in sector CN8 to pin 7 of the keypad via jumper wire.
        - connect the Nucleo pin labeled PC11 in the pin diagram in sector CN8 to pin 8 of the keypad via jumper wire.

    - connect LCD display pins
        - connect the - lane of the breadboard to the LCD pin labeled GND using the LCD cable and a jumper wire.
        - connect the + lane of the breadboard to the LCD pin labeled VCC using the LCD cable and a jumper wire.
        - connect the Nucleo pin labeled SDA / D14 to the LCD pin labeled SDA using the LCD cable and a jumper wire.
        - connect the Nucleo pin labeled SCL / D15 to the LCD pin labeled SCL using the LCD cable and a jumper wire.

	- connect "input detected" indicator LED
        - connect the Nucleo pin labeled PB10 in the pin diagram in sector CN10 to the anode of the indicator LED.
		- connect the cathode of the indicator LED to a 1k resistor.
		- connect the other end of the 1k resistor to the common ground.
	
	- connect alarm LEDs
		- connect the Nucleo pin labeled PB11 in the pin diagram in sector CN10 to the anodes of three LEDs.
		- connect the cathodes of each LED to a different 1k resistor.
		- connect the free end of each 1k resistor to the common ground.
		- this should result in three branches consisting of one LED and one resistor each connected in parallel between pin PB11 and ground.
7. Click on the play button labeled "Run Program" to begin executing the code on the Nucleo


# CSE321_project2_mnelyubo_main.cpp:
This program takes inputs from a 4x4 matrix keypad to control a timer.
Timer mode-based text and input/remaining time are output to a connected LCD.

The timer is configured to have the following four modes:
- Input Mode
    - Entered by pressing the D key and during system startup.
    - The user can press numeric inputs on the number pad to configure the duration of the timer up to 9 minute and 59 seconds.
    - The LCD will display the currently input time.
    - For time durations less than 9 minutes and 59 seconds, inputs of more than 60 seconds will be accepted (e.g. 0:77 will function the same as 1:17).

- Countdown Mode
    - Started by pressing the A key.
    - The LCD will display the remaining timer duration.
    - Upon ticking down to 0:00, the timer will automatically switch to Alarm Mode.

- Stopped Mode
    - Entered by pressing the B key during the Countdown or Alarm modes.
    - Clears the value of an ongoing countdown and shuts down alarm notifications.
    - The LCD will display "Timer stopped".

- Alarm Mode
    - The system will automatically switch to this mode from the Countdown mode when the countdown timer reaches 0 seconds remaining.
    - The LCD will dysplay "Times up".
    - A single pin signal () will be sent high, instructing multiple LEDs to be turned on.
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
    - This value controls the mode of the timer.
    - This value is used to determine which behaviors to perform when a key is pressed.

- int bounceLockout
    - This value is set to a defined (bounceTimeoutWindow) quantity of milliseconds when a button is pressed down.
    - This value is ticked down every millisecond by the function tickBounceHandler.
    - If the variable is accessed by handleMatrixButtonEvent while the value is positive, this indicates that a button press is most likely a duplicate.

- int keypadVccRow
    - This value indicates the only row that is to be supplied power.
    - This is used to determine which keypad input was pressed in the function handleMatrixButtonEvent().

- int outputChangesMade
    - This (boolean) variable indicates if the output to the LCD needs to be refreshed.
    - The initial value is 1 to populate the display during startup.


- int buttonPressed
    - This (boolean) variable indicates if any button is currently pressed.
    - A value of 1 means some button is pressed and 0 means no button is pressed.
    - The polling of rows will be halted as long as this value is true.
    - This is not guaranteed to be correct if multiple buttons are pressed at the same time.

- char charPressed
    - This variable contains the ASCII character representation of the button that is currently being pressed.  If no button is being pressed, it will contain an ASCII value of '\0', the null terminator of a string.  
    - This is not guaranteed to be correct if multiple buttons are pressed at the same time.

- char[][] keyValues
    - This two-dimensional array contains the ASCII character values associated with each key in the matrix.  It can be accessed with the index of a triggered input column and currently powered row to determine the ASCII value associated with that button.


## API and Built-In Elements Used
- mbed.h
    - InterruptIn objects (4) used to detect button presses on the matrix keypad
        - colLL (far left keypad column)
        - colCL (center left keypad column)
        - colCR (center right keypad column)
        - colRR (far right keypad column)
    - Ticker objects (2) used to execute regularly scheduled events
        - countdownTicker (counts down timer clock)
        - bounceHandlerTicker (counts down bounce lockout)
- 1802.h
    - Used to create CSE321_LCD lcdObject (16 Columns, 2 Rows) to control LCD
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
    - This function converts an input event type, row, and column received from an ISR handler into the character that is represented by that button press.
    - The function filters for duplicate and conflicting inputs.
    - Clean inputs will have that input character stored in the global variable charPressed.
    - Inputs: 
        - int isRisingEdgeInterrupt - whether the button is pressed down (1) or released (0)
        - int column - the matrix keypad column in which the button press was detected
        - int row - the matrix keypad row which is currently being supplied power
    - Outputs: 
        - None
    - Global variables accessed:
        - row (global variable keypadVccRow)
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
    - This function modifies the mode or duration of the timer based on the specified input to the system.  
    - The action taken as the result of any input depends upon the present mode of the system.
    - Inputs:
        - char inputKey
    - Outputs: 
        - None
    - Global variables accessed:
        - timerMode
        - modeLCDvalues
    - Global variables modified:
        - timerMode may be modified depending on the input and present mode.
        - The second line of the input mode string in the modeLCDvalues matrix may be modified to represent a different time.
    - Functions called:
        - switchToCountdownMode
            - called when the button A is pressed in certain modes

- switchToCountdownMode
    - This function sets the system to the Countdown timer and sets its initial value to match the input.
    - This function is to be run when the A key is pressed.
    - Inputs:
        - None
    - Outputs: 
        - The LCD output text is configured, but not instructed to display.
    - Global variables accessed:
        - timerMode
        - modeLCDvalues
        - countdownTicker
    - Global variables modified:
        - timerMode is set to CountdownMode.
        - countdownTicker is reattached by this function call in order to synchronize ticks with the button press.
        - The second line of the modeLCDvalues matrix for the countdown mode will be modified to represent the input time.
    - Functions called:
        - tickCountdownTimer

- populateLcdOutput
    - This funciton will only attempt to send a request to modify the LCD if the global variable outputChangesMade is true.
    - This function handles the LCD output data flow based on the modeLCDvalues string array.
    - This function controls the output state of the alarm LED signal.
    - Modification of LCD output can't be successfully executed from an InterruptIn or basic Ticker call.
    - Inputs:
        - None
    - Outputs: 
        - The LCD output is set based on the current timer Mode and (if in countdown mode) elapsed time.
        - Pin 11 from port B will be set to high if the timer is currently in alarm mode. Otherwise, it will be turned off.
    - Global variables accessed:
        - outputChangesMade
        - timerMode
        - modeLCDvalues
        - lcdObject
    - Global variables modified:
        - outputChangesMade will be set to false if the body of the function is to be executed
        - The second line of the modeLCDvalues matrix for the countdown mode will be modified to represent the input time.
    - Functions called:
        - None

- tickCountdownTimer
    - This function is called by the countdownTicker.
    - This function will only modify the remaining time in Countdown Mode.
    - If called outside of Countdown Mode, this function will instead disable the ticker that is responsible for calling it in order to save resources.
    - This function counts down the remaining time of the timer by 1 second when the system is in Countdown Mode.
    - This funciton will switch the timer mode to Alarm Mode when the countdown timer is 0:00.
    -Inputs:
        - None
    - Outputs: 
        - None
    - Global variables accessed:
        - outputChangesMade
        - timerMode
        - modeLCDvalues
        - countdownTicker
    - Global variables modified:
        - outputChangesMade will be set to true if the timer mode is currently CountdownMode.
        - The second line of the modeLCDvalues matrix for the countdown mode will be modified to represent the remaining time.
        - timerMode will be set to AlarmMode when the countdown timer reaches 0:00.
        - countdownTicker will be dettached if the funciton is called while not in Countdown Mode.
    - Functions called:
        - None

- tickBounceHandler
    - This function counts down the bounce lockout in order to prevent duplicate events from being generated due to a single button press.
    - Inputs:
        - None
    - Outputs: 
        - None
    - Global variables accessed:
        - bounceLockout
    - Global variables modified:
        - bounceLockout is decremented if it is greater than 0
    - Functions called:
        - None