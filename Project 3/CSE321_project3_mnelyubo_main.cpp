/******************************************************************************
*   File Name:      CSE321_project3_mnelyubo_range_test.cpp
*   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
*   Date Created:   11/20/2021
*   Last Modified:  11/20/2021
*   Purpose:
*       This program operates four peripherals to notify workers about if there
*         are food items remaining in a container that can be taken home
*         at closing time.
*
*   Functions:      
*
*   Assignment:     Project 3
*
*   Inputs:         Range Detection Sensor, 4x4 Matrix Keypad
*
*   Outputs:        Serial printout, LCD, Alarm Buzzer
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


//Shared variables
int stableDistance = 0;
Mutex stableDistanceRWMutex;

//Internal variables exclusive to input Data Stream: Distance Sensor 
Thread distanceSensorThread;
EventQueue distanceSensorEventQueue(32 * EVENTS_EVENT_SIZE);

void enqueuePoll();
void pollDistanceSensor();
void processDistanceData();

void distanceEchoRiseHandler();
void distanceEchoFallHandler();

ull getTimeSinceStart();
int updateStableDistance();

int distanceBuffer[stabilizerArrayLen];
int distanceBuffIdx = 0;

Timer distanceEchoTimer;
Ticker distanceSensorPollStarter;

ull riseEchoTimestamp = 0;
ull fallEchoTimestamp = 0;

InterruptIn echo(PC_8);

int main(){

    //create rise and fall timers for input port
    echo.rise(distanceEchoRiseHandler);
    echo.fall(distanceEchoFallHandler);

    /*************************
    *   Pin configuration    *
    *************************/

    //enable port C
    RCC->AHB2ENR |= 0x4;

    //configure pin C9 as an output
    GPIOC->MODER &= ~(0x80000);
    GPIOC->MODER |= 0x40000;

    //configure pin C8 as an input
    GPIOC->MODER &= ~(0x30000);


    /*************************
    *  Thread configuration  *
    *************************/
    distanceSensorThread.start(callback(&distanceSensorEventQueue, &EventQueue::dispatch_forever));
    distanceSensorPollStarter.attach(&enqueuePoll, 100ms);

    while(true){
        thread_sleep_for(1000);
    }
    return 0;
}


void enqueuePoll(){
    distanceSensorEventQueue.call(pollDistanceSensor);
}

void pollDistanceSensor(){
    distanceEchoTimer.start();  //start the timer to measure response time

    //send trigger signal high for 10 us
    GPIOC->ODR |= 0x200;
    wait_us(10);
    GPIOC->ODR &= ~(0x200);
}

void processDistanceData(){
    ull deltaTime = fallEchoTimestamp - riseEchoTimestamp;
    int distance = deltaTime / 58;
    if(distance < DISTANCE_MAXIMUM){
        distanceBuffer[distanceBuffIdx++ % stabilizerArrayLen]=distance;
    }

    printf("Threaded sample measured: %d cm \tStabilized estimate: %d\n", distance, updateStableDistance());   //documentation states divide by 58 to provide distance in CM
    riseEchoTimestamp=false;
    fallEchoTimestamp=false;

    distanceEchoTimer.stop();
    distanceEchoTimer.reset();  //reset the timer for the next run

}


void distanceEchoFallHandler(){
    fallEchoTimestamp=getTimeSinceStart();
    distanceSensorEventQueue.call(processDistanceData);
}

void distanceEchoRiseHandler(){
    riseEchoTimestamp=getTimeSinceStart();
}

//locks and writes to the stableDistance shared variable
int updateStableDistance(){
    int sum = 0;
    for(int i = 0; i < stabilizerArrayLen; i++){
        sum += distanceBuffer[i];
    }
    int averageDistance = sum / stabilizerArrayLen;
    stableDistanceRWMutex.lock();
    stableDistance = averageDistance;
    stableDistanceRWMutex.unlock();
    return averageDistance;
}

//Access global object timer to get the time since the process began in microseconds
//code in this function is from https://os.mbed.com/docs/mbed-os/v6.15/apis/timer.html
ull getTimeSinceStart() {
    using namespace std::chrono;
    return duration_cast<microseconds>(distanceEchoTimer.elapsed_time()).count();
}
