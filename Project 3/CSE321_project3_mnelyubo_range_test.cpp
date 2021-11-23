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
*                     Trig - PC_9
*                     Echo - PC_8
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


//timing behavior
#define POLLING_HIGH_TIME     10us
#define POLLING_CYCLE_TIME_MS 100

//data type alias
#define ull unsigned long long

//output stabilization buffer data
#define stabilizerArrayLen 40
#define DISTANCE_MAXIMUM 400  /* sensor max range is stated to be 4m */

void riseHandler();
void fallHandler();

ull getTimeSinceStart();
int getStableDistance();

int stableDistance[stabilizerArrayLen];
int stableDistIdx = 0;

Timer timer;

ull riseDetected = 0;
ull fallDetected = 0;

// DigitalOut trig(PC_8);
InterruptIn echo(PC_8);

int main(){

    //create rise and fall timers for input port
    echo.rise(riseHandler);
    echo.fall(fallHandler);

    //enable port C
    RCC->AHB2ENR |= 0x4;

    //configure pin C9 as an output
    GPIOC->MODER &= ~(0x80000);
    GPIOC->MODER |= 0x40000;

    //configure pin C8 as an input
    GPIOC->MODER &= ~(0x30000);

    while(true){
        timer.start();

        //send trigger signal high for 10 us
        GPIOC->ODR |= 0x200;
        wait_us(10);
        GPIOC->ODR &= ~(0x200);

        thread_sleep_for(60);   //sensor documentation states wait for 60ms before measuring upper limit


        while(getTimeSinceStart() < POLLING_CYCLE_TIME_MS * 1000){
            if(riseDetected && fallDetected){
                ull deltaTime = fallDetected - riseDetected;
                //printf("Rise and Fall Detected! Rise Time: %llu\tFall Time: %llu\tDelta Time: %llu\n", riseDetected, fallDetected, deltaTime);
                int distance = deltaTime / 58;
                if(distance < DISTANCE_MAXIMUM){
                    stableDistance[stableDistIdx++ % stabilizerArrayLen]=distance;
                }

                printf("Sample measured: %d cm \tStabilized estimate: %d\n", distance, getStableDistance());   //documentation states divide by 58 to provide distance in CM
                riseDetected=false;
                fallDetected=false;
            }
        }
        
        //prevent half-returns from cascading into the next data point
        riseDetected=false;
        fallDetected=false;

        timer.stop();
        timer.reset();
    }
    return 0;
}


void fallHandler(){
    fallDetected=getTimeSinceStart();
}

void riseHandler(){
    riseDetected=getTimeSinceStart();
}

int getStableDistance(){
    int sum = 0;
    for(int i = 0; i < stabilizerArrayLen; i++){
        sum += stableDistance[i];
    }
    return sum / stabilizerArrayLen;
}

//Access global object timer to get the time since the process began in microseconds
//code in this function is from https://os.mbed.com/docs/mbed-os/v6.15/apis/timer.html
ull getTimeSinceStart() {
    using namespace std::chrono;
    return duration_cast<microseconds>(timer.elapsed_time()).count();
}
