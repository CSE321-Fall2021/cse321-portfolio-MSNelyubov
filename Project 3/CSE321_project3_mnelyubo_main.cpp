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

//Reused from Project 2: Matrix event aliases
#define RisingEdgeInterrupt 1
#define FallingEdgeInterrupt 0

//Reused from Project 2: Associated column values with their respective character key value index in the matrix
#define ColABC 0
#define Col369 1
#define Col258 2
#define Col147 3

//LCD character positions
#define distancePosition1    13
#define distancePosition10   12
#define distancePosition100  11

//Reused from Project 2: dimension (row and column) of the Matrix keypad
#define MatrixDim 4

//Distance Sensor data
#define POLLING_HIGH_TIME     10us
#define POLLING_CYCLE_TIME_MS 100
#define DISTANCE_MINIMUM 2    /* sensor min range is stated to be 2cm */
#define DISTANCE_MAXIMUM 400  /* sensor max range is stated to be 4m  */

//data type alias
#define ull unsigned long long

//output stabilization buffer data
#define stabilizerArrayLen 4

//system state configuration
#define SetRealTime 0x0
#define SetClosingTime 0x2
#define SetMax 0x4
#define SetMin 0x6
#define Observer 0x8

//Shared variables
    int currentState = SetMax;
    Mutex currentStateRW;               //mutex order: 1

    int outputChangesMade = false;
    Mutex outputChangesMadeRW;          //mutex order: 2

    int stableDistance = 0;
    Mutex stableDistanceRWMutex;        //mutex order: 3

    char lcdOutputTextTable[][COL + 1] = {        //COL + 1 due to '\0' string suffix
        "Set current time","(24hr)  hh:mm:ss",    //output configuration for the State:  SetRealTime
        "Set closing time","(24hr)  hh:mm:ss",    //output configuration for the State:  SetClosingTime
        "[A] confirm     ","Set full:  000cm",    //output configuration for the State:  SetMax
        "[A] confirm     ","Set empty: 000cm",    //output configuration for the State:  SetMin
        "Time       Space","hh:mm:dd    nnn%"     //output configuration for the State:  SetMax
    };
    Mutex lcdOutputTableRW;             //mutex order: 4

    Mutex matrixButtonEventHandlerMutex;//mutex order: 5

    //Reused from Project 2
    char charPressed = '\0';
    int keypadVccRow = 0;
    Mutex keypadButtonPressMutex;       //mutex order: 6

    int maxDistance = DISTANCE_MAXIMUM;
    Mutex maxDistanceRW;                //mutex order: 7

    int minDistance = DISTANCE_MINIMUM;
    Mutex minDistanceRw;                //mutex order: 8

    Mutex printerMutex;                 //mutex order: last


//Internal variables exclusive to output data path: LCD
    Thread lcdRefreshThread;
    EventQueue lcdRefreshEventQueue(32 * EVENTS_EVENT_SIZE);

    Ticker lcdRefreshTicker;

    CSE321_LCD lcdObject(COL,ROW);  //create interface to control the output LCD.  Reused from Project 2
    void populateLcdOutput();
    
    void enqueueLcdRefresh();


//Internal variables exclusive to input data path: 4x4 matrix keypad
    Thread matrixThread;
    EventQueue matrixOpsEventQueue(32 * EVENTS_EVENT_SIZE);
    Ticker matrixAlternationTicker;

    void enqueueMatrixAlternation();
    void alternateMatrixInput();

    //Reused from Project 2
    char keyValues[][MatrixDim + 1] = {"dcba","#963","0852","*741"};

    //Reused from Project 2
    InterruptIn colLL(PC_0,PullDown);    //declare the connection to pin PC_0 as a source of input interrupts, connected to the far left column of the matrix keypad
    InterruptIn colCL(PC_3,PullDown);    //declare the connection to pin PC_3 as a source of input interrupts, connected to the center left column of the matrix keypad
    InterruptIn colCR(PC_1,PullDown);    //declare the connection to pin PC_1 as a source of input interrupts, connected to the center right column of the matrix keypad
    InterruptIn colRR(PC_4,PullDown);    //declare the connection to pin PC_4 as a source of input interrupts, connected to the far right column of the matrix keypad

    //Reused from Project 2: declare rising edge interrupt handler events
    void rising_isr_abc();
    void rising_isr_369();
    void rising_isr_258();
    void rising_isr_147();

    //Reused from Project 2: declare falling edge interrupt handler events
    void falling_isr_abc();
    void falling_isr_369();
    void falling_isr_258();
    void falling_isr_147();

    //Reused from Project 2: 
    void handleMatrixButtonEvent(int isRisingEdgeInterrupt, int column, int row);
    void handleInputKey(char charPressed);


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
    printf("\n\n=== System Startup ===\n");

    //create rise and fall timers for input port
    echo.rise(distanceEchoRiseHandler);
    echo.fall(distanceEchoFallHandler);

    //reused from Project 2
    colLL.rise(&rising_isr_abc);   //assign interrupt handler for a rising edge event from the column containing buttons a,b,c,d
    colCL.rise(&rising_isr_369);   //assign interrupt handler for a rising edge event from the column containing buttons 3,6,9,#
    colCR.rise(&rising_isr_258);   //assign interrupt handler for a rising edge event from the column containing buttons 2,5,8,0
    colRR.rise(&rising_isr_147);   //assign interrupt handler for a rising edge event from the column containing buttons 1,4,7,*

    //reused from Project 2
    colLL.fall(&falling_isr_abc);   //assign interrupt handler for a falling edge event from the column containing buttons a,b,c,d
    colCL.fall(&falling_isr_369);   //assign interrupt handler for a falling edge event from the column containing buttons 3,6,9,#
    colCR.fall(&falling_isr_258);   //assign interrupt handler for a falling edge event from the column containing buttons 2,5,8,0
    colRR.fall(&falling_isr_147);   //assign interrupt handler for a falling edge event from the column containing buttons 1,4,7,*

    /*************************
    *   Pin configuration    *
    *************************/

    //enable ports B,C,E
    RCC->AHB2ENR |= 0x16;

    //configure pin C9 as an output (Distance Sensor)
    GPIOC->MODER &= ~(0x80000);
    GPIOC->MODER |= 0x40000;

    //configugure GPIO pins PE2, PE4, PE5, PE6 as outputs (4x4 matrix)
    GPIOE->MODER |= 0x01510;       
    GPIOE->MODER &= ~(0x02A20);


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

    matrixThread.start(callback(&matrixOpsEventQueue, &EventQueue::dispatch_forever));
    matrixAlternationTicker.attach(&enqueueMatrixAlternation, 10ms);

    while(true){
        thread_sleep_for(1000);
    }
    return 0;
}


void enqueueMatrixAlternation(){
    matrixOpsEventQueue.call(alternateMatrixInput);
}

//Reused from Project 2
void alternateMatrixInput(){
    //if(!keypadButtonPressMutex.trylock_for(1ms)) return;    //do not alternate input if the keypad mutex is locked, indicating a button press

    if(charPressed) return;     //secondary safety to prevent repetition if the Mutex fails

    GPIOE->ODR &= ~(0x74);         //reset voltage to output 0 on all pins

    thread_sleep_for(1);    //provide window between one input going low and the next going high

    //update the row target to poll the next row
    keypadVccRow++;
    keypadVccRow%=MatrixDim;

    //Reused from Project 2
    switch (keypadVccRow){
        case 0:
            GPIOE->ODR |= 0x04;        //supply voltage to the row of keypad buttons with labels *0#D (PE_2)
            break;
        case 1:
            GPIOE->ODR |= 0x10;        //supply voltage to the row of keypad buttons with labels 789C (PE_4)
            break;
        case 2:
            GPIOE->ODR |= 0x20;        //supply voltage to the row of keypad buttons with labels 456B (PE_5)
            break;
        case 3:
            GPIOE->ODR |= 0x40;        //supply voltage to the row of keypad buttons with labels 123A (PE_6)
            break;
    }
    //keypadButtonPressMutex.unlock();
}

//Reused from Project 2
void rising_isr_abc(void) {matrixOpsEventQueue.call(handleMatrixButtonEvent, RisingEdgeInterrupt,  ColABC, keypadVccRow);}
void falling_isr_abc(void){matrixOpsEventQueue.call(handleMatrixButtonEvent, FallingEdgeInterrupt, ColABC, keypadVccRow);}
void rising_isr_369(void) {matrixOpsEventQueue.call(handleMatrixButtonEvent, RisingEdgeInterrupt,  Col369, keypadVccRow);}
void falling_isr_369(void){matrixOpsEventQueue.call(handleMatrixButtonEvent, FallingEdgeInterrupt, Col369, keypadVccRow);}
void rising_isr_258(void) {matrixOpsEventQueue.call(handleMatrixButtonEvent, RisingEdgeInterrupt,  Col258, keypadVccRow);}
void falling_isr_258(void){matrixOpsEventQueue.call(handleMatrixButtonEvent, FallingEdgeInterrupt, Col258, keypadVccRow);}
void rising_isr_147(void) {matrixOpsEventQueue.call(handleMatrixButtonEvent, RisingEdgeInterrupt,  Col147, keypadVccRow);}
void falling_isr_147(void){matrixOpsEventQueue.call(handleMatrixButtonEvent, FallingEdgeInterrupt, Col147, keypadVccRow);}


//Reused from Project 2
void handleMatrixButtonEvent(int isRisingEdgeInterrupt, int column, int row){
    // matrixButtonEventHandlerMutex.lock();

    char detectedKey = keyValues[column][row];      //fetch the char value associated with the index that was detected
    if(isRisingEdgeInterrupt){
        if(!charPressed){   //fail immediately if the mutex is already locked by another process
            charPressed = detectedKey;
            handleInputKey(charPressed);
        }
    }else{
        if(charPressed == detectedKey){
            // printerMutex.lock();
            // printf("Key released: %c\n", charPressed);
            // printerMutex.unlock();
            charPressed = '\0';                 //reset the char value to '\0' to indicate that no key is pressed
        }
    }
    // matrixButtonEventHandlerMutex.unlock();
}

//Reused from Project 2
//non-ISR function
//todo: fix Mutex ordering
void handleInputKey(char charPressed){
    // printerMutex.lock();
    // printf("Key pressed: %c\n", charPressed);
    // printerMutex.unlock();
    //TODO: handle state-based logic
    currentStateRW.lock();
    if(currentState == SetRealTime){
        //TODO
    }
    
    if(currentState == SetClosingTime){
        //TODO
    }
    
    if(currentState == SetMax){
        //TODO
    }

    if(currentState == SetMin){
        //TODO
    }

    if(currentState == Observer){
        //TODO
    }

    outputChangesMadeRW.lock();
    outputChangesMade = true;
    outputChangesMadeRW.unlock();

    currentStateRW.unlock();
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
    printf("Threaded sample measured: %d cm \tStabilized estimate: %d\tChar Pressed: %c\n", distance, updateStableDistance(), charPressed);   //documentation states divide by 58 to provide distance in CM
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
    lcdOutputTextTable[SetMax + 1][distancePosition100] = '0' + (stableDistance/100) % 10;
    lcdOutputTextTable[SetMax + 1][distancePosition10]  = '0' + (stableDistance/10)  % 10;
    lcdOutputTextTable[SetMax + 1][distancePosition1]   = '0' + (stableDistance/1)   % 10;

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
