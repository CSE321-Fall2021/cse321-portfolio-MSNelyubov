/******************************************************************************
*   File Name:      CSE321_project3_mnelyubo_buzzer_test.cpp                  *
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)                     *
*   Date Created:   12/01/2021                                                *
*   Last Modified:  12/01/2021                                                *
*   Purpose:        This file tests the effect of various frequency inputs    *
*                     on the buzzer output.                                   *
*                   WARNING: Unpleasant sounds produced.                      *
*   Functions:                                                                *
*                                                                             *
*   Assignment:     Project 3                                                 *
*                                                                             *
*   Inputs:         None                                                      *
*                                                                             *
*   Outputs:        Buzzer                                                    *
*                                                                             *
*   Constraints:                                                              *
*       The buzzer must be connected to the system with the following pins:   *
*                       GND - GND                                             *
*                       I/O - PB_11                                           *
*                       VCC - PB_10                                           *
*                                                                             *
*   References:     TODO                                                      *
*                                                                             *
******************************************************************************/

#include <mbed.h>

#define nanosecondsPerSecond 1000*1000*1000 

Thread buzzerThread;
Thread alternatorThread;

void runBuzzer();

DigitalOut alarm_Vcc(PB_10);    //starts off with 0V. power to alarm disabled until the alarm state has been set to inactive
DigitalOut alarm_L(PB_11);      //starts off with 0V. active low component that produces a noise when active


int OSCILLATION_FREQ = 10;

int main() {
    alarm_L.write(1);   //start the alarm in a disabled state (active low -> 1 disables)
    alarm_Vcc.write(1); //supply power to alarm

    buzzerThread.start(runBuzzer);

    while(1) {
        thread_sleep_for(5);         //idle for a second between decrements
        OSCILLATION_FREQ*=1.1;   //gradually decrease the frequency every second
        printf("New operating frequency: %d Hz\n",OSCILLATION_FREQ);
    }

  return 0;
}

void runBuzzer(){
    while(1) {
        int halfPeriod = nanosecondsPerSecond / (2 * OSCILLATION_FREQ);
        alarm_L.write(0);
        wait_ns(halfPeriod);
        alarm_L.write(1);
        wait_ns(halfPeriod);
    }
}
