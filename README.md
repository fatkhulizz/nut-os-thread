# NUT/OS multithread

## Description

Controling ATMega128 using Modular RTOS (Nut/OS) for multithread.

This program is used to control ATMega128 using Nut/OS for multithreading support.
* thread 1 - control led 1 for blinking every 0.5 second
* thread 2 - control led 2 for blinking every 1.0 second
* thread 3 - control led 2 and led 3 for blinking every 2.0 second alternately
* thread 4 - control motor stepper, control led, display SHT11 sensor data using UART

## Flowchart
![Flowchart](https://github.com/fatkhulizz/nut-os-thread/raw/main/embed.png)

## Getting Started

### Dependencies

#### Hardware

* Board Ethernut with ATMega128
* Programmer STK500
* LED
* Motor stepper
* Sensor SHT11
* USB cable to RS232

#### Hardware Configuration

ATMega128 Configuration
* `LED` pin B4 - B7
* `Motor Steppper` pin D0 - D3
* `Sensor SHT11` pin E3
* `UART` pin UART0

#### Software

* RTOS (Nut/OS) 
* WinAVR
* remLab - *optional*

### Installing

* Install all the dependencies
* Clone this repo 
* Move the folder `nutapp` folder
* Open `cmd` and move the directories to that folder
* Run `make clean` & `make all` command on `cmd` to compile the program

### Executing program

After compile the program, upload the hex file into Nut/OS 

If uploading process is successful, LED1 - LED4 will blink as ordered, and serial monitor will display three options 
* options 1 - control motor stepper. motor stepper can be controlled based on 3 parameters

        input format = [<step><space><direction><space><speed>]
    * `step` number of steps (n×4) for 180 degree rotation = 200 step
    * `direction` direction of rotation. 1 for rotate right, and 0 for rotate left
    * `speed` rotation speed. smaller the value, the faster
    

    
* options 2 - display temperature and humidity from SHT11 sensor
* options 3 - control led. led can be controlled based on 3 parameters

        input format = [<ledpin><space><duration><space><blink>]
    * `ledpin` chose which led will be controlled 
    * `duration` duration of blink
    * `blink` how many led will blink 
    



# Disclaimer
This is just a toy project Something for me to mess around with so I can learn more about
microcontroller, embedded system and also writing better documentation.
