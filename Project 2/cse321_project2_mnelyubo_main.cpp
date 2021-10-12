/******************************************************************************
*   File Name:      cse321_project2_mnelyubo_main.cpp
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   10/10/2021
*   Last Modified:  10/10/2021
*   Purpose:        Stage 1: extract clean input from keypad.
*
*   Functions:      
*               The following set of functions are used as handlers for the rising and falling edge behaviors of keypad buttons
*               isr_abc
*               isr_369
*               isr_258
*               isr_147
*               osr_abc
*               osr_369
*               osr_258
*               osr_147
*
*   Assignment:     CSE321 Project 2
*
*   Inputs:         4x4 matrix array input buttons
*
*   Outputs:        Serial output, LCD display (TBD)
*
*   Constraints:    single-file polling of 4 input channels may trigger a button to be counted more than once
*
*   References:     
*               https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
*
*
******************************************************************************/
#include "mbed.h"
#include "1802.h"

#define COL 16
#define ROW 2

//LCD_5x10DOTS

//CSE321_LCD lcdObject(COL,ROW,);




#include <cstdio>
void isr_abc(void);
void isr_369(void);
void isr_258(void);
void isr_147(void);

void osr_abc(void);
void osr_369(void);
void osr_258(void);
void osr_147(void);

char keyValues[][5] = {"abcd","369#","2580","147*"};

int row = 0;            //the row currently being supplied a non-zero voltage
int logLine = 0;        //debugging utility to notify how many lines have been printed for understanding otherwise identical output
int buttonPressed = 0;  //boolean for if a keypad number that is currently live has been pressed down.  Used to halt row oscillation until it is opened.

InterruptIn rowLL(PC_0);
InterruptIn rowCL(PC_3);
InterruptIn rowCR(PC_1);
InterruptIn rowRR(PC_4);

int main() {

    RCC->AHB2ENR |= 0x4;

    GPIOC->MODER |= 0x550000;
    GPIOC->MODER &= ~(0xAA0000);
    printf("==Initialized==\n");
    rowLL.rise(&isr_abc);
    rowCL.rise(&isr_369);
    rowCR.rise(&isr_258);
    rowRR.rise(&isr_147);

    rowLL.fall(&osr_abc);
    rowCL.fall(&osr_369);
    rowCR.fall(&osr_258);
    rowRR.fall(&osr_147);

    while (1) {
        switch (row){
            case 0:
                GPIOC->ODR |= 0x100;
                break;
            case 1:
                GPIOC->ODR |= 0x200;
                break;
            case 2:
                GPIOC->ODR |= 0x400;
                break;
            case 3:
                GPIOC->ODR |= 0x800;
                break;
        }
        thread_sleep_for(500);
        GPIOC->ODR &= ~(0xF00);
        thread_sleep_for(20);
        row++;
        row%=4;
    }
    return 0;
}


void isr_abc(void) {printf("found a, b, c, or d. ll: %d  row: %d\n",logLine++, row);}
void osr_abc(void) {printf("open  a, b, c, or d. ll: %d  row: %d\n",logLine++, row);}

void isr_369(void) {printf("found 3, 6, 9, or #. ll: %d  row: %d\n",logLine++, row);}
void osr_369(void) {printf("open  3, 6, 9, or #. ll: %d  row: %d\n",logLine++, row);}

void isr_258(void) {printf("found 2, 5, 8, or 0. ll: %d  row: %d\n",logLine++, row);}
void osr_258(void) {printf("open  2, 5, 8, or 0. ll: %d  row: %d\n",logLine++, row);}

void isr_147(void) {printf("found 1, 4, 7, or *. ll: %d  row: %d\n",logLine++, row);}
void osr_147(void) {printf("open  1, 4, 7, or *. ll: %d  row: %d\n",logLine++, row);}
