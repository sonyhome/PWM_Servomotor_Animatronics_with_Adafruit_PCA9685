///////////////////////////////////////////////////////////////////////////////
// 2_PWM_Servomotor_Positions
//
// PWM Servo Motor testing code using AdaFruit's Adafruit_PWMServoDriver.
//
///////////////////////////////////////////////////////////////////////////////
//
// The purpose of this code is to move motors to specific coordinates.
//
// This code was developped for twinbluechimera and Navajoleo to move the
// Barbary lion costume animatronics motors to predefined positions, and find
// positions that represent valid expressions to make realistic animations.
//
// The motors are controlled through the Arduino serial port interface.
// On the Arduino IDE, use menu Tools->Serial Monitor.
// A window will appear to send/receive data with this program:
// A menu will be displayed, to control the motors simultaneously. You can send
// a sequence of multiple 1 letter commands to define where the motors should
// move to. They will all act more or less simultaneously (within a few msec).
// For example sending LDBRDBWWLUFRFU would make both ears move down and back,
// wait for 2 seconds and then move them up and front.
// When no command is sent, the serial port prints a dot as a hearbeat to show
// the console and the board are still working fine, and the onboard LED starts
// blinking.
// When the motors are moving, the LED turns on. It's very fast so it looks dim
///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019 Dan Truong
#define COPYRIGHT "Copyright (c) 2019 Dan Truong"
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
///////////////////////////////////////////////////////////////////////////////
// Versions
// 2_PWM_Servomotor_Positions 1.0 Oct 14, 2019 - Initial release
#define VERSION "Positions 1.0 Oct 2019"
///////////////////////////////////////////////////////////////////////////////

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////

// Messages used to debug the code
#define DEBUG_PRINT(t) if (DEBUG_ON){ Serial.print(t); }
#define DEBUG_VERBOSE(t) if (DEBUG_VERBOSE_ON == true){ DEBUG_PRINT(t); }

// Change the values below to enable tracing of specific elements:
// Debugging creates strings, and reduces dynamic memory available.
// You may run out of dynamic memory on a borad like a UNO!
#define DEBUG_ON true
#define DEBUG_VERBOSE_ON false // Shows all motor steps
#define DEBUG_STATE_ON true    // Shows state machine transitions
#define DEBUG_MENU_ON true     // Shows menu inputs

// Instantiate the PWM motor driver at address 0x40
Adafruit_PWMServoDriver servoMotors = Adafruit_PWMServoDriver(0x40);

// You can lower this value if there's less than 8 motors. We use 4
constexpr uint8_t NUM_SERVO_MOTORS = 6;

// Movement boundaries for each servo motor. This program only defines
// 3 targets for a motor: bottom, center and top of its range of motion
constexpr uint16_t SERVO_MOTOR_BOTTOM[NUM_SERVO_MOTORS] = {400, 220, 200, 210, 200, 200};
constexpr uint16_t SERVO_MOTOR_TOP[NUM_SERVO_MOTORS]    = {150, 280, 140, 280, 140, 140};

// Compute 1/2 way point of servo motor
#define MIN(motorId) ((SERVO_MOTOR_BOTTOM[motorId] > SERVO_MOTOR_TOP[motorId]) ? \
  SERVO_MOTOR_TOP[motorId] : SERVO_MOTOR_BOTTOM[motorId])
#define MAX(motorId) ((SERVO_MOTOR_BOTTOM[motorId] > SERVO_MOTOR_TOP[motorId]) ? \
  SERVO_MOTOR_BOTTOM[motorId] : SERVO_MOTOR_TOP[motorId])
#define SERVOMOTOR_CENTER_VALUE(motorId) (MIN(motorId) + \
  (MAX(motorId) - MIN(motorId)) /2)

// Neutral position for each servo motor, set to 1/2 way point but may be overriden
constexpr uint16_t SERVO_MOTOR_CENTER[NUM_SERVO_MOTORS] ={
  SERVOMOTOR_CENTER_VALUE(0),
  SERVOMOTOR_CENTER_VALUE(1),
  SERVOMOTOR_CENTER_VALUE(2),
  SERVOMOTOR_CENTER_VALUE(3),
  SERVOMOTOR_CENTER_VALUE(4),
  SERVOMOTOR_CENTER_VALUE(5) // Note: #2 assumed broken so duplicated to #4
};

// Time in msec between each iteration
// 2 = about 1/2 sec to perform the biggest move
// 10 = makes the movement slow enough to see the steps on the console
constexpr uint8_t  LOOP_DELAY = 10;
constexpr uint16_t ONE_SECOND = 1000/LOOP_DELAY;

// Known state machine states:
// All the target states a servo motor can be in
// A state relates to target positions or movements
enum servoMotorsStates {
  toBottom, // Moving toward down or back position
  toCenter, // Moving towards neutral or middle position
  toTop,    // Moving towards front or up position
  stopped,  // Stable position reached, no movement needed
  flick    // Perform an ear flick at current position
};

///////////////////////////////////////////////////////////////////////////////
// Global Variables
///////////////////////////////////////////////////////////////////////////////

// Current motor position as this program knows it.
uint16_t servoMotorsPosition[NUM_SERVO_MOTORS] = {
  SERVO_MOTOR_CENTER[0],
  SERVO_MOTOR_CENTER[1],
  SERVO_MOTOR_CENTER[2],
  SERVO_MOTOR_CENTER[3],
  SERVO_MOTOR_CENTER[4]
};

// Current servo motor state
// Motors stop moving when the target position is reached.
servoMotorsStates servoMotorState[NUM_SERVO_MOTORS] = {
  stopped, stopped, stopped, stopped, stopped
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// ONBOARD LED HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool isLedOn = false;

inline void __attribute__((always_inline)) ledSetup()
{
  pinMode(LED_BUILTIN, OUTPUT);
}

inline void __attribute__((always_inline)) ledOn()
{
  digitalWrite(LED_BUILTIN, HIGH);
}

inline void __attribute__((always_inline)) ledOff()
{
  digitalWrite(LED_BUILTIN, LOW);
}

inline void __attribute__((always_inline)) ledBlink()
{
  if (isLedOn) {
    ledOff();
  } else {
    ledOn();
  }
  isLedOn = !isLedOn;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// ONBOARD PWM MOTOR HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline __attribute__((always_inline)) void moveServoMotor(uint8_t motorId, uint16_t pos)
{
  ledOn();
  servoMotors.setPWM(motorId, 0, pos);
  ledOff();
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SERVO MOTOR MOVEMENT HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Serial port debug message formatting for the state machine
#define DEBUG_STATE_MACHINE(motorId, command, to) if (DEBUG_STATE_ON) { \
      DEBUG_PRINT(" SM - ");                                            \
      DEBUG_PRINT(motorId);                                             \
      DEBUG_PRINT(": " #command " -> " #to "\n"); }
#define DEBUG_STATE_MACHINE_PRINT(t) if (DEBUG_STATE_ON) { DEBUG_PRINT(t); }

///////////////////////////////////////////////////////////////////////////////
// Perform incremental moves at every loop iteration from the current position
// All motors perform their incremental moves one after the other, so it looks
// like they all run at the same time.
// @return true if there was a movement
///////////////////////////////////////////////////////////////////////////////
template <const uint16_t targetPosTable[NUM_SERVO_MOTORS]>
boolean singleStepMove(const uint8_t motorId)
{
  DEBUG_VERBOSE(motorId);
  // Move towards the target position
  if (servoMotorsPosition[motorId] > targetPosTable[motorId]) {
    DEBUG_VERBOSE('-');
    --servoMotorsPosition[motorId];
    moveServoMotor(motorId, servoMotorsPosition[motorId]);
    return true;
  } else if (servoMotorsPosition[motorId] < targetPosTable[motorId]) {
    DEBUG_VERBOSE('+');
    ++servoMotorsPosition[motorId];
    moveServoMotor(motorId, servoMotorsPosition[motorId]);
    return true;
  }
  // Done moving
  DEBUG_VERBOSE('~');
  servoMotorState[motorId] = stopped;
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// Perform an ear flick move from the current position
// This is a temporary move. At the end the ear goes to the original position.
// Since I'm lazy the flick is done sequentially. This means no other motor
// can move while this motor does a flick.
///////////////////////////////////////////////////////////////////////////////
inline void flickMove(const uint8_t motorId)
{
  // A flick is always a motion towards the bottom value
  if (servoMotorsPosition[motorId] == SERVO_MOTOR_BOTTOM[motorId]) {
    return;
  }
  DEBUG_VERBOSE(motorId);
  moveServoMotor(motorId, SERVO_MOTOR_BOTTOM[motorId]);
  DEBUG_VERBOSE('\\');
  delay(LOOP_DELAY);
  moveServoMotor(motorId, servoMotorsPosition[motorId]);
  DEBUG_VERBOSE('/');
  delay(LOOP_DELAY);
}

///////////////////////////////////////////////////////////////////////////////
// Moves a motor specified to its target position from the current position.
// This obeys the commands in servoMotorState[] and may change the state.
//
// Implements a simple state machine.
///////////////////////////////////////////////////////////////////////////////
void runServoMotorStateMachine(uint8_t motorId)
{
  switch (servoMotorState[motorId])
  {
    case toBottom:
      if (!singleStepMove<SERVO_MOTOR_BOTTOM>(motorId))
      {
        DEBUG_STATE_MACHINE(motorId, toBottom, stopped);
        servoMotorState[motorId] = stopped;
      }
      return;
    case toCenter:
      if(!singleStepMove<SERVO_MOTOR_CENTER>(motorId))
      {
        DEBUG_STATE_MACHINE(motorId, toCenter, stopped);
        servoMotorState[motorId] = stopped;
      }
      return;
    case toTop:
      if(!singleStepMove<SERVO_MOTOR_TOP>(motorId))
      {
        DEBUG_STATE_MACHINE(motorId, toTop, stopped);
        servoMotorState[motorId] = stopped;
      }
      return;
    case stopped:
      //DEBUG_STATE_MACHINE(motorId, stopped, stopped);
      return;
    case flick:
      flickMove(motorId);
      DEBUG_STATE_MACHINE(motorId, flick, previous);
      servoMotorState[motorId] = stopped;
      return;
    default:
      DEBUG_STATE_MACHINE_PRINT("ERROR - SM\n");
    break;
  }
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// MENU HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Serial port debug message formatting for the menu
#define DEBUG_MENU(isLeft, motorId, command, to) if (DEBUG_MENU_ON) {        \
    if(isLeft) {                                                             \
      DEBUG_PRINT(" MN " #motorId ": " #command " -> " #to " - left ear\n");\
    } else {                                                                 \
      DEBUG_PRINT(" MN " #motorId ": " #command " -> " #to "- right ear\n");\
    }                                                                        \
  }
#define DEBUG_MENU_PRINT(t) if (DEBUG_MENU_ON) { DEBUG_PRINT(t); }

///////////////////////////////////////////////////////////////////////////////
// Prints the menu
///////////////////////////////////////////////////////////////////////////////
static void showMenu()
{
  Serial.println("\nMenu:");
  Serial.println("L - Left ear");
  Serial.println("R - Right ear\n");
  Serial.println("D - Down");
  Serial.println("N - Neutral");
  Serial.println("U - Up\n");
  Serial.println("B - Back");
  Serial.println("C - Center");
  Serial.println("F - Front");
  Serial.println("T - Twitch\n"); // flick ear
  Serial.println("W - wait 1s");
  Serial.println("M - Menu\n");
}

constexpr uint16_t WAIT_TICK = 10*ONE_SECOND;
constexpr uint16_t WAIT_BLINK = ONE_SECOND/2;

///////////////////////////////////////////////////////////////////////////////
// Wait for the Serial console input from the user
// When no action is detected, after a while it shows heartbeats
// Returns true if the menu should not try to read the Serial console
// Returns false if there's commands to read from the Serial console
///////////////////////////////////////////////////////////////////////////////
static bool waitForSerialInput()
{
  // Count how long we've been waiting for the user to input data
  static uint16_t waiting = 0;
  
  // Read user input only when there's incoming data
  if (!Serial.available()) {
    ++waiting;
    // Blink if we've been waiting 10s
    if (waiting >= WAIT_TICK && waiting % WAIT_BLINK == 0) {
      ledBlink();
    }
    if (waiting % WAIT_TICK == 0) {
      // Print heartbeat every 10s
      Serial.print(".");
    }
    return true;
  }
  // New line if we printed ticks.
  if (waiting >= WAIT_TICK)
  {
    Serial.println("");
    waiting = 0;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// If the user asked for a wait to be performed, this function will execute it
// Returns true if the menu must wait more.
// Returns false if it's OK to read more commands.
///////////////////////////////////////////////////////////////////////////////
constexpr uint16_t WAIT_TIME = ONE_SECOND;
uint16_t menuWait = 0;

bool handleMenuWaitCommand()
{
  if (menuWait == 0) {
    return false;
  }

  // Before waiting for 1 second, we first wait for all movements to stop
  // This is OK as long as all states always transition to "stopped"
  for(uint8_t motorId = 0; motorId < NUM_SERVO_MOTORS; ++motorId) {
    // It's a bit dirty to access the motor states from here but it works
    if (servoMotorState[motorId] != stopped) {
      return true;
    }
  }

  // Wait for predefined number of cycles
  --menuWait;
  if (menuWait == 0) {
     DEBUG_VERBOSE("w\n");     
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Handles all the menu actions
// This sets the states for the PWM motors in the global arrays.
///////////////////////////////////////////////////////////////////////////////
static void runMenu()
{
  // Select which ear is controled: left=true, right=false.
  static boolean isLeftEar = true;
  
  // Handle command to wait before loading more commands.
  if (handleMenuWaitCommand()) {
    return;
  }

  // Read user input only when there's incoming data
  if (waitForSerialInput()) {
    return;
  }
  
  // Read a character from the Serial console, convert to uppercase.
  char menuChoice = Serial.read() | 0x20;
  //DEBUG_PRINT(menuChoice);
  //DEBUG_PRINT("<-\n");

  // Handle action for main menu option from showMenu()
  switch (menuChoice)
  {
  case 'm':
    showMenu();
    break;
  case 'l':
    DEBUG_MENU_PRINT("L\n");
    isLeftEar = true;
    break;
  case 'r':
    DEBUG_MENU_PRINT("R\n");
    isLeftEar = false;
    break;
    
  case 'd':
    if (isLeftEar) {
      DEBUG_MENU(isLeftEar, 5, down, toBottom);
      //servoMotorState[2] = toBottom;
      servoMotorState[4] = toBottom;
      servoMotorState[5] = toBottom;
    } else {
      DEBUG_MENU(isLeftEar, 1, down, toBottom);
      servoMotorState[1] = toBottom;
    }
    break;
  case 'n':
    if (isLeftEar) {
      DEBUG_MENU(isLeftEar, 5, neutral, toCenter);
      //servoMotorState[2] = toCenter;
      servoMotorState[5] = toCenter;
      servoMotorState[4] = toCenter;
    } else {
      DEBUG_MENU(isLeftEar, 1, neutral, toCenter);
      servoMotorState[1] = toCenter;
    }
    break;
  case 'u':
    if (isLeftEar) {
      DEBUG_MENU(isLeftEar, 5, up, toTop);
      //servoMotorState[2] = toTop;
      servoMotorState[4] = toTop;
      servoMotorState[5] = toTop;
    } else {
      DEBUG_MENU(isLeftEar, 1, up, toTop);
      servoMotorState[1] = toTop;
    }
    break;
    
  case 'b':
    if (isLeftEar) {
      DEBUG_MENU(isLeftEar, 3, back, toBottom);
      servoMotorState[3] = toBottom;
    } else {
      DEBUG_MENU(isLeftEar, 0, back, toBottom);
      servoMotorState[0] = toBottom;
    }
    break;
  case 'c':
    if (isLeftEar) {
      DEBUG_MENU(isLeftEar, 3, center, toCenter);
      servoMotorState[3] = toCenter;
    } else {
      DEBUG_MENU(isLeftEar, 0, center, toCenter);
      servoMotorState[0] = toCenter;
    }
    break;
  case 'f':
    if (isLeftEar) {
      DEBUG_MENU(isLeftEar, 3, front, toTop);
      servoMotorState[3] = toTop;
    } else {
      DEBUG_MENU(isLeftEar, 0, front, toTop);
      servoMotorState[0] = toTop;
    }
    break;
  // Ignore invalid menu choices
  case 't':
    if (isLeftEar) {
      DEBUG_MENU(isLeftEar, 3, twitch, flick);
      servoMotorState[3] = flick;
    } else {
      DEBUG_MENU(isLeftEar, 0, twitch, flick);
      servoMotorState[0] = flick;
    }
    break;
  // Ignore invalid menu choices
  
  case 'w':
    DEBUG_MENU_PRINT("W\n");
    menuWait = WAIT_TIME;
    break;
  default:
    // We want to ignore carriage returns and other garbage
    if (menuChoice <= 45) {
      break;
    }
    DEBUG_MENU_PRINT(menuChoice);
    // Empty all pending choices after a bad command
    while (Serial.available())
    {
      char c;
      c = Serial.read();
      DEBUG_MENU_PRINT(c);
      delay(1);
    }
    DEBUG_MENU_PRINT("\n");
    Serial.print("ERROR [");
    Serial.print(menuChoice);
    Serial.println("]!");
    break;
  } // switch
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Initialization
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void setup()
{
  ledSetup();

  // Initialize the servo motors at a 60Hz PWM frequency.
  servoMotors.begin();
  servoMotors.setPWMFreq(60);

  // Set up the Arduino Serial port to control the motors manually
  // Some boards may take a while to initialize their serial port
  Serial.begin(9600);
  while (!Serial){};
  Serial.println(VERSION);
  Serial.println(COPYRIGHT);
  
  // If you don't see these messages the Serial port failed to initialize
  for (uint8_t motorId =0; motorId < NUM_SERVO_MOTORS; motorId++)
  {
    Serial.print(motorId);
    Serial.print(": ");
    Serial.print(SERVO_MOTOR_BOTTOM[motorId]);
    Serial.print(" ");
    Serial.print(SERVO_MOTOR_CENTER[motorId]);
    Serial.print(" ");
    Serial.println(SERVO_MOTOR_TOP[motorId]);
    // Move to demo full range, then go to expected position
    //moveServoMotor(motorId, SERVO_MOTOR_BOTTOM[motorId]);
    //delay(500);
    //moveServoMotor(motorId, SERVO_MOTOR_TOP[motorId]);
    //delay(500);
    moveServoMotor(motorId, servoMotorsPosition[motorId]);
    delay(50);
  }

  showMenu();
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Main loop
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void loop()
{
  runMenu();
  // Control all the the motors
  for (uint8_t motorId = 0; motorId < NUM_SERVO_MOTORS; ++motorId)
  {
    if (motorId == 2) continue; // don't send commands to #2 (debugging)
    runServoMotorStateMachine(motorId);
  }
  // Wait a millisecond at the end of the loop
  delay(LOOP_DELAY);
}
