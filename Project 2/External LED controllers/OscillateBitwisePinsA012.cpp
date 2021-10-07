/******************************************************************************
*   File Name:      OscillateBitwisePinsA012.cpp
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
*   Outputs:        This program endlessly alternates between sending a digital 
*                     high signal to pins A0, A1, and A2 for 500 ms per pin
*                     before proceeding to the next.
*
*   Constraints:    Requires external connection to 3 LEDs, resistors, and 
*                     (optionally) a breadboard for output.
*
*   References:     https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
*
******************************************************************************/

#include "mbed.h"

//the time that should be spent at one output stage before proceeding to the next stage
uint32_t STAGE_TIME = 500;

int main(){
    printf("===== Beginning execution =====\n");
    printf("=== Made by  Misha Nelyubov ===\n");

    //enable PA and PC
    RCC->AHB2ENR|=5;

    //set PA3 (labeled A0 on board) to output
    GPIOA->MODER&=~(0x80);
    GPIOA->MODER|=0x40;
    
    //set PC0 (labeled A1 on board) to output
    GPIOC->MODER &=~(0x2);
    GPIOC->MODER |=0x1;
    
    //set PC3 (labeled A2 on board) to output
    GPIOC->MODER &=~(0x80);
    GPIOC->MODER |=0x40;


    while(true){
        //cycle A0
        printf("Setting A0 to 5V DC\n");
        GPIOA->ODR |=   0x8;

        thread_sleep_for(STAGE_TIME);

        printf("Setting A0 to 0V DC\n");
        GPIOA->ODR &= ~(0x8);


        //cycle A1
        printf("Setting A1 to 5V DC\n");
        GPIOC->ODR |=   0x1;
        
        thread_sleep_for(STAGE_TIME);

        printf("Setting A1 to 0V DC\n");
        GPIOC->ODR &= ~(0x1);


        //cycle A2
        printf("Setting A2 to 5V DC\n");
        GPIOC->ODR |=   0x8;
        
        thread_sleep_for(STAGE_TIME);

        printf("Setting A2 to 0V DC\n");
        GPIOC->ODR &= ~(0x8);
    }

}
