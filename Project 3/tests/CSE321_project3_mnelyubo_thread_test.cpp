// /******************************************************************************
// *   File Name:      CSE321_project3_mnelyubo_range_test.cpp
// *   Author:         Misha Nelyubov (mnelyubo@buffalo.edu)
// *   Date Created:   11/22/2021
// *   Last Modified:  11/22/2021
// *   Purpose:        This test code verifies the expected behavior of threads, 
// *                     event queues, and mutexes.  These scheduling utilities
// *                     will be used in the main project implementation.
// *
// *   Functions:      N/A
// *
// *   Assignment:     Project 3
// *
// *   Inputs:         None
// *
// *   Outputs:        Serial printout
// *
// *   Constraints:    N/A
// *
// *   References:
// *       NUCLEO datasheet:                  https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
// *       MBED OS API: Mutex                 https://os.mbed.com/docs/mbed-os/v6.15/apis/mutex.html
// *
// ******************************************************************************/

// #include "mbed.h"
// #include <chrono>

// Thread t1;
// Thread t2;
// Thread t3;

// Mutex printer;  //create mutex for controlling access to serial output printing

// //the amount of times that each output function has been called
// int beat[3] = {0,0,0};

// Ticker tick1;
// Ticker tick2;
// Ticker tick3;

// void outputInit1();
// void outputInit2();
// void outputInit3();
// void output(int beatThread);

// EventQueue eq1(32 * EVENTS_EVENT_SIZE);
// EventQueue eq2(32 * EVENTS_EVENT_SIZE);
// EventQueue eq3(32 * EVENTS_EVENT_SIZE);

// int main(){
//     printf("== Beginning execution ==\n");
//     t1.start(callback(&eq1, &EventQueue::dispatch_forever));    //start thread 1 to process event queue 1
//     t2.start(callback(&eq2, &EventQueue::dispatch_forever));    //start thread 2 to process event queue 2
//     t3.start(callback(&eq3, &EventQueue::dispatch_forever));    //start thread 3 to process event queue 3

//     tick1.attach(&outputInit1, 1s);       //set ticker 1 to run the output initializer once every second for thread 1
//     tick2.attach(&outputInit2, 1s);       //set ticker 2 to run the output initializer once every second for thread 2
//     tick3.attach(&outputInit3, 1s);       //set ticker 3 to run the output initializer once every second for thread 3

//     while(true){}                       //run indefinitely

//     return 0;
// }

// //these ISR functions adds the time-costly thread functions to the event queue
// void outputInit1(){eq1.call(output, 1);}
// void outputInit2(){eq2.call(output, 2);}
// void outputInit3(){eq3.call(output, 3);}

// //this EventQueue-called function executes lengthier calls
// void output(int beatThread){
//     printer.lock();
//     printf("thread %d rise %d\n", beatThread, ++beat[beatThread]);
//     printer.unlock();
    
//     thread_sleep_for(500);

//     printer.lock();
//     printf("thread %d fall %d\n", beatThread, beat[beatThread]);
//     printer.unlock();
// }

