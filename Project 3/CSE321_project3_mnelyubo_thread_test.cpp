/******************************************************************************
*   File Name:      CSE321_project3_mnelyubo_range_test.cpp
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   11/22/2021
*   Last Modified:  11/22/2021
*   Purpose:        This program tests the operation of EventQueue
*
*   Functions:      N/A
*
*   Assignment:     Project 3
*
*   Inputs:         None
*
*   Outputs:        Serial printout
*
*   Constraints:    N/A
*
*   References:
*       NUCLEO datasheet:                  https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
*       MBED OS API: timer                 https://os.mbed.com/docs/mbed-os/v6.15/apis/timer.html
*
******************************************************************************/

#include "mbed.h"
#include <chrono>

Thread t1;
Thread t2;
Thread t3;


//the amount of times that output has been called
int beat1 = 0;
int beat2 = 0;
int beat3 = 0;

Ticker tick;

void outputInit();
void output();

EventQueue eq1(32 * EVENTS_EVENT_SIZE);

int main(){
    t1.start(callback(&eq1, &EventQueue::dispatch_forever));    //start thread1 to process event queue 1

    tick.attach(&outputInit, 1s);       //set the ticker to run the output initializer once every second

    while(true){}                       //run indefinitely

    return 0;
}

//this ISR function adds the time-costly event to the event queue
void outputInit(){
    eq1.call(output);
}

//this EventQueue-called function executes lengthier calls
void output(){
    printf("heartbeat rise %d\n", ++beat1);
    
    thread_sleep_for(500);

    printf("heartbeat fall %d\n", beat1);
}

