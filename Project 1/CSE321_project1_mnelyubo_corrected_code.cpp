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
*   References:     N/A
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
InterruptIn userButton1(BUTTON1);   //establish interrupt handler from 
                                      //Button 1 input (B1 USER)


//internal variables

//Indicates the state of the userButton1 input. 
//0 -> button is not pressed
//1 -> button is pressed
int buttonPressed  = 0;

//Active low indicator for whether or not a new cycle of
//turning the LED on and off should be undertaken. 
//0 -> begin a new cycle
//1 -> do not begin a new cycle
int oscillateLED_L = 0;


int main() {
    // start the allowed execution of the thread
    printf("----------------START----------------\n");
	printf("Starting state of thread: %d\n", controller.get_state());
    controller.start(cycleLedState);
	printf("State of thread right after start: %d\n", controller.get_state());
    
    //map handlers for when the button is pressed and released
    userButton1.rise(button1PushDownBehavior);
	userButton1.fall(button1OpenBehavior);
    return 0;
}


// 
void cycleLedState() {
    while (true) {
        if(oscillateLED_L==0){
            outputLED = !outputLED;
            printf("LED switched to state: HIGH\tu %d\t z %d\r\n", oscillateLED_L, buttonPressed); //you do need to update the print statement to be correct
            thread_sleep_for(2000); //Thread_sleep is a time delay function, causes a 2000 unit delay
            outputLED = !outputLED;
            printf("LED switched to state: low \tu %d\t z %d\r\n", oscillateLED_L, buttonPressed);
            thread_sleep_for(500); //Thread_sleep is a time delay function, causes a 500 unit delay
        }
    }
}


// this function is only triggered upon button 1 being pressed
// this function enables the behavior of button1OpenBehavior
void button1PushDownBehavior() {
    buttonPressed=1;
}

void button1OpenBehavior() {
    if (buttonPressed==1){
        oscillateLED_L++; 
        oscillateLED_L %= 2;
        buttonPressed=0;
    }
}