/******************************************************************************
*   File Name:      CSE321_project3_mnelyubo_range_test.cpp
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   11/20/2021
*   Last Modified:  11/20/2021
*   Purpose:        This program tests the operation of the range detection sensor
*
*   Functions:      
*
*   Assignment:     Project 3
*
*   Inputs:         Range Detection Sensor
*
*   Outputs:        Serial printout
*
*   Constraints:    Range Detection Sensor must be connected to the Nucleo with the following pins:
*                     Vcc  - 5V
*                     Trig - PC_8
*                     Echo - PC_9
*                     Gnd  - GND
*
*   References:
*       HC-SR04 audio sensor datasheet:    https://www.digikey.com/htmldatasheets/production/1979760/0/0/1/hc-sr04.html
*       NUCLEO datasheet:                  https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
*       MBED OS API: timer                 https://os.mbed.com/docs/mbed-os/v6.15/apis/timer.html
*
******************************************************************************/

#include "mbed.h"
#include <chrono>


// Blinking rate in milliseconds
#define POLLING_HIGH_TIME     10us
#define POLLING_CYCLE_TIME_MS 1000
#define ull unsigned long long

void riseHandler();
void fallHandler();

Timer timer;

ull getTimeSinceStart();

ull riseDetected = 0;
ull fallDetected = 0;

// DigitalOut trig(PC_8);
InterruptIn echo(PC_9);

int main(){

    echo.rise(riseHandler);
    echo.fall(fallHandler);

    RCC->AHB2ENR |= 0x4;    //enable clock to port C

    //configure pin C8 as an output
    GPIOC->MODER &= ~(0x20000);
    GPIOC->MODER |= 0x10000;

    //configure pin C9 as an input
    GPIOC->MODER &= ~(0xC0000);

    printf("\nInitial Input State: %d\n", GPIOC->IDR & 0x200);

    timer.start();
    //printf("=== beginning signal broadcast at %llu ===\n", getTimeSinceStart());
    GPIOC->ODR |= 0x100;
    wait_us(10);
    GPIOC->ODR &= ~(0x100);
    ull endOfHigh = getTimeSinceStart();

    thread_sleep_for(60);   //documentation states sleep for 60ms before measuring
    //printf("=== ended signal broadcast at %llu\n", endOfHigh);
    
    int i=0;
    
    while(getTimeSinceStart() < POLLING_CYCLE_TIME_MS * 1000){
        if(riseDetected && fallDetected){
            ull deltaTime = fallDetected - riseDetected;
            printf("Rise and Fall Detected! Rise Time: %llu\tFall Time: %llu\tDelta Time: %llu\n", riseDetected, fallDetected, deltaTime);
            printf("Distance estimate: %llu cm\n", deltaTime / 58);   //documentation states divide by 58 to provide distance in CM
        }

        if(riseDetected){
            printf("Rise detected at %llu\n", riseDetected);
            riseDetected=false;
        }

        if(fallDetected){
            printf("Fall detected at %llu\n", fallDetected);
            fallDetected=false;
        }

        i++;

        if(i%0x10000 == 0){
            printf("Input State: %d\tTimer: %llu\n", GPIOC->IDR & 0x200, getTimeSinceStart());
        }
    }
    return 0;
}


void fallHandler(){
    fallDetected=getTimeSinceStart();
}

void riseHandler(){
    riseDetected=getTimeSinceStart();
}


//https://os.mbed.com/docs/mbed-os/v6.15/apis/timer.html
using namespace std::chrono;

//https://os.mbed.com/docs/mbed-os/v6.15/apis/timer.html
unsigned long long getTimeSinceStart() {
    return duration_cast<microseconds>(timer.elapsed_time()).count();
}
