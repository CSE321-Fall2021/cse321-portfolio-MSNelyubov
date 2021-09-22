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


//Can you fix me?
//well you kinda have no choice

#include "mbed.h"

// Create a thread to drive an LED to have an on time of _______ms and off time
// _______ms

Thread controller;
//what is a thread?? eh like something tottaly Real time like. 

void saveTheWorld(); //someone has to right?

DigitalOut fire(LED2); // establish blue led as an output
int unicorn=0; InterruptIn cherish(BUTTON1); void live_or_die(); int zombie = 0;
void wrong_or_not();
































int main() {
  // start the allowed execution of the thread
  printf("----------------START----------------\n");
	printf("Starting state of thread: %d\n", controller.get_state());
  controller.start(saveTheWorld);
			printf("State of thread right after start: %d\n", controller.get_state());
  cherish.rise(live_or_die);
	cherish.fall(wrong_or_not);
	//https://youtu.be/XN2FrUUq-zI 

  return 0;
}

// make the handler
void saveTheWorld() {
while (true) {





















if(unicorn==0){
fire = !fire;
printf("What did I just do????"); //you do need to update the print statement to be correct
thread_sleep_for(2000); //Thread_sleep is a time delay function, causes a 2000 unit delay
fire = !fire;
printf("Did I do it again??????"); thread_sleep_for(500); //Thread_sleep is a time delay function, causes a 500 unit delay
}
  }
}

void live_or_die() {
												  // togle the state of the thread
												  
												  
												  
												  
												  
												  // set flag...this is where we end
													zombie=1;
}

void wrong_or_not() {
  if (zombie==1){
	  
	  
	  
	  
    unicorn++; 
	
	
	
    unicorn %= 2; 						    zombie=0;
}}