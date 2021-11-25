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
#include "1802.h"
#include <chrono>


//LCD properties
#define COL 16
#define ROW 2

//LCD character positions
#define distancePosition1    11
#define distancePosition10   10
#define distancePosition100  9


//timing behavior
#define POLLING_HIGH_TIME     10us
#define POLLING_CYCLE_TIME_MS 100

//data type alias
#define ull unsigned long long

//output stabilization buffer data
#define stabilizerArrayLen 40
#define DISTANCE_MAXIMUM 400  /* sensor max range is stated to be 4m */

//system state configuration
#define Waiting 0x0


//Shared variables
    int currentState = Waiting;
    Mutex currentStateRW;               //mutex order: 1

    int outputChangesMade = false;
    Mutex outputChangesMadeRW;          //mutex order: 2

    int stableDistance = 0;
    Mutex stableDistanceRWMutex;        //mutex order: 3

    char lcdOutputTextTable[][COL] = {
        "Distance Measur","ed:      000 cm"     //output configuration for the State:  Waiting
    };
    Mutex lcdOutputTableRW;             //mutex order: 4


    Mutex printerMutex;                 //mutex order: last

//Internal variables exclusive to output data path: LCD
    Thread lcdRefreshThread;
    EventQueue lcdRefreshEventQueue(32 * EVENTS_EVENT_SIZE);

    Ticker lcdRefreshTicker;

    CSE321_LCD lcdObject(COL,ROW);  //create interface to control the output LCD.  Reused from Project 2
    void populateLcdOutput();
    
    void enqueueLcdRefresh();


//Internal variables exclusive to input data path: Distance Sensor 
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
    *Peripheral configuration*
    *************************/
    lcdObject.begin();              //initialize LCD, reused from Project 2


    /*************************
    *  Thread configuration  *
    *************************/
    distanceSensorThread.start(callback(&distanceSensorEventQueue, &EventQueue::dispatch_forever));
    distanceSensorPollStarter.attach(&enqueuePoll, 100ms);

    lcdRefreshThread.start(callback(&lcdRefreshEventQueue, &EventQueue::dispatch_forever));
    lcdRefreshTicker.attach(&enqueueLcdRefresh, 100ms);

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

    printerMutex.lock();
    printf("Threaded sample measured: %d cm \tStabilized estimate: %d\n", distance, updateStableDistance());   //documentation states divide by 58 to provide distance in CM
    printerMutex.unlock();
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
    
    int acquiredLock = stableDistanceRWMutex.trylock();
    if(acquiredLock){
        if(stableDistance != averageDistance){
            outputChangesMadeRW.lock();
            outputChangesMade = true;
            outputChangesMadeRW.unlock();

            stableDistance = averageDistance;
        }
        stableDistanceRWMutex.unlock();
    }
    return averageDistance;
}

//Access global object timer to get the time since the process began in microseconds
//code in this function is from https://os.mbed.com/docs/mbed-os/v6.15/apis/timer.html
ull getTimeSinceStart() {
    using namespace std::chrono;
    return duration_cast<microseconds>(distanceEchoTimer.elapsed_time()).count();
}

void enqueueLcdRefresh(){
    lcdRefreshEventQueue.call(populateLcdOutput);
}

//reused from Project 2 due to modularity of code and reuse of peripheral
void populateLcdOutput(){
    currentStateRW.lock();
    outputChangesMadeRW.lock();
    stableDistanceRWMutex.lock();
    //printerMutex.lock();

    if(!outputChangesMade){
        // printerMutex.unlock();
        stableDistanceRWMutex.unlock();
        outputChangesMadeRW.unlock();
        currentStateRW.unlock();
        return;
    }
    outputChangesMade = false;

    //update capacity distance in state
    lcdOutputTextTable[Waiting + 1][distancePosition100] = '0' + (stableDistance/100) % 10;
    lcdOutputTextTable[Waiting + 1][distancePosition10]  = '0' + (stableDistance/10)  % 10;
    lcdOutputTextTable[Waiting + 1][distancePosition1]   = '0' + (stableDistance/1)   % 10;

    //refresh each line of the LCD display
    for(char line = 0; line < ROW; line++){
        char* printVal = lcdOutputTextTable[currentState + line];   //retrieve the string associated with the current line of the LCD
        
        //printf("Refreshing line %d of LCD with text: %s\n", line, printVal);
        

        lcdObject.setCursor(0, line);                       //reset cursor to position 0 of the line to be written to
        lcdObject.print(printVal);                          //send a print request to configure the text of the line
    }

    // printf("Unlocking mutexes\n");

    // printerMutex.unlock();
    stableDistanceRWMutex.unlock();
    outputChangesMadeRW.unlock();
    currentStateRW.unlock();
}
