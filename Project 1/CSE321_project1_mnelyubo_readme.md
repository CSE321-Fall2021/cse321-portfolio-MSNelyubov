-------------------
About
-------------------
Project Description: This project contains 
- CSE321_project1_mnelyubo_template.cpp will be used as the basis for all C++ files generated for CSE321 by Misha Nelyubov
- CSE321_project1_mnelyubo_corrected_code.cpp is a program written in C++ to oscillate the blinking of a light on a Nucleo L4R5ZI
- push-ToGitRepo.ps1 is a PowerShell script to push files from the working Mbed Studio directory into the Git version control directory


Contribitor List:
- Misha Nelyubov (mnelyubo@buffalo.edu)
- Dr. Jennifer Winikus (jwinikus@buffalo.edu)

-------------------
Features
-------------------


-------------------
Required Materials
-------------------
- Nucleo L4R5ZI
- Mbed Studio (https://os.mbed.com/studio/)


-------------------
Resources and References
-------------------


-------------------
Getting Started
-------------------
1. Clone the git repository locally
2. Open the repository with Mbed Studio
3. Select "Project 1" as the Active program in Mbed studio
4. Connect the Nucleo-L4R5ZI to your computer via USB cable
5. Select Nucleo-L4R5ZI as the Target in Mbed studio
6. Click on the play button labeled "Run Program" to begin executing the code on the Nucleo



-------------------
CSE321_project1_mnelyubo_corrected_code.cpp:
-------------------
 
This file has lots of things. There is a os typle tool used to create periodic events with a periepherial. The name of the file and the contents in here will be entirely updated.
 
This is totally not bare metal since there are some cool tools used. Those tools instantiate with a finite reference but gots their unique object created. 


----------
Global Declarations
----------
- Functions
    - cycleLedState
    - button1PushDownBehavior
    - button1OpenBehavior

- Variables
    - buttonPressed
        - Active high indicator for the state of the userButton1 input
    - oscillateLED_L
        - Active low indicator for whether or not a new cycle of turning the LED on and off should be undertaken

- API objects (described further in "API and Built-In Elements Used")
    - controller  (Thread)
    - outputLED   (DigitalOut)
    - userButton1 (InterruptIn)
----------
API and Built-In Elements Used
----------
- mbed.h
    - Thread controller
        - used to handle concurrent scheduled events with the operating system
        - allows cycling of the LED states and interrupting for input changes
    - DigitalOut outputLED
        - used to output a high or low signal to the onboard blue LED
    - InterruptIn userButton1
        - used to read input from the onboard User button 1

----------
Functions
----------

- cycleLedState:
    - This function runs continously once called.  Whenever the global variable oscillateLED_L is low, this function will begin a cycle in which it turns on an LED for 2000 ms and then turns it off for 500 ms before repeating this behavior.
    - Inputs:
        - None
    - Outputs:
        - the state of LED2 is toggled during this function 
    - Global variables accessed:
        - outputLED, oscillateLED_L
    - Global variables modified:
        - outputLED


- button1PushDownBehavior:
    - This function enables the next call to button1OpenBehavior to switch the state of the LED behavior between oscillating and resting.
    - Inputs:
        - None
    - Outputs:
        -  None
    - Global variables accessed:
        - buttonPressed
    - Global variables modified:
        - buttonPressed


- button1OpenBehavior:
    - This function is triggered at the end of an button click. 
    - This function toggles the state of whether or not the system output LED will continue to go through blinking cycles.
        - Blinking will not necessarily be stopped immediately: the current cycle will complete before a pause will take effect.
    - Inputs:
        - None
    - Outputs:
        -  None
    - Global variables accessed:
        - buttonPressed, oscillateLED_L
    - Global variables modified:
        - buttonPressed, oscillateLED_L
