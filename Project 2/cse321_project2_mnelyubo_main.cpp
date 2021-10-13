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
*                   no more than one keypad button can be pressed at any point in time
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

#define RisingEdgeInterrupt 1
#define FallingEdgeInterrupt 0

//LCD_5x10DOTS

//CSE321_LCD lcdObject(COL,ROW,);




#include <cstdio>

////declare individual interrupt handler events
//declare rising edge interrupt handler events
void isr_abc(void);
void isr_369(void);
void isr_258(void);
void isr_147(void);

//declare falling edge interrupt handler events
void osr_abc(void);
void osr_369(void);
void osr_258(void);
void osr_147(void);

//declare general handler for Matrix keypad input events
void handleMatrixButtonEvent(int isRisingEdgeInterrupt,int column, int row);


char keyValues[][5] = {"dcba","#963","0852","*741"};

int row = 0;                //the row currently being supplied a non-zero voltage
int logLine = 0;            //debugging utility to notify how many lines have been printed for understanding otherwise identical output

int buttonPressed = 0;      //boolean for if a keypad number that is currently live has been pressed down.  Used to halt row oscillation until it is opened.
char charPressed = '\0';    //the character on the input matrix keypad which is currently pressed down.  Defaults to '\0' when no key is pressed.
                            //undefined behavior when more than one key is pressed at the same time


InterruptIn rowLL(PC_0);
InterruptIn rowCL(PC_3);
InterruptIn rowCR(PC_1);
InterruptIn rowRR(PC_4);

int main() {

    RCC->AHB2ENR |= 0x4;    //enable RCC for GPIO C

    GPIOC->MODER |= 0x550000;       //configugure GPIO pins PC8,PC9,PC10,PC11
    GPIOC->MODER &= ~(0xAA0000);    //as outputs

    printf("==Initialized==\n");

    rowLL.rise(&isr_abc);   //assign interrupt handler for a rising edge event from the row containing buttons a,b,c,d
    rowCL.rise(&isr_369);   //assign interrupt handler for a rising edge event from the row containing buttons 3,6,9,#
    rowCR.rise(&isr_258);   //assign interrupt handler for a rising edge event from the row containing buttons 2,5,8,0
    rowRR.rise(&isr_147);   //assign interrupt handler for a rising edge event from the row containing buttons 1,4,7,*

    rowLL.fall(&osr_abc);   //assign interrupt handler for a falling edge event from the row containing buttons a,b,c,d
    rowCL.fall(&osr_369);   //assign interrupt handler for a falling edge event from the row containing buttons 3,6,9,#
    rowCR.fall(&osr_258);   //assign interrupt handler for a falling edge event from the row containing buttons 2,5,8,0
    rowRR.fall(&osr_147);   //assign interrupt handler for a falling edge event from the row containing buttons 1,4,7,*

    while (1) {

        //supply voltage to one output row at a time
        switch (row){
            case 0:
                GPIOC->ODR |= 0x100;        //supply voltage to the row of keypad buttons with labels *0#D
                break;
            case 1:
                GPIOC->ODR |= 0x200;        //supply voltage to the row of keypad buttons with labels 789C
                break;
            case 2:
                GPIOC->ODR |= 0x400;        //supply voltage to the row of keypad buttons with labels 456B
                break;
            case 3:
                GPIOC->ODR |= 0x800;        //supply voltage to the row of keypad buttons with labels 123A
                break;
        }

        thread_sleep_for(5);              //maintain power to the row for a brief period of time to account for bounce

        //proceed to scanning the next input if and only if there is no closed loop in the current scan set
        if(!buttonPressed){
            GPIOC->ODR &= ~(0xF00);         //reset voltage to output 0 on all pins

            thread_sleep_for(1);           //wait to give any falling edge triggers a chance to resolve before proceeding

            row++;                          //update the row target to poll the next row
            row%=4;
        }
    }

    return 0;
}


void isr_abc(void) {buttonPressed = 1;printf("found %c. ll: %d\n",keyValues[0][row],logLine++);}
void osr_abc(void) {buttonPressed = 0;printf("lost  %c. ll: %d\n",keyValues[0][row],logLine++);}
void isr_369(void) {buttonPressed = 1;printf("found %c. ll: %d\n",keyValues[1][row],logLine++);}
void osr_369(void) {buttonPressed = 0;printf("lost  %c. ll: %d\n",keyValues[1][row],logLine++);}
void isr_258(void) {buttonPressed = 1;printf("found %c. ll: %d\n",keyValues[2][row],logLine++);}
void osr_258(void) {buttonPressed = 0;printf("lost  %c. ll: %d\n",keyValues[2][row],logLine++);}
void isr_147(void) {buttonPressed = 1;printf("found %c. ll: %d\n",keyValues[3][row],logLine++);}
void osr_147(void) {buttonPressed = 0;printf("lost  %c. ll: %d\n",keyValues[3][row],logLine++);}


void handleMatrixButtonEvent(int isRisingEdgeInterrupt,int column, int row){
    if(isRisingEdgeInterrupt)
        printf("press  %c. ll: %d\n",keyValues[column][row],logLine++);

}