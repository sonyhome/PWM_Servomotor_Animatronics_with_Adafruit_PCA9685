# PWM_Servomotor_Animatronics_with_Adafruit_PCA9685
 Arduino sketches to tune and control PWM motors using Adafruits PCA9685, featherette or other I2C servo drivers

---

## Overview
Arduino program to manually set the range of motion of **PWM Servo Motors** using Serial port console inputs, for use with an Adafruit PWM servo shield. 

This program can be useful for people who need to manually carefully adjust the range of motion of their motors for their project. It allows manually adjusting the limits of motion as the motor(s) move, so the range can be visually inspected for precise motion range.

---

## History
This program was written to test the range of animation for the Barbari lion animatronic motors, controlled by an [AdaFruit nRF52 Bluefruit LE board](https://www.adafruit.com/product/3406), but should work with any Arduino style board. By default this program can control simulataneously up to 8 motors. The menu allows to toggle motors on and off, set their motion range limit and the speed of the movement.
All motors can be on and moving simultaneously, while receiving commands from the Serial console. When a motor is active, it updates the Serial console when it reaches the end of its range of motion.

This program is also a good illustration of storing states to "do multiple things at the same time", which is something first time Arduino programmers struggle to do.

## Version
1.0
* September 2019 - 1_PWM_Servomotor_Range_Scan, Initial version
* October 2019 - 2_PWM_Servomotor_Positions, Initial version

## Requirements
The program runs on Arduino with the Adafruit I2C PWM Servo shields ([PCA9685](https://www.adafruit.com/product/815), [Uno](https://www.adafruit.com/product/1411) or [FeatherWing](https://www.adafruit.com/product/2928)) and requires the following:
* Arduino compatible board (based on AVR, ARM, nRF... micro-controllers)
* Serial console library (note AtTiny often don't support it)
* [Adafruit_PWMServoDriver](https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library)

It is expected you know how to install the necessary library in your Arduino IDE (see the library's how-to), as well as installing the board or controller you're using in your Arduino IDE, and know how to compile and load a program on your board/controller to be run, using the proper ISP.
* copy the ino file into a directory with the same name as the file in your Arduino program directory (possibly *Documents/Arduino/Arduino_PWM_Servo_Motor_Range_Test*)

## Licencing
This program is available for use under the **MIT licence** which is the most common on GitHub, and has very few restrictions, so enjoy!

## General usage - Serial console

This program relies on the Serial console for user inpu to control the motors.

1- Compile and load the sketch onto your Arduino compatible board
2- Open the console in the Arduino IDE with the menu "Tools" "Serial Monitor"
<img src=SerialMonitorMenu.png>
3- The Serial console window will appear. These programs will print a menu in it and allow you to input commands.
<img src=SerialMonitor.png>
4- In the top input text box, type the menu command(s), then press the "send" button. The Serial console will update accordingly, and your Arduino board will perform the requested actions.

If no action is performed, no menu is displayed by the program, or no console window appears, you will need to debug what you did with the hardware.

# 1_PWM_Servomotor_Range_Scan sketch

The purpose of this program is to help you find the movement range allowed for each motor by entering and tweaking the min and max values allowed for each PWM servo motor.

This program relies on the Serial console for user input.

## Serial console menu

The menu allows you to control up to 8 motors simultaneously.

Menus 1,2,3 will prompt you for a value.
Sending enter or 0 will re-display the menu.
Menu 1 sets the current motor to act on (0 to 7) when using menus 2,3,4. Menu 7 will point which motor is selected.
Menus 5,6,7 act on all motors.

    ============= Main menu =============
    0 - Display this menu
    1 - Select Servo Motor
    2 - Set current Servo Motor Min value
    3 - Set current Servo Motor Max value
    4 - Turn on current Servo Motor
    5 - Turn off all Servo Motors
    6 - Set Servo Motors delay
    7 - Show Servo Motors status
    =====================================

## Using the serial console menu

At first only the menu is shown, and nothing else happens until you turn on the motor(s).

Note that all the motors what are turned on will simulataneously move continuously between their min and manx position.

To tune your PWM motors, I recommend that at first you work with only one motor at a time.
To start, set min and max values of the servo close to the middle of their range.
Turn on that motor, and then slowly adjust the range of the motor until you find the best min and max values for your project.

If the servo moves too fast, increase the delay between movements to slow it down.

Work on other motors in a similar fashion.

Once all your min/max values are set, you can turn on all motors at once. The program will move all the motors at the same time continuously. You can use this mode to fine tune the limits of the motors, in case they interact with each other.

Once you're satisfied, you can print the current motor settings on the console, to save them for later use. That's the purpose of this sketch.


## Tweaking

- You may have to change addresses in the code for the servos (default 0x40).
- The initial motor position and ranges are set at the middle of the range, moving very little, and the delay pretty high so the motors move slow. You can change the hard-coded values to the values you recorded previously, especially if you're planning to re-load the sketch to fine tune your project later.

# Last words
If you use these programs please leave me a comment, a star, requests, or even an issue on GitHub so that I know it is of use. I would appreciate also that if you make a project you mention it used or was inspired by my work, and link back to this GitHub.

If you use GitHub, it is easy to "fork" this code, and enhance it. You can then request a merge back if your additions do not change the original intent of the sketches, but enhance or augment them, so that others can use them.

# 2_PWM_Servomotor_Positions

The purpose of this sketch is to play with the different motor positions to help define "attitudes" of the animatronics costume head.

The motors control 2 ears (2 motors per ear to down/up and move front/back). The goal is to test every resting position and define the set of valid positions for the ears, and name them based on what they represent (sad, angry, happy, attentive, etc.)

The ear positions are defined in this spreadsheet, using motor positions that were defined using 1_PWM_Servomotor_Range_Scan:
https://docs.google.com/spreadsheets/d/1kdY3KxZtphHCWXRy3MNVyo5G2VPGZbcAcurjgDs7uXk/edit?usp=sharing

## Serial console menu

The menu allows you to control the ears to move them to specific positions. For example typing "LDBRDB[send]" would lower both ears in what would look like an angry position. Note that once a position is reached, the motors stop, until more commands are sent.

   Positions 1.0 Oct 2019
   Copyright (c) 2019 Dan Truong
   0: 400 275 150
   1: 220 250 280
   2: 200 170 140
   3: 210 245 280
   
   Menu:
   M - Menu
   L - Left ear
   R - Right ear
   D - Down
   N - Neutral
   U - Up
   B - Back
   C - Center
   F - Front
   T - Twitch
   W - wait 1s

At boot, the console shows the valid min/center/max positions available for each motor.
The motors are paired-up, a pair can do up-neural-down and front-center-back.

It is possible to insert waits in 1 second increments. A wait will start after the last motor ended its movement.
If you enter multiple movements without waits, the motors will move at the same time.
Unlike the first program, motors don't move until instructed to, and will stop once they reach the chosen position.

For example, "LDBRDBWWLUFRUF" will move both ears down and back at the same time, wait 1s and then bring the ears up facing the front.
"LDBWRDB" will move the left ear down and back, wait 1s and then move the right ear down and back.

## Heartbeat

Sometimes you can't tell if a program crashed, or if the console stopped responding.
When there is no user input, this sketch will print a dot on the console every 10s. Furthermore, after 10s, the board's LED will start blinking.

When the user gives commands, the dots and blinking stop. Instead the LED will turn on when a command is sent to a servo. That's very brief so the LED will look like it has a dim glow.

## Debugging

* If your motors have different limits, change their ranges in the sketch in SERVO_MOTOR_BOTTOM and SERVO_MOTOR_TOP.
* If the motors are too slow or too fast, change LOOP_DELAY.
* If you want more or less debug output on the console you can set DEBUG_VERBOSE_ON, DEBUG_STATE_ON, DEBUG_MENU_ON and DEBUG_ON to true or false.