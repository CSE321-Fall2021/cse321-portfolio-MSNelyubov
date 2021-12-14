# About
Project Description:
This project tracks the design and development of a used-volume monitor for a container to notify when there is still food present at the end of a work day.  The objective of the project is to minimize food waste by alerting staff of leftover food that can be taken home before leaving work.  Modular elements from Project 2 are reused in this project to control previously implemented peripherals.


Contribitor List:
- Misha Nelyubov (mnelyubo@buffalo.edu)
- Dr. Jennifer Winikus (jwinikus@buffalo.edu)


# Features
The code in this repository will execute on a Nucleo L4R5ZI to control an embedded system in order to
- Measure and report the used space of a container as a function of the the distance between the base and top of the container
- Report if there is food left over inside of the container at the end of a work day
- Provide a user interface to input the current time and closing time after which to alert staff


# Bill of Materials
- NUCLEO L4R5ZI
	- https://www.mouser.com/ProductDetail/511-NUCLEO-L4R5ZI
- 4x4 Matrix Keypad
	- https://www.amazon.com/dp/B07THCLGCZ
- LCD
	- JHD1804 - https://www.mouser.com/ProductDetail/713-104020111
- Distance Sensor
	- SainSmart HC-SR04 - https://www.amazon.com/dp/B004U8TOE6
- Buzzer Alarm Sound Module
	- https://www.amazon.com/dp/B07MPYWVGD
- Breadboard
- Jumper wires
- Container for food
- Food (or substitute objects while testing) for the container


# Resources and References
- NUCLEO datasheet:
	- https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- HC-SR04 distance sensor datasheet:
	- https://www.digikey.com/htmldatasheets/production/1979760/0/0/1/hc-sr04.html
- Buzzer datasheet:
	- https://www.mouser.com/datasheet/2/400/ef532_ps-13444.pdf
 - MBED OS API: Timer
 	- https://os.mbed.com/docs/mbed-os/v6.15/apis/timer.html
 - MBED OS API: Watchdog
 	- https://os.mbed.com/docs/mbed-os/v6.15/apis/watchdog.html


# Getting Started
1. Connect the Nucleo to the input and output peripherals through the breadboard
    - Connect power supply lanes
        - Connect a Nucleo pin labeled GND to the - bus of the breadboard
        - Connect a Nucleo pin labeled 5V (VCC) to the + bus of the breadboard
    - Connect the Matrix keypad to the NUCLEO
    	- Connect the NUCLEO pin A1 (PC_0) to pin 1 of the Matrix Keypad 
    	- Connect the NUCLEO pin A2 (PC_3) to pin 2 of the Matrix Keypad 
    	- Connect the NUCLEO pin A3 (PC_1) to pin 3 of the Matrix Keypad 
    	- Connect the NUCLEO pin A4 (PC_4) to pin 4 of the Matrix Keypad 
    	- Connect the NUCLEO pin PE_2 to pin 5 of the Matrix Keypad 
    	- Connect the NUCLEO pin PE_4 to pin 6 of the Matrix Keypad 
    	- Connect the NUCLEO pin PE_5 to pin 7 of the Matrix Keypad 
    	- Connect the NUCLEO pin PE_6 to pin 8 of the Matrix Keypad 
    - Connect the LCD to the NUCLEO
    	- Connect the NUCLEO pin PB_9 to the pin labeled "SDA" on the LCD
    	- Connect the NUCLEO pin PB_8 to the pin labeled "SCL" on the LCD
    	- Connect the GND breadboard bus to the pin labeled "GND" on the LCD
    	- Connect the VCC breadboard bus to the pin labeled "VCC" on the LCD
    - Connect the Distance Sensor to the NUCLEO
    	- Connect the GND breadboard bus to the pin labeled "GND" on the Distance Sensor
    	- Connect the NUCLEO pin PC_8 to the pin labeled "Echo" on the Distance Sensor
    	- Connect the NUCLEO pin PC_9 to the pin labeled "Trig" on the Distance Sensor
    	- Connect the VCC breadboard bus to the pin labeled "VCC" on the Distance Sensor
    - Connect the Buzzer to the NUCLEO
    	- Connect the GND breadboard bus to the pin labeled "GND" on the Buzzer
    	- Connect the NUCLEO pin PB_11 to the pin labeled "I/O" on the Buzzer
    	- Connect the NUCLEO pin PB_10 to the pin labeled "VCC" on the Buzzer
2. Connect the NUCLEO to the computer that has MBED Studio running via USB cable.
3. Clone the git repository locally.
4. Open the repository with Mbed Studio.
5. Select "Project 2" as the Active program in Mbed studio.
6. Connect the Nucleo L4R5ZI to your computer via USB cable.
7. Select Nucleo L4R5ZI as the Target in Mbed studio.
8. Click on the play button to load the program onto the NUCLEO and begin execution.



# File Structure
## Main Implementation
- CSE321_project3_mnelyubo_main.cpp
	-  This program operates a distance sensor, buzzer, LCD, and matrix keypad to notify workers if there are food items remaining in a container that can be taken home at closing time.


## Unit Tests
-  CSE321_project3_mnelyubo_buzzer_test.cpp
	-  This program tests the effect of various digital frequency and duty cycle inputs on the buzzer output peripheral.
-  CSE321_project3_mnelyubo_range_test.cpp
	-  This program tests the operation of the range detection sensor by repeatedly polling the sensor and printing the computed distance data.
-  CSE321_project3_mnelyubo_range_test.cpp
	-  This test code verifies the expected behavior of threads, event queues, and mutexes.  These scheduling utilities are used in the main project implementation.

