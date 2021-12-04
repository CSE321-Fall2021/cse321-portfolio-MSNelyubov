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
*   References:                                                               *
*                                                                             *
******************************************************************************/

#include <mbed.h>

#define nanosecondsPerSecond 1000*1000*1000 

Thread buzzerThread;
Thread alternatorThread;

void runBuzzer();

#define frequencyTableLength 64
#define frequencyTableFields 3
#define tableOffsetFreq 0
#define tableOffsetDutyCycle 1
#define tableOffsetDuration 2

/**
*   This array contains the "audio" data to continously play on the buzzer.
*   array entries %3 are as follows:
*   %3=0 -> frequency (Hz)
*   %3=1 -> duty cycle (0-100, 0 -> off)
*   %3=2 -> note duration (ms)
*/
int outputSoundTable[frequencyTableLength * frequencyTableFields] = {
  /*frequency,  duty cycle, note duration(ms)*/
    200,        20,         100,        //first cycle starts with lowest pitch high-switch sound
    200,        00,         100,
    200,        20,         100,
    200,        00,         100,
    050,        20,         400,        //low note after two short high notes
    200,        20,         100,
    200,        00,         100,
    200,        20,         100,
    200,        00,         100,
    075,        20,         400,        //low note after two short high notes
    150,        20,         200,    //end of tune jingle
    100,        20,         200,    //
    125,        20,         200,    //
    100,        20,         200,    //
    125,        20,         200,    //
    1,          00,         750,        //produce no sound for at end of cycle

    250,        20,         100,        //repeat with higher fast-switch tones
    250,        00,         100,
    250,        20,         100,
    250,        00,         100,
    050,        20,         400,        //low note after two short high notes
    250,        20,         100,
    250,        00,         100,
    250,        20,         100,
    250,        00,         100,
    075,        20,         400,        //low note after two short high notes
    150,        20,         200,    //end of tune jingle
    100,        20,         200,    //
    125,        20,         200,    //
    100,        20,         200,    //
    125,        20,         200,    //
    1,          00,         750,        //produce no sound for at end of cycle

    300,        20,         100,        //repeat with even higher fast-switch tones
    300,        00,         100,
    300,        20,         100,
    300,        00,         100,
    050,        20,         400,        //low note after two short high notes
    300,        20,         100,
    300,        00,         100,
    300,        20,         100,
    300,        00,         100,
    075,        20,         400,        //low note after two short high notes
    150,        20,         200,    //end of tune jingle
    100,        20,         200,    //
    125,        20,         200,    //
    100,        20,         200,    //
    125,        20,         200,    //
    1,          00,         750,        //produce no sound for at end of cycle

    350,        20,         100,        //repeat with highest pitch fast-switch tones
    350,        00,         100,
    350,        20,         100,
    350,        00,         100,
    050,        20,         400,        //low note after two short high notes
    350,        20,         100,
    350,        00,         100,
    350,        20,         100,
    350,        00,         100,
    075,        20,         400,        //low note after two short high notes
    150,        20,         200,    //end of tune jingle
    100,        20,         200,    //
    125,        20,         200,    //
    100,        20,         200,    //
    125,        20,         200,    //
    1,          00,         750        //produce no sound for at end of cycle

};


int DUTY_CYCLE = 0;             //integer between 0 and 100 indicating what percent of the time the signal should be high


DigitalOut alarm_Enable(PB_10);    //starts off with 0V. power to alarm disabled until the alarm state has been set to inactive
DigitalOut alarm_data_L(PB_11);      //starts off with 0V. active low component that produces a noise when active


int OSCILLATION_FREQ;      //frequency of digital signal oscillation in Hertz

int main() {
    alarm_data_L.write(1);   //start the alarm in a disabled state (active low -> 1 disables)
    alarm_Enable.write(1); //supply power to alarm
    
    buzzerThread.start(runBuzzer);      //set the buzzer execution thread to oscillate I/O at the variable oscillation frequency

    int currentNoteIndex = 0;
    int waitTime = 0;       //time (ms) to wait before switching to the next note
    while(1) {
        OSCILLATION_FREQ = outputSoundTable[frequencyTableFields * currentNoteIndex + tableOffsetFreq];     //switch the frequency to the next table value
        DUTY_CYCLE = outputSoundTable[frequencyTableFields * currentNoteIndex + tableOffsetDutyCycle];      //switch the duty cycle to the next table value
        waitTime = outputSoundTable[frequencyTableFields * currentNoteIndex + tableOffsetDuration];         //switch the duration to the next table value

        // printf("New operating frequency: %d Hz\n",OSCILLATION_FREQ);

        thread_sleep_for(waitTime);                                         //idle for the designated note duration before proceeding
        currentNoteIndex = (currentNoteIndex + 1) % frequencyTableLength;                                   //proceed to next table value in next cycle of while loop
    }

  return 0;
}

void runBuzzer(){
    while(1) {
        int Period = nanosecondsPerSecond / (OSCILLATION_FREQ);
        int highPeriod = Period * DUTY_CYCLE / 100;
        int lowPeriod = Period - highPeriod;        //ensure that low period + high period time adds to period time 
        alarm_data_L.write(0);
        wait_ns(highPeriod);
        alarm_data_L.write(1);
        wait_ns(lowPeriod);
    }
}
