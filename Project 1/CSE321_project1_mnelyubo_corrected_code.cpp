/******************************************************************************
*   File Name:      CSE321_project1_mnelyubo_corrected_code.cpp
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   09/23/2021
*   Last Modified:  09/26/2021
*   Purpose:        
*       This program creates a thread to operate an LED.  Default behavior 
*         without usage of inputs will cause the LED to cycle between ON and OFF
*         states with the ON state lasting 2000 ms and the OFF state lasting 
*         500 ms.
*       Pressing and releasing Button 1 will toggle the board between states of
*         continuing to cycle the LED and pausing at the end of the current 
*         cycle until the button is pressed and released again.  The next cycle
*         will begin immediately after Button 1 is released while the board is
*         paused.
*
*   Functions:      
*
*   Assignment:     CSE 321 Project 1, Fall 2021
*
*   Inputs:         Blue User button (BUTTON1)
*
*   Outputs:        Blue LED  (LED2)
*
*   Constraints:    N/A
*
*   References:     
*                   https://www.cs.utah.edu/~germain/PPS/Topics/commenting.html
*
******************************************************************************/


#include "mbed.h"


Thread controller;  // Create a thread to drive an LED to have an 
                    //on time of 2000 ms and off time of 500 ms


//declare internal functions
void cycleLedState();
void button1PushDownBehavior();
void button1OpenBehavior();


//initialize I/O
DigitalOut outputLED(LED2);          //establish blue led as an output
InterruptIn userButton1(BUTTON1);    //establish interrupt handler from Button 1 input (B1 USER)


//internal variables

//Active high indicator for the state of the userButton1 input. 
//0 -> button is not pressed
//1 -> button is pressed
int buttonPressed  = 0;

//Active low indicator for whether or not a new cycle of
//turning the LED on and off should be undertaken. 
//0 -> begin a new cycle
//1 -> do not begin a new cycle
//The system will begin in a blinking state
int oscillateLED_L = 0;


int main() {
    // start the execution of the LED cycling thread
    printf("----------------START----------------\n");
	printf("Starting state of thread: %d\n", controller.get_state());
    controller.start(cycleLedState);
	printf("State of thread right after start: %d\n", controller.get_state());
    
    //map handlers for when the button is pressed and released
    userButton1.rise(button1PushDownBehavior);
	userButton1.fall(button1OpenBehavior);
    return 0;
}


/**
* void cycleLedState ()
* 
* Summary of the function:
*    This function cycles the onboard LED between On and off states if oscillateLED_L is currently set to oscillate.
*
* Parameters:   None
*
* Return value: None
*
* Outputs:      The state of the blue LED is temporarily modified during executions of this function. 
*
* Description:  
*    The function spin tests until oscillateLED_L has a value of 0
*    Whenever oscillateLED_L is 0, the LED will switch on for 2000 ms then off for 500 ms.
*    After this cycle, the system resumes testing for the condition to begin the cycle again
*
*/
void cycleLedState() {
    while (true) {
        if(oscillateLED_L == 0){
            outputLED = !outputLED;
            printf("LED switched to state: HIGH\n");

            thread_sleep_for(2000); //Thread_sleep is a time delay function, causes a 2000 unit delay

            outputLED = !outputLED;
            printf("LED switched to state: low \n");

            thread_sleep_for(500); //Thread_sleep is a time delay function, causes a 500 unit delay
        }
    }
}


/**
* void button1PushDownBehavior ()
* 
* Summary of the function:
*    This function enables the next call to button1OpenBehavior to switch the state of the system between oscillating and resting.
*
* Parameters:   None
*
* Return value: None
*
* Description:
*    This function sets the state of buttonPressed to high in order to 
*    validate execution for the next instance of the function button1OpenBehavior.
*    Note: this value is not a mutex and may end up validating more than a single instance of button1OpenBehavior
*/
void button1PushDownBehavior() {
    buttonPressed=1;
}

/**
* void button1OpenBehavior ()
*
* Summary of the function:
*    This function is triggered at the end of an button click. 
*    This function toggles the state of whether or not the system output LED will continue to go through blinking cycles.
*
* Parameters:   None
*
* Return value: None
*
* Description:
*    This function will only make any changes if a button press event was first registered into the variable buttonPressed.
*    A mutex is NOT used to ensure that only a single modifying instance of this function can be triggered by a falling edge.
*    This function will flip the value of oscillateLED_L between 1 and 0 with each call to the function.
*    This function will clear the value of buttonPressed back to zero at the end of its execution.
*    The current cycle of LED blinking will complete before the effect of this function is observed.
*
*/
void button1OpenBehavior() {
    if (buttonPressed == 1){
        oscillateLED_L++; 
        oscillateLED_L%=2;
        buttonPressed=0;
    }
}