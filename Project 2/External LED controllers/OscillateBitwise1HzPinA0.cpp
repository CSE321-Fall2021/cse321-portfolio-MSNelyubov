/******************************************************************************
*   File Name:      OscillateBitwise1HzPinA0.cpp
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   2021/10/03
*   Last Modified:  2021/10/03
*   Purpose:        Demonstrate a blinking LED and console print statements for 
*                     the CSE321 Lab deliverable.
*
*   Functions:      
*
*   Assignment:     CSE321 Lab deliverable 1
*
*   Inputs:         None
*
*   Outputs:        1Hz 50% uptime 5V/0V square wave to pin A0
*
*   Constraints:    Requires external connection to 3 LEDs, resistors, and 
*                     (optionally) a breadboard for output.
*
*   References:     https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
*
******************************************************************************/

#include "mbed.h"

int main(){
    printf("===== Beginning execution =====");

    //enable PA
    RCC->AHB2ENR|=1;

    //set PA3 (labeled A0 on board) to output
    GPIOA->MODER&=~(0x0080);
    GPIOA->MODER|=0x0040;
    
    while(true){
        printf("Sending 5V DC to A0\n");
        GPIOA->ODR |=   0x8;
        thread_sleep_for(500);

        printf("Sending 0V DC to A0\n");
        GPIOA->ODR &= ~(0x8);
        thread_sleep_for(500);        
    }

}
