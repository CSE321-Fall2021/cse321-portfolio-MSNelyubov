/******************************************************************************
*   File Name:      
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   
*   Last Modified:  
*   Purpose:        This file is the standard template that will be used as the
*                     basis for all subsequent .cpp files for CSE321.
*
*   Functions:      
*
*   Assignment:     CSE 321 Project 1, Fall 2021
*
*   Inputs:         
*
*   Outputs:        
*
*   Constraints:    
*
*   External Sources:
*
******************************************************************************/

#include "mbed.h"

// Create a thread to drive an LED to have an on time of _______ms and off time
// _______ms

Thread controller;   //

void oscillateLED(); //someone has to right?

void button1PushDownBehavior();
void button1OpenBehavior();

DigitalOut fire(LED2);          //establish blue led as an output
InterruptIn cherish(BUTTON1);   //establish interrupt handler from Button 1 input (B1 USER)

int buttonPressed = 0;
int oscillateLED_L = 0;

int main() {
    // start the allowed execution of the thread
    printf("----------------START----------------\n");
	printf("Starting state of thread: %d\n", controller.get_state());
    controller.start(oscillateLED);
	printf("State of thread right after start: %d\n", controller.get_state());
    
    cherish.rise(button1PushDownBehavior);    //set the button to 
	cherish.fall(button1OpenBehavior);
    return 0;
}

// 
void oscillateLED() {
    while (true) {
        if(oscillateLED_L==0){
            fire = !fire;
            printf("LED switched to state: HIGH\tu %d\t z %d\r\n", oscillateLED_L, buttonPressed); //you do need to update the print statement to be correct
            thread_sleep_for(2000); //Thread_sleep is a time delay function, causes a 2000 unit delay
            fire = !fire;
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