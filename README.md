## cse321-portfolio-MSNelyubov

## About
This repository contains files for the Projects developed by Misha Nelyubov during the Fall 2021 instance of CSE321: Real-Time Operating Systems at UB.

## Project 1
This project focused on standards for documentation.  The project contains the following files:

- CSE321_project1_mnelyubo_template.cpp will be used as the basis for all C++ files written for CSE321 by Misha Nelyubov
- CSE321_project1_mnelyubo_corrected_code.cpp is a program written in C++ to oscillate the blinking of a light on a Nucleo L4R5ZI
- push-ToGitRepo.ps1 is a PowerShell script to push files from the working Mbed Studio directory into the Git version control directory

## Project 2
This project tracks the design and development of a countdown timer which can be programmed by its user through a peripheral keypad and displays output text with an LCD display.  The project contains the following files:

- CSE321_project2_mnelyubo_main.cpp is the C++ file which implements the core functionality of the countdown timer
- CSE321_project2_stage2_part1_mnelyubo.pdf provides a high-level overview of the design of the microcontroller behavior
- 1802.cpp and 1802.h are the library files for interacting with the output LCD

## Project 3
This project tracks the design and development of a used-volume monitor for a container to notify when there is still food present at the end of a work day.  The objective of the project is to minimize food waste by alerting staff of leftover food that can be taken home before leaving work.  The project contains the following files:

- CSE321_project3_mnelyubo_main.cpp operates a distance sensor, buzzer, LCD, and matrix keypad to notify workers if there are food items remaining in a container that can be taken home at closing time.
The following hardware test programs are included in the project subfolder "tests".
-  tests/CSE321_project3_mnelyubo_buzzer_test.cpp tests the effect of various digital frequency and duty cycle inputs on the buzzer output peripheral.
-  tests/CSE321_project3_mnelyubo_range_test.cpp tests the operation of the range detection sensor by repeatedly polling the sensor and printing the computed distance data.
-  tests/CSE321_project3_mnelyubo_range_test.cpp tests the expected behavior of threads, event queues, and mutexes.  These scheduling utilities are used in the main project implementation.

