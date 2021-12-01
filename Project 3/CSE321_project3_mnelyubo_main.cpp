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
#define RisingEdgeInterrupt true
#define FallingEdgeInterrupt false

//Reused from Project 2: Associated column values with their respective character key value index in the matrix
#define ColABC 0
#define Col369 1
#define Col258 2
#define Col147 3

//LCD character positions
#define distancePosition100  11
#define distancePosition10   12
#define distancePosition1    13

//string indexes of the Observer mode for informing a user of the currently used capacity of the container
#define percentPosition100 0
#define percentPosition10  1
#define percentPosition1   2

//string indexes of the SetRealTIme, SetClosingTime, and Observer modes indicating where the time string is stored
#define timeInputHours01 9
#define timeInputHours10 8
#define timeInputMins01 12
#define timeInputMins10 11
#define timeInputSecs01 15
#define timeInputSecs10 14

#define alarmIndicatorPosition 7
#define alarmIndicatorArmed '#'
#define alarmIndicatorOff ' '


//Reused from Project 2: dimension (row and column) of the Matrix keypad
#define MatrixDim 4

//Reused from Project 2: keypad bounce timeout window (ms)
#define bounceTimeoutWindow 100

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
#define SetRealTime    0x0
#define SetClosingTime 0x2
#define SetMax         0x4
#define SetMin         0x6
#define Observer       0x8

//Shared variables
    /**************************************************************************
    *                               Mutex Order                               *
    * Mutexes must be locked in ascending order and unlocked in descending    * 
    *  order to avoid any potential cases of Deadlock.  The mutex ordering    *
    *  number (e.g. (3)) is listed with each declared mutex and must be       *
    *  included on every lock and unlock mutex call to ensure that operations *
    *  proceed without unrecoverable conflicts.                               *
    **************************************************************************/
    //Reused from Project 2:
    int bounceLockout = 0;              //set to a value greater than zero whenever a button press is detected in order to lock out duplicate button presses for a brief period
    Mutex bounceHandlerMutex;           //mutex order: (0)

    int currentState = SetRealTime;     //the current state of the system
    Mutex currentStateRW;               //mutex order: (1)

    int outputChangesMade = false;      //indicates if there have been any changes made that would require a change to the output display
    Mutex outputChangesMadeRW;          //mutex order: (2)

    int stableDistance = 0;             //the stabilized distance from an average of multiple polls by the distance sensor
    Mutex stableDistanceRWMutex;        //mutex order: (3)

    //a table of output values to display on the LCD matrix during any given state
    char lcdOutputTextTable[][COL + 1] = {        //COL + 1 due to '\0' string suffix
        "Set current time","(24hr)  hh:mm:ss",    //output configuration for the State:  SetRealTime
        "Set closing time","(24hr)  hh:mm:ss",    //output configuration for the State:  SetClosingTime
        "[A] confirm     ","Set empty: 000cm",    //output configuration for the State:  SetMax
        "[A] confirm     ","Set full:  000cm",    //output configuration for the State:  SetMin
        "Space       Time","nnn%    hh:mm:ss"     //output configuration for the State:  Observer
    };
    Mutex lcdOutputTableRW;             //mutex order: (4)

    int maxDistance = DISTANCE_MAXIMUM; //The maximum distance detected by the distance sensor.  
                                        //Once configured, the stable distance value equaling this value indicates that the container is currently emptied.
                                        //defaults to maximum distance that can be detected by the distance sensor, 4m.
    Mutex maxDistanceRW;                //mutex order: (5)

    int minDistance = DISTANCE_MINIMUM; //the minimum distance detected by the distance sensor.
                                        //Once configured, the stable distance value equaling this value indicates that the container is full.
                                        //defaults to minimum distance that can be detected by the distance sensor, 2cm.
    Mutex minDistanceRW;                //mutex order: (6)

    int alarmArmed = false;             //indicates if the alarm should sound when the container is not empty after closing time
    Mutex alarmArmedRW;                 //mutex order: (7)


//Internal variables exclusive to output data path: LCD
    Thread outputRefreshThread;                                    //thread to execute output modification functions that cannot be handled in an ISR context
    EventQueue outputModificationEventQueue(32 * EVENTS_EVENT_SIZE);    //queue of events that must be handled by the lcdRefreshThread

    Ticker outputRefreshTicker;                                    //periodically enqueues an event into the lcdRefresh queue to update the contents of the LCD output

    CSE321_LCD lcdObject(COL,ROW);                              //create interface to control the output LCD.  Reused from Project 2
    void populateLcdOutput();                                   //non-ISR function that will update the contents of the LCD output and toggle the state of the buzzer as appropriate
    bool closingTimeCrossed();                                  //checks if the current time is later than the closing time.  returns true if this is the case
    
    void enqueueOutputRefresh();                                   //helper function to enqueue a refresh of the LCD for the lcdRefreshThread to execute

    Ticker rtClockHandler;                                      //ticker that will periodically enqueue an event increment real-time clock once it is input every second
    void enqueueRTClockTick();                                  //helper event that enqueues an incrementation of the real-time clock
    void tickRealTimeClock();                                   //non-ISR function that will increment the real-time clock and handle numeric roll-over

    DigitalOut alarm_Vcc(PB_10);    //starts off with 0V. power to alarm disabled until the alarm state has been set to inactive
    DigitalOut alarm_L(PB_11);      //starts off with 0V. active low component that produces a noise when active

//Internal variables exclusive to input data path: 4x4 matrix keypad
    Thread matrixThread;                                    //thread to execute handler functions triggered by user interaction with the matrix keypad
    EventQueue matrixOpsEventQueue(32 * EVENTS_EVENT_SIZE); //queue of events for the matrix thread to execute
    Ticker matrixAlternationTicker;                         //ticker that will create periodic matrix events to alternate the channel being polled

    void enqueueMatrixAlternation();                        //helper function to enqueue alternator function alternateMatrixInput
    void alternateMatrixInput();                            //alternates matrix poll channel when no button is pressed

    //Reused from Project 2
    char charPressed = '\0';            //the character on the matrix that is currently pressed
    int keypadVccRow = 0;               //the output into the keypad that is currently being supplied a voltage
    char keyValues[][MatrixDim + 1] = {"dcba","#963","0852","*741"};       //a 2D character array mapping keypad row/column indexes to their representative characters

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
    void handleMatrixButtonEvent(bool isRisingEdgeInterrupt, int column, int row);   //filter duplicates and parse matrix button events into input key events
    void handleInputKey(char charPressed);                                          //handle input keys based on the current state of the system

    int timeInputPositions[] = {    //an array of the defined time value positions for use by iteration during user input 
        timeInputHours10,
        timeInputHours01,
        timeInputMins10,
        timeInputMins01,
        timeInputSecs10,
        timeInputSecs01
    };
    int timeInputIndex = 0;         //the current index through the timeInputPositions array.  Accessed solely by the function handleInputKey

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


//beginning of main execution
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

    alarm_L.write(1);       //Disable active low alarm
    alarm_Vcc.write(1);     //Enable power to alarm

    /*************************
    *Peripheral configuration*
    *************************/
    lcdObject.begin();              //initialize LCD, reused from Project 2


    /*************************
    *  Thread configuration  *
    *************************/
    distanceSensorThread.start(callback(&distanceSensorEventQueue, &EventQueue::dispatch_forever));    //set the distance sensor thread to continuously execute anything in the distance sensor event queue
    distanceSensorPollStarter.attach(&enqueuePoll, 100ms);                  //set the distance sensor poll starting ticker to enqueue a poll of the distance every 100ms

    outputRefreshThread.start(callback(&outputModificationEventQueue, &EventQueue::dispatch_forever)); //set the LCD and alarm refresh thread to continously execute anything in the output modification event queue
    outputRefreshTicker.attach(&enqueueOutputRefresh, 100ms);               //set the output refresh starting ticker to enqueue an output refresh every 100 ms
    rtClockHandler.attach(&enqueueRTClockTick, 1s);                         //set the real-world clock time ticker to enqueue an increment ever second (executed in the output modifications event queue)

    matrixThread.start(callback(&matrixOpsEventQueue, &EventQueue::dispatch_forever));  //set the matrix I/O thread to continously execute anything in the matrix operations event queue
    matrixAlternationTicker.attach(&enqueueMatrixAlternation, 10ms);        //set the output to matrix alternation ticker to enqueue an alternation every 10ms

    
    while(true){ //Idle on main thread to prevent program from exiting
        thread_sleep_for(1);                    //timeout thread for 1ms every cycle with no mutexes locked.
        bounceHandlerMutex.lock();              //(0) lock the bounce handler mutex to avoid concurrent R/W operations from another thread
        if(bounceLockout > 0) bounceLockout--;  //decrement the bounce lockout if it is still greater than 0 after a recent rising edge button press
        bounceHandlerMutex.unlock();            //(0) unlock the bounce handler immediately after the critical section is exited
    }
    return 0;
}


/**
* void alternateMatrixInput()
* non-ISR Function
* Reused from Project 2
*
* Summary of the function:
*    This function alternates which column of the matrix receives voltage to bounce back a signal on the input pins that are expecting interrupt events.
*    This function only switches matrix columns if no character is currently pressed.
*
* Parameters:   
*    None
*
* Return value:
*    None
*
* Outputs:
*    Exactly one digital high DC output signal between pins PE_2, PE_4, PE_5, and PE_6 will be sent at the end of the execution of this function
*    During the execution of the function, a brief period (~1ms) may exist where none of the four channels are given a non-zero voltage.
*
* Shared variables accessed:
*    None.  Global variable charPressed is exclusive to operations executing on the matrix operations thread, which can only be one function at any point in time, so a mutex is not required.
*
* Helper ISR Function:
*    enqueueMatrixAlternation
*/
void alternateMatrixInput(){
    if(charPressed) return;     //secondary safety to prevent repetition if the Mutex fails

    GPIOE->ODR &= ~(0x74);      //reset voltage to output 0 on all four alternating pins

    thread_sleep_for(1);        //provide a 1ms window between one input going low and the next going high

    keypadVccRow = (1 + keypadVccRow) % MatrixDim;    //update the row target to poll the next row

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
}
//helper function
void enqueueMatrixAlternation(){matrixOpsEventQueue.call(alternateMatrixInput);}


/**
* void handleMatrixButtonEvent
* Reused from Project 2
* non-ISR function
*
* Summary of the function:
*    This function converts any event triggered by a matrix button input into a character, handles duplicate events due to bounce, and calls handleInputKey on the rising edge of filtered results.
*
* Parameters:   
*    - isRisingEdgeInterrupt - boolean indicating whether the trigger event is a rising or falling edge of a button press
*    - column                - integer 0-3 indicating the matrix column of the input button press, used to map to the proper key value
*    - row                   - integer 0-3 indicating the matrix row of the input button press, used to map to the proper key value
*
* Return value:
*    None
*
* Outputs:
*    handleInputKey called to modify system state based on button press.
*    
* Shared variables accessed:
*    bounce lockout  -  mutex (8)
*
* Helper ISR Functions:
*    rising_isr_abc
*    falling_isr_abc
*    rising_isr_369
*    falling_isr_369
*    rising_isr_258
*    falling_isr_258
*    rising_isr_147
*    falling_isr_147
*
*/
void handleMatrixButtonEvent(bool isRisingEdgeInterrupt, int column, int row){
    char detectedKey = keyValues[column][row];      //fetch the char value associated with the index that was detected
    if(isRisingEdgeInterrupt){
        bounceHandlerMutex.lock();                //(0) lock bounce handler mutex to avoid concurrent access
        if(!charPressed && bounceLockout == 0){   //fail immediately if another key is pressed or was pressed recently
            bounceLockout = bounceTimeoutWindow;  //ensure no additional events are acted upon
            charPressed = detectedKey;            //set the global variable charPressed to the detected key press
            handleInputKey(charPressed);          //(0) call the function to handle the input key without unlocking the mutex
        }
        bounceHandlerMutex.unlock();              //unlock the bounce handler mutex only after all updates to system state have been completed
    }else{
        if(charPressed == detectedKey){
            charPressed = '\0';                 //reset the char value to '\0' to indicate that no key is pressed
        }
    }
}

//Helper Functions:
//Reused from Project 2
// Handle interrupts by enqueueing matrix operation queue events to address the cause of the interrupt in a non-ISR context
void rising_isr_abc(void) {matrixOpsEventQueue.call(handleMatrixButtonEvent, RisingEdgeInterrupt,  ColABC, keypadVccRow);}
void falling_isr_abc(void){matrixOpsEventQueue.call(handleMatrixButtonEvent, FallingEdgeInterrupt, ColABC, keypadVccRow);}
void rising_isr_369(void) {matrixOpsEventQueue.call(handleMatrixButtonEvent, RisingEdgeInterrupt,  Col369, keypadVccRow);}
void falling_isr_369(void){matrixOpsEventQueue.call(handleMatrixButtonEvent, FallingEdgeInterrupt, Col369, keypadVccRow);}
void rising_isr_258(void) {matrixOpsEventQueue.call(handleMatrixButtonEvent, RisingEdgeInterrupt,  Col258, keypadVccRow);}
void falling_isr_258(void){matrixOpsEventQueue.call(handleMatrixButtonEvent, FallingEdgeInterrupt, Col258, keypadVccRow);}
void rising_isr_147(void) {matrixOpsEventQueue.call(handleMatrixButtonEvent, RisingEdgeInterrupt,  Col147, keypadVccRow);}
void falling_isr_147(void){matrixOpsEventQueue.call(handleMatrixButtonEvent, FallingEdgeInterrupt, Col147, keypadVccRow);}





//todo: add Watchdog for when a button press is held for longer than 60 seconds
/**
* void handleInputKey()
* non-ISR Function
* Reused from Project 2
*
* Summary of the function:
*    This function modifies the system state and internal variables based on the user input to the system.
*    While in the SetRealTime and SetClosingTime states,
*      Numeric inputs are used to configure the two respective times.
*      A is used to confirm the current input, defaulting all unentered positions to 0.
*      C is used to clear the current input time and begin again from the tens of hours.
*
*    While in the SetMax and SetMin states,
*      A is used to lock in the current distance measured by the distance sensor for that particular mode.
*    
*   While in the Observer state,
*      # is used to toggle the alarm being armed or not.
*
*    While in any state,
*      D is used to reset the system to the SetRealTime state to reconfigure the system.
*
* Parameters:   
*    - charPressed - an ASCII character value indicating the user input, based on which to modify the system state or variables
*
* Return value:
*    None
*
* Outputs:
*    None directly.  The configuration of the alarm and LCD outputs may be modified due to calling this function.
*
* Shared variables accessed:
*    currentState       - mutex (1)
*    outputChangesMade  - mutex (2)
*    stableDistance     - mutex (3)
*    lcdOutputTextTable - mutex (4)
*    maxDistance        - mutex (5)
*    minDistance        - mutex (6)
*    alarmArmed         - mutex (7)
*
* Helper ISR Function:
*    no direct helper.  Dependent on handleMatrixButtonEvent
*/

//MACRO to update an input number that will work as a timestamp.
//Due to the variable scope of incrementInputIndex, this set of frequently called lines cannot effectively be a function.
#define updateTimeData  lcdOutputTableRW.lock();    /*(4) lock the LCD output table mutex before modifying values in the table*/      \
                        lcdOutputTextTable[entryState + 1][timeInputPositions[timeInputIndex]] = charPressed; /*modify table values*/ \
                        lcdOutputTableRW.unlock();  /*(4) unlock the table mutex as soon as the operation is completed*/              \
                        incrementInputIndex=true;   /*indicate that the input index must be updated*/

void handleInputKey(char charPressed){
    currentStateRW.lock();              //(1)
    int entryState = currentState;      //act based on the system state preceeding the button press to avoid rollover
    if(entryState == SetRealTime){
        lcdOutputTableRW.lock();            //(4)
        switch(charPressed){
            case 'a':               //switch to next state and filter input of the current state
                timeInputIndex = 0; //reset the index of the next button to be updated to 0
                currentState = SetClosingTime;  //set the system state to setting the closing time

                //iterate over the time input and replace any remaining 'h','m', and 's' characters with '0'
                for(int i = timeInputHours10; i <= timeInputSecs01; i++){   //iterate over the confirmed input time of the SetRealTime state output string
                    if(lcdOutputTextTable[entryState + 1][i] > ':'){        //fix any unset characters (where the value is still h,m,s (all ASCII values greater than ':'))
                        lcdOutputTextTable[entryState + 1][i] = '0';        //by setting those unset characters to 0
                    }
                }
                
                break;
            
            case 'c':               //reset time input when the button C is pressed
                timeInputIndex = 0; //reset the index of the next button to be updated to 0
                lcdOutputTextTable[entryState + 1][timeInputHours10] = 'h'; //reset the value of tens of hours to h
                lcdOutputTextTable[entryState + 1][timeInputHours01] = 'h'; //reset the value of hours to h
                lcdOutputTextTable[entryState + 1][timeInputMins10] = 'm';  //reset the value of tens of minutes to m
                lcdOutputTextTable[entryState + 1][timeInputMins01] = 'm';  //reset the value of minutes to m
                lcdOutputTextTable[entryState + 1][timeInputSecs10] = 's';  //reset the value of tens of seconds to s
                lcdOutputTextTable[entryState + 1][timeInputSecs01] = 's';  //reset the value of seconds to s
                break;

            case '1': //Handle all number inputs in the same manner:
            case '2': //Update the time input of the state
            case '3': //if the input number is valid for the current time index
            case '4': //otherwise take no action
            case '5': 
            case '6': 
            case '7':
            case '8':
            case '9':
            case '0':
                bool incrementInputIndex = false;    //indicate if a valid button press was detected and the next time index should be switched to
                int timeInputPosition = timeInputPositions[timeInputIndex]; //the current index of the time input position to update

                //repeat for each time input position.  This cannot be a single for loop due to the variance of rollover conditions for each digit
                //if the current time input position matches the time input position of this time unit, make sure that the
                // input number does not break the conditions required for the time to remain valid on a 24 hour clock

                if(timeInputPosition == timeInputHours10){
                    if(charPressed < '3'){  //tens of hours cannot exceed 2
                        updateTimeData;
                    }
                }

                if(timeInputPosition == timeInputHours01){
                    if(lcdOutputTextTable[entryState + 1][timeInputHours10] < '3' && charPressed < '4'  ||   //hours cannot exceed 23 (23:59:59 is followed by 00:00:00)
                       lcdOutputTextTable[entryState + 1][timeInputHours10] < '2'){                            //hours can reach 19:00:00 and 09:00:00
                        updateTimeData;
                    }
                }

                if(timeInputPosition == timeInputMins10){
                    if(charPressed < '6'){  //minutes cannot exceed 59
                        updateTimeData;
                    }
                }

                if(timeInputPosition == timeInputMins01){
                    updateTimeData;         //the entire range of 0-9 is valid for single minutes
                }

                if(timeInputPosition == timeInputSecs10){
                    if(charPressed < '6'){  //seconds cannot exceed 59
                        updateTimeData;
                    }

                }

                if(timeInputPosition == timeInputSecs01){
                    updateTimeData;         //the entire range of 0-9 is valid for single seconds
                }

                if(incrementInputIndex) timeInputIndex++;   //this is done at the end to prevent a cascade of multiple time indexes being updated due to a number input that is valid for all of them

                break;
        }
        lcdOutputTableRW.unlock();          //(4)
    }
    
    if(entryState == SetClosingTime){
        lcdOutputTableRW.lock();            //(4)
        switch(charPressed){
            case 'a':               //switch to next state and filter input of the current state
                timeInputIndex = 0; //reset the index of the next button to be updated to 0
                currentState = SetMax; //set the system state to configuring the maximum distance within the sensor range

                //iterate over the time input and replace any remaining 'h','m', and 's' characters with '0'
                for(int i = timeInputHours10; i <= timeInputSecs01; i++){   //iterate over the confirmed input time of the SetRealTime state output string
                    if(lcdOutputTextTable[entryState + 1][i] > ':'){        //fix any unset characters (where the value is still h,m,s (all ASCII values greater than ':'))
                        lcdOutputTextTable[entryState + 1][i] = '0';        //by setting those unset characters to 0
                    }
                }
                
                break;
            case 'c':               //reset time input when the button C is pressed
                timeInputIndex = 0; //reset the index of the next button to be updated to 0
                lcdOutputTextTable[entryState + 1][timeInputHours10] = 'h'; //reset the value of tens of hours to h
                lcdOutputTextTable[entryState + 1][timeInputHours01] = 'h'; //reset the value of hours to h
                lcdOutputTextTable[entryState + 1][timeInputMins10] = 'm';  //reset the value of tens of minutes to m
                lcdOutputTextTable[entryState + 1][timeInputMins01] = 'm';  //reset the value of minutes to m
                lcdOutputTextTable[entryState + 1][timeInputSecs10] = 's';  //reset the value of tens of seconds to s
                lcdOutputTextTable[entryState + 1][timeInputSecs01] = 's';  //reset the value of seconds to s
                break;

            case '1': //Handle all number inputs in the same manner:
            case '2': //Update the time input of the state
            case '3': //if the input number is valid for the current time index
            case '4': //otherwise take no action
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
                bool incrementInputIndex = false;    //indicate if a valid button press was detected and the next time index should be switched to
                int timeInputPosition = timeInputPositions[timeInputIndex]; //the current index of the time input position to update
 
                //repeat for each time input position.  This cannot be a single for loop due to the variance of rollover conditions for each digit
                //if the current time input position matches the time input position of this time unit, make sure that the
                // input number does not break the conditions required for the time to remain valid on a 24 hour clock

                if(timeInputPosition == timeInputHours10){
                    if(charPressed < '3'){  //tens of hours cannot exceed 2
                        updateTimeData;
                    }
                }

                if(timeInputPosition == timeInputHours01){
                    if(lcdOutputTextTable[entryState + 1][timeInputHours10] < '3' && charPressed < '4'  ||   //hours cannot exceed 23 (23:59:59 is followed by 00:00:00)
                       lcdOutputTextTable[entryState + 1][timeInputHours10] < '2'){                            //hours can reach 19:00:00 and 09:00:00
                        updateTimeData;
                    }
                }

                if(timeInputPosition == timeInputMins10){
                    if(charPressed < '6'){  //minutes cannot exceed 59
                        updateTimeData;
                    }
                }

                if(timeInputPosition == timeInputMins01){
                    updateTimeData;         //the entire range of 0-9 is valid for single minutes
                }

                if(timeInputPosition == timeInputSecs10){
                    if(charPressed < '6'){  //seconds cannot exceed 59
                        updateTimeData;
                    }

                }

                if(timeInputPosition == timeInputSecs01){
                    updateTimeData;         //the entire range of 0-9 is valid for single seconds
                }

                if(incrementInputIndex) timeInputIndex++;   //this is done at the end to prevent a cascade of multiple time indexes being updated due to a number input that is valid for all of them

                break;
        }
        lcdOutputTableRW.unlock();          //(4)
    }

    if(entryState == SetMax){
        switch(charPressed){
            case 'a':   //set the maximum distance from the sensor (empty container) equal to the stabilized distance at the time that the button was pressed
                stableDistanceRWMutex.lock();   //(3)
                maxDistanceRW.lock();           //(5)
                maxDistance = stableDistance;   //critical section: set maximum distance equal to stable distance
                maxDistanceRW.unlock();         //(5)
                stableDistanceRWMutex.unlock(); //(3)
                currentState = SetMin;          //with the maximum distance set, switch to the next state for setting the minimum distance (full container)
                break;
        }
    }

    if(entryState == SetMin){
        switch(charPressed){
            case 'a':       //lock in the minimum distance and switch to the observing state
                stableDistanceRWMutex.lock();    //(3)
                minDistanceRW.lock();            //(6)
                alarmArmedRW.lock();             //(7)
                
                minDistance = stableDistance;    //set the minimum distance (full container) to the current stabilized distance measurement
                currentState = Observer;         //switch to the observer mode to monitor for the conditions required to trigger the alarm
                alarmArmed = true;               //arm the alarm to be activated when the necessary trigger conditions are met

                alarmArmedRW.unlock();           //(7)
                minDistanceRW.unlock();          //(6)
                stableDistanceRWMutex.unlock();  //(3)
                break;
        }
    }

    if(entryState == Observer){
        switch (charPressed) {
        case '#':       //toggle state of alarm between armed and off
            alarmArmedRW.lock();       //(7)
            alarmArmed = !alarmArmed;
            alarmArmedRW.unlock();     //(7)
            break;
        }
    }

    //todo: continue documentation from here
    //Inputs Independent of State:
    switch(charPressed){
        case 'd':       //return to setup, deactivate the alarm until setup completes
            currentState = SetRealTime;     //return to state SetRealTime
            timeInputIndex = 0;             //reset edit cursor to 10's of hours, but do not clear stored data
            alarmArmedRW.lock();       //(7)
            alarmArmed = false;        //disable the alarm while not in Observer mode
            alarmArmedRW.unlock();     //(7)
            break;
    }

    outputChangesMadeRW.lock();     //(2)
    outputChangesMade = true;       //after any button press, raise the mutex-protected flag indicating that changes to the output have been made and require an output refresh
    outputChangesMadeRW.unlock();   //(2)
    currentStateRW.unlock();        //(1) end of critical section where the current state of the system may be modified and lower-level mutexes
}


void enqueuePoll(){distanceSensorEventQueue.call(pollDistanceSensor);}
//non-ISR function
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

    printf("Threaded sample measured: %d cm \tStabilized estimate: %d cm \tChar Pressed: %c\n", distance, updateStableDistance(), charPressed);   //documentation states divide by 58 to provide distance in CM
    riseEchoTimestamp=false;
    fallEchoTimestamp=false;

    distanceEchoTimer.stop();
    distanceEchoTimer.reset();  //reset the timer for the next run

}

//ISR function to immediately handle falling edge of distance scan
void distanceEchoFallHandler(){
    fallEchoTimestamp=getTimeSinceStart();
    distanceSensorEventQueue.call(processDistanceData);
}

//ISR function to immediately handle rising edge of distance scan
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
    
    bool acquiredLock = stableDistanceRWMutex.trylock();  //(3)
    if(acquiredLock){
        if(stableDistance != averageDistance){
            outputChangesMadeRW.lock();          //(2)
            outputChangesMade = true;
            outputChangesMadeRW.unlock();        //(2)

            stableDistance = averageDistance;
        }
        stableDistanceRWMutex.unlock();       //(3)
    }
    return averageDistance;
}

//Access global object timer to get the time since the process began in microseconds
//code in this function is from https://os.mbed.com/docs/mbed-os/v6.15/apis/timer.html
ull getTimeSinceStart() {
    using namespace std::chrono;
    return duration_cast<microseconds>(distanceEchoTimer.elapsed_time()).count();
}


//ISR function called by ticker to enqueue main function: tickRealTimeClock
void enqueueRTClockTick(){outputModificationEventQueue.call(tickRealTimeClock);}
void tickRealTimeClock(){
    currentStateRW.lock();          //(1)
    outputChangesMadeRW.lock();     //(2)
    lcdOutputTableRW.lock();        //(4)
    outputChangesMade = true;

    if(currentState != SetRealTime){    //only update the clock when not setting the clock
        if(lcdOutputTextTable[SetRealTime + 1][timeInputSecs01]++ >= '9'){      //increment seconds up to the limit of 9
            lcdOutputTextTable[SetRealTime + 1][timeInputSecs01] = '0';

            if(lcdOutputTextTable[SetRealTime + 1][timeInputSecs10]++ >= '5'){  //only occurs when seconds were previously :59
                lcdOutputTextTable[SetRealTime + 1][timeInputSecs10] = '0';
                //handle minutes
                if(lcdOutputTextTable[SetRealTime + 1][timeInputMins01]++ >= '9'){  //increment minutes up to the limit of 9
                    lcdOutputTextTable[SetRealTime + 1][timeInputMins01] = '0';

                    if(lcdOutputTextTable[SetRealTime + 1][timeInputMins10]++ >= '5'){  //only occurs when minutes were previously :59
                        lcdOutputTextTable[SetRealTime + 1][timeInputMins10] = '0';

                        //handle hours
                        if(lcdOutputTextTable[SetRealTime + 1][timeInputHours01]++ >= '9'){ //increment hours up to the generic limit of 9
                            lcdOutputTextTable[SetRealTime + 1][timeInputHours01]='0';
                            lcdOutputTextTable[SetRealTime + 1][timeInputHours10]++;

                            //handle day rollover when hours = "24" by reseting to "00"
                            if(lcdOutputTextTable[SetRealTime + 1][timeInputHours01] == '4' && lcdOutputTextTable[SetRealTime + 1][timeInputHours01] == '2'){
                               lcdOutputTextTable[SetRealTime + 1][timeInputHours01] = '0';
                               lcdOutputTextTable[SetRealTime + 1][timeInputHours10] = '0'; 
                            }
                        }
                    }
                }
            }
        }
    }

    //put new value of SetRealTime
    for(int i = timeInputHours10; i <= timeInputSecs01; i++){
        lcdOutputTextTable[Observer + 1][i] = lcdOutputTextTable[SetRealTime + 1][i];
    }

    lcdOutputTableRW.unlock();     //(4)
    outputChangesMadeRW.unlock();  //(2)
    currentStateRW.unlock();       //(1)
}

void enqueueOutputRefresh(){outputModificationEventQueue.call(populateLcdOutput);}
//reused from Project 2 due to modularity of code and reuse of peripheral
void populateLcdOutput(){
    currentStateRW.lock();          //(1)
    outputChangesMadeRW.lock();     //(2)
    stableDistanceRWMutex.lock();   //(3)
    lcdOutputTableRW.lock();        //(4)
    maxDistanceRW.lock();           //(5)
    minDistanceRW.lock();           //(6)

    if(!outputChangesMade){
        minDistanceRW.unlock();          //(6)
        maxDistanceRW.unlock();          //(5)
        lcdOutputTableRW.unlock();       //(4)
        stableDistanceRWMutex.unlock();  //(3)
        outputChangesMadeRW.unlock();    //(2)
        currentStateRW.unlock();         //(1)
        return;
    }
    outputChangesMade = false;

    //update capacity distance in min/max states
    if(currentState == SetMax || currentState == SetMin){
        lcdOutputTextTable[currentState + 1][distancePosition100] = '0' + (stableDistance/100) % 10;
        lcdOutputTextTable[currentState + 1][distancePosition10]  = '0' + (stableDistance/10)  % 10;
        lcdOutputTextTable[currentState + 1][distancePosition1]   = '0' + (stableDistance/1)   % 10;
    }


    int spaceValue;
    //update the value of the percent of space used in the Observer State
    if(maxDistance != minDistance){
        spaceValue = 100 * (maxDistance - stableDistance);
        spaceValue = spaceValue / (maxDistance - minDistance);

        if(spaceValue < 0) spaceValue = 0;  //set a hard limit of 0% full in case the container moves

        lcdOutputTextTable[Observer + 1][percentPosition100] = '0' + (spaceValue/100) % 10;
        lcdOutputTextTable[Observer + 1][percentPosition10]  = '0' + (spaceValue/10)  % 10;
        lcdOutputTextTable[Observer + 1][percentPosition1]   = '0' + (spaceValue/1)   % 10;
    }else{
        lcdOutputTextTable[Observer + 1][percentPosition100] = 'N';
        lcdOutputTextTable[Observer + 1][percentPosition10]  = '/';
        lcdOutputTextTable[Observer + 1][percentPosition1]   = '0';
    }

    //update the state of the alarm
    bool activateAlarm = false;
    alarmArmedRW.lock();     //(7)
    if(alarmArmed){
        lcdOutputTextTable[Observer + 1][alarmIndicatorPosition] = alarmIndicatorArmed;
        if(closingTimeCrossed() && spaceValue > 0){
            activateAlarm = true;
        }
    }else{
        lcdOutputTextTable[Observer + 1][alarmIndicatorPosition] = alarmIndicatorOff;
    }
    alarmArmedRW.unlock();   //(7)

    if(activateAlarm){
        //send signal low to active low alarm pin (PB_11)
        alarm_L.write(0);
    }else{
        //send signal high to active low alarm pin (PB_11)
        alarm_L.write(1);
    }

    //refresh each line of the LCD display
    for(char line = 0; line < ROW; line++){
        char* printVal = lcdOutputTextTable[currentState + line];   //retrieve the string associated with the current line of the LCD
        lcdObject.setCursor(0, line);                       //reset cursor to position 0 of the line to be written to
        lcdObject.print(printVal);                          //send a print request to configure the text of the line
    }

    minDistanceRW.unlock();           //(6)
    maxDistanceRW.unlock();           //(5)
    lcdOutputTableRW.unlock();        //(4)
    stableDistanceRWMutex.unlock();   //(3)
    outputChangesMadeRW.unlock();     //(2)
    currentStateRW.unlock();          //(1)
}


//returns true if the current time is greater than closing time.  Assumes that the calling thread has locked the lcdOutputTextTable mutex
bool closingTimeCrossed(){

    for(int i = timeInputHours10; i<=timeInputSecs01; i++){
        //compare for a greater quantity of unit time
        if(lcdOutputTextTable[SetRealTime + 1][i] > lcdOutputTextTable[SetClosingTime + 1][i]) return true;       //current unit time is greater than closing unit time
        if(lcdOutputTextTable[SetRealTime + 1][i] < lcdOutputTextTable[SetClosingTime + 1][i]) return false;      //current unit time is less than closing unit time
        //all remaining possibilities must have this unit of time equal and can thus be ignored
    }

    //times are exactly equal if the for loop has been escaped.  Return false since it has not yet been *crossed*
    return false;
}