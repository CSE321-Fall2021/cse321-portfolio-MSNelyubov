/******************************************************************************
*   File Name:      OscillateBitwiseRainbow.cpp
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   2021/10/03
*   Last Modified:  2021/10/06
*   Purpose:        Demonstrate a blinking LED and console print statements for 
*                     the CSE321 Lab deliverable.
*
*   Functions:      
*
*   Assignment:     CSE321 Lab deliverable 1
*
*   Inputs:         None
*
*   Outputs:        Five external LEDs connected to pins A1,A2,A3,A4,A5 output
*                     the boolean representations of 0-31 with 250ms intervals
*                     between number switches.
*
*   Constraints:    Requires external connection to 3 LEDs, resistors, and 
*                     (optionally) a breadboard for output.
*
*   References:     https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
*
******************************************************************************/

#include "mbed.h"


int pins[] = {0x1, 0x8, 0x2, 0x10, 0x20};
int names[] = {1,2,3,4,5};
int pinArrayLen = 5;


uint32_t PHASE_TIME = 250;

int main(){
    printf("===== Beginning execution =====\n");
    printf("=== Made by  Misha Nelyubov ===\n");

    //enable PA and PC
    RCC->AHB2ENR|=5;
    
    //set PC0 (labeled A1 on board) to output
    GPIOC->MODER &= ~(0x2);
    GPIOC->MODER |= 0x1;
    

    //set PC3 (labeled A2 on board) to output
    GPIOC->MODER &= ~(0x80);
    GPIOC->MODER |= 0x40;


    //set PC1 (labeled A3 on board) to output
    GPIOC->MODER &= ~(0x8);
    GPIOC->MODER |= 0x4;


    //set PC4 (labeled A4 on board) to output
    GPIOC->MODER &= ~(0x200);
    GPIOC->MODER |= 0x100;


    //set PC5 (labeled A5 on board) to output
    GPIOC->MODER &= ~(0x800);
    GPIOC->MODER |= 0x400;

    //the sequence of pin outputs as a table (2D array) with each row being an output stage and each column being the behavior of a given pin at that stage
    int stageCount = 32;
    int stages[][5] = {
        {0,0,0,0,0},
        {0,0,0,0,1},
        {0,0,0,1,0},
        {0,0,0,1,1},
        {0,0,1,0,0},
        {0,0,1,0,1},
        {0,0,1,1,0},
        {0,0,1,1,1},
        {0,1,0,0,0},
        {0,1,0,0,1},
        {0,1,0,1,0},
        {0,1,0,1,1},
        {0,1,1,0,0},
        {0,1,1,0,1},
        {0,1,1,1,0},
        {0,1,1,1,1},
        {1,0,0,0,0},
        {1,0,0,0,1},
        {1,0,0,1,0},
        {1,0,0,1,1},
        {1,0,1,0,0},
        {1,0,1,0,1},
        {1,0,1,1,0},
        {1,0,1,1,1},
        {1,1,0,0,0},
        {1,1,0,0,1},
        {1,1,0,1,0},
        {1,1,0,1,1},
        {1,1,1,0,0},
        {1,1,1,0,1},
        {1,1,1,1,0},
        {1,1,1,1,1}
    };

    for(int stage = 0; true; stage = (stage+1)%stageCount){
        //enable outputs of the given stage
        printf("Switching to stage %d\n", stage);
        for(int pin = 0; pin<pinArrayLen; pin++){
            if(stages[stage][pin])
                GPIOC->ODR |= pins[pin];
        }
        
        //pause for the designated time at the given stage
        thread_sleep_for(PHASE_TIME);
        
        //set ALL pins to low
        for(int pin = 0; pin<pinArrayLen; pin++){
            GPIOC->ODR &= ~pins[pin];
        }

    }

}
