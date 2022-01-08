/******************************************************************************
*   File Name:      cse321_project2_mnelyubo_LCD_test.cpp
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   10/16/2021
*   Last Modified:  10/16/2021
*   Purpose:        Verify output channels to external LDC. 
*
*   Functions:      
*               
*   Assignment:     CSE321 Project 2
*
*   Inputs:         None
*
*   Outputs:        LCD display
*
*   Constraints:    LCD must be connected to system
*
*   References:     
*               https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
*
*
******************************************************************************/
#include "mbed.h"
#include "1802.h"
#include <cstdio>
#include <ctime>
#include <string>

// #define COL 16
// #define ROW 2


// //create interface to output LCD
// CSE321_LCD lcdObject(COL,ROW);

// int main() {
//     lcdObject.begin();       //initialize LCD

//     printf("\n\n== Initialized LCD Test ==\n");

//     while (1) {
//            //Every 0.5 seconds, print a character to the console to create the pattern:
//            //0123456789
//            //0123456789
//         for(char i=0; i<20; i++){
//             lcdObject.setCursor(i%10, i/10);
//             char printVal[]= {(char)('0' + i%10), '\0'};
//             lcdObject.print(printVal);
//             thread_sleep_for(500);
//         }
//         lcdObject.clear();       //reset the output after fully populating the LCD output

//     }

//     return 0;
// }

