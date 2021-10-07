/******************************************************************************
*   File Name:      OscillateApi1_5HzPinsA1_2_3.cpp
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   2021/10/03
*   Last Modified:  2021/10/03
*   Purpose:        Demonstrate 3 alternating blinking LEDs and console print 
*                     statements for the CSE321 Lab deliverable.
*
*   Functions:      
*
*   Assignment:     CSE321 Lab deliverable 1
*
*   Inputs:         None
*
*   Outputs:        pins A1,A2,A3 on board
*
*   Constraints:    Requires external connection to 3 LEDs, resistors, and 
*                     (optionally) a breadboard for output.
*
*   References:     https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
*
******************************************************************************/

#include "mbed.h"


DigitalOut out0(PC_0);          //establish PA_0 output
DigitalOut out1(PC_3);          //establish PC_0 output
DigitalOut out2(PC_1);          //establish PC_3 output


uint32_t PHASE_TIME = 1000;

int main(){
    printf("===== Beginning execution =====");
    while(true){
        //cycle A0
        printf("Sending 5V DC to A0\n");
        out0 = !out0;
        thread_sleep_for(PHASE_TIME);

        printf("Sending 0V DC to A0\n");
        out0 = !out0;


        //cycle A1
        printf("Sending 5V DC to A1\n");
        out1 = !out1;
        thread_sleep_for(PHASE_TIME);

        printf("Sending 0V DC to A1\n");
        out1 = !out1;


        //cycle A2
        printf("Sending 5V DC to A2\n");
        out2 = !out2;
        thread_sleep_for(PHASE_TIME);

        printf("Sending 0V DC to A2\n");
        out2 = !out2;
    }

}
