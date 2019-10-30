///////////////////////////////////////////////////////////////////////////////
// 1_PWM_Servomotor_Range_Scan
//
// PWM Servo Motor testing code using AdaFruit's Adafruit_PWMServoDriver.
//
///////////////////////////////////////////////////////////////////////////////
//
// The purpose of this code is to manually control PWM motors to find their
// motion limits.
//
// This code was developped for twinbluechimera and Navajoleo to test drive the
// Barbary lion costume animatronics motors, and detect manually the valid range
// of motion for each motor within the specs of the costume.
//
// The motors are controlled through the Arduino serial port interface.
// On the Arduino IDE, use menu Tools->Serial Monitor.
// A window will appear to send/receive data with this program:
// A menu will be displayed, to control up to 8 motors simultaneously. You can
// set parameters like their min/max range motor, on/off, and overal speed of
// motion. When motor(s) are turned on they will sweep their selected range,
// All motors can run simultaneously, while the menu is live at the same time.
// All are handled simultaneously.
//
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
// 1_PWM_Servomotor_Range_Scan 1.0 September 2019 - Initial release
#define VERSION "Range_Scan 1.0 September 2019 - Initial release"
///////////////////////////////////////////////////////////////////////////////

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Verbose printing to debug the code only
//#define DEBUG_PRINT(t)
#define DEBUG_PRINT(t) Serial.print(t)
#define DEBUG_PRINT_MAX(t)

// Instantiate the PWM motor driver at address 0x40
Adafruit_PWMServoDriver servoMotors = Adafruit_PWMServoDriver(0x40);


// Limits for each motor
#define TARGET_BOARD 2

#if (TARGET_BOARD == 1)
// Actual values for the Barbary costume head
constexpr uint8_t numServoMotors = 6; // Motors 3&4 are unused.
uint16_t servoMotorsMin[numServoMotors] =       {150, 220, 200, 485, 250, 485};
uint16_t servoMotorsMax[numServoMotors] =       {400, 280, 140, 240, 250, 240};
uint16_t servoMotorsPosition[numServoMotors] =  {250, 250, 250, 250, 250, 250};
uint8_t  servoMotorsWait[numServoMotors] =      {0, 0, 0, 0, 0, 0};
int8_t   servoMotorsDirection[numServoMotors] = {1, 1, 1, 1, 1, 1};
bool     servoMotorsOn[numServoMotors] =        {false, false, false, false, false, false};
uint8_t  servoMotorStepDelay = 5;
#pragma message "Target: BARBARY costume  "

#elif (TARGET_BOARD == 2)
constexpr uint8_t numServoMotors = 4;
uint16_t servoMotorsMin[numServoMotors] =       {190, 210, 210, 190};
uint16_t servoMotorsMax[numServoMotors] =       {500, 450, 450, 500};
uint16_t servoMotorsPosition[numServoMotors] =  {250, 250, 250, 250};
uint8_t  servoMotorsWait[numServoMotors] =      {0, 0, 0, 0,};
int8_t   servoMotorsDirection[numServoMotors] = {1, 1, 1, 1};
bool     servoMotorsOn[numServoMotors] =        {false, false, false, false};
uint8_t  servoMotorStepDelay = 5;
#pragma message "Target: barbary PROTOTYPE"

#else
// You can lower this value if there's less than 8 motors
constexpr uint8_t numServoMotors = 8;
//       Min/max positions of the motors (what we're trying to find with this program)
uint16_t servoMotorsMin[numServoMotors] =       {240, 240, 240, 240, 240, 240, 240, 240};
uint16_t servoMotorsMax[numServoMotors] =       {260, 260, 260, 260, 260, 260, 260, 260};
//       Current position of the motor (as is known by the program)
uint16_t servoMotorsPosition[numServoMotors] =  {250, 250, 250, 250, 250, 250, 250, 250};
//       Time left for a motor to wait before moving again at the end of a move
uint8_t  servoMotorsWait[numServoMotors] =      {0, 0, 0, 0, 0, 0, 0, 0};
//       Direction of movement, +1:min-> max, -1:max->min
int8_t   servoMotorsDirection[numServoMotors] = {1, 1, 1, 1, 1, 1, 1, 1};
//       Is the motor enabled to move
bool     servoMotorsOn[numServoMotors] =        {false, false, false, false, false, false, false, false};
// How long to wait between increments? 255 = no wait between min/max
uint8_t  servoMotorStepDelay = 5;

#endif

// The Feather supports up to 8 motors (0..7)
static_assert(numServoMotors <= 8);

// Delay between each step of the motor (bigger is slower, in msec)
// Motor delay at the end of a movement, before changing direction (msec)
constexpr uint16_t servoMotorEndDelay = 1000;


///////////////////////////////////////////////////////////////////////////////
// Initialization
///////////////////////////////////////////////////////////////////////////////
void setup()
{
  delay(1000);  

  // Initialize the servo motors at a 60Hz PWM frequency.
  servoMotors.begin();
  servoMotors.setPWMFreq(60);

  // Set up the Arduino Serial port to control the motors manually
  Serial.begin(9600);
  // Some boards may take a while to initialize their serial port
  while (!Serial){};
  Serial.println(VERSION);
  Serial.println(COPYRIGHT);
  delay(1000);  

  // Initialize the motor positions
  Serial.println("Initializing motors...");
  for (uint8_t m = 0; m < numServoMotors; m++)
  {
    Serial.println(m);
    servoMotors.setPWM(m, 0, servoMotorsMax[m]);
    delay(100);  
    servoMotors.setPWM(m, 0, servoMotorsPosition[m]);
    delay(1000);  
  }
  Serial.println("Ready!");

  showMenu();
}

///////////////////////////////////////////////////////////////////////////////
// Main loop
///////////////////////////////////////////////////////////////////////////////
void loop()
{

  menu();

  // Control all the the motors
  for (uint8_t m = 0; m < numServoMotors; m++)
  {
    // Don't update motors that are off
    if (servoMotorsOn[m] == false)
    {
      continue;
    }
    // If motor needs to wait at the end of a movement skip its turn
    if (servoMotorsWait[m] > 0)
    {
      --servoMotorsWait[m];
      if (servoMotorsWait[m] == 0)
      {
        DEBUG_PRINT(m);
        DEBUG_PRINT("WAIT ");
      }
      continue;
    }
    // Update the motor's position then move it to the new position
    // If speed is 255, then just move min to max, no steps, but also
    // motors are not moving at the same time.
    if (servoMotorStepDelay != 255)
    {
      servoMotorsPosition[m] += servoMotorsDirection[m];
    } else if (servoMotorsDirection[m] == 1)
    {
      servoMotorsPosition[m] = servoMotorsMax[m];
    } else
    {
      servoMotorsPosition[m] = servoMotorsMin[m];
    }
    
    // End of movement conditions
    if ((servoMotorsPosition[m] >= servoMotorsMax[m]) ||
        (servoMotorsPosition[m] <= servoMotorsMin[m]))
    {
      // Convert the wait (msec) into loop iterations
      servoMotorsWait[m] = servoMotorEndDelay / servoMotorStepDelay;

      // Enforce limits and change direction
      if (servoMotorsPosition[m] >= servoMotorsMax[m])
      {
        servoMotorsPosition[m] = servoMotorsMax[m];
        servoMotorsDirection[m] = -1;
        DEBUG_PRINT("M");
      }
      else if (servoMotorsPosition[m] <= servoMotorsMin[m])
      {
        servoMotorsPosition[m] = servoMotorsMin[m];
        servoMotorsDirection[m] = 1;
        DEBUG_PRINT("m");
      }
      DEBUG_PRINT(m);
      DEBUG_PRINT("=");
      DEBUG_PRINT(servoMotorsPosition[m]);
      DEBUG_PRINT("\n");
    }

    servoMotors.setPWM(m, 0, servoMotorsPosition[m]);
  } // for

  // Wait a millisecond at the end of the loop
  delay(servoMotorStepDelay);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// MENU HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Current menu choice
uint8_t menuChoice = 255;

// True means next we're parsing a main menu entry, False means the
// next value will be parsed by a sub-menu (for example input a value)
bool readMainMenu = true;

// Current Servo Motor the menu is acting on
uint8_t selectedServoMotor = 255;

///////////////////////////////////////////////////////////////////////////////
// Prints the menu
///////////////////////////////////////////////////////////////////////////////
constexpr uint8_t maxMenu = 5;
void showMenu()
{
  Serial.println("\n============= Main menu =============");
  Serial.println("0 - Display this menu");
  Serial.println("1 - Set current Servo Motor");
  Serial.println("2 - Set current Servo Motor Min value");
  Serial.println("3 - Set current Servo Motor Max value");
  Serial.println("4 - Turn on current Servo Motor");
  Serial.println("5 - Turn on all Servo Motors");
  Serial.println("6 - Center all Servo Motors");
  Serial.println("7 - Turn off all Servo Motors");
  Serial.println("8 - Set Servo Motors delay");
  Serial.println("9 - Show Servo Motors status");
  Serial.println("=====================================\n");
}

///////////////////////////////////////////////////////////////////////////////
// Handles all the menu actions
///////////////////////////////////////////////////////////////////////////////
void menu()
{
  // Read user input only when there's incoming data
  if (!Serial.available())
  {
    return;
  }

  // Read a number from the Serial console. Note that
  // if the number is greater than 64K, the value will
  // be garbage (there's no protection for that).
  uint16_t input = 0;
  do
  {
    // Get a byte from the console, it is ASCII.
    uint8_t character = Serial.read();
    //Serial.print(character);
    if (character >= '0' && character <= '9')
    {
      input = input * 10 + (character - '0');
      DEBUG_PRINT_MAX("[");
      DEBUG_PRINT_MAX(input);
      DEBUG_PRINT_MAX("]");
    }
    else
    {
      DEBUG_PRINT_MAX(".");
    }
    delay(1); // Needed to capture whole input line
  } while (Serial.available());
  DEBUG_PRINT_MAX("~ ");

  // Choosing an option from the main showMenu() menu
  if (readMainMenu)
  {
    menuChoice = input;
  }

  // Handle action for main menu option from showMenu()
  switch (menuChoice)
  {
  // Display main menu
  case 0:
    showMenu();
    break;

  // Set selectedServoMotor
  case 1:
    if (readMainMenu)
    {
      // Set-up for reading the value.
      Serial.print("Motor ID: ");
      readMainMenu = false;
    }
    else
    {
      // Read the value
      selectedServoMotor = input;
      if (selectedServoMotor >= numServoMotors)
      {
        // Retry if it's invalid
        Serial.print("Error: motor ");
        Serial.print(selectedServoMotor);
        Serial.print(" is not in use (0-");
        Serial.print(numServoMotors - 1);
        Serial.println(")!");
        Serial.print("Motor ID: ");
        readMainMenu = false;
      }
      else
      {
        // Go back to main menu if it's valid
        Serial.print(selectedServoMotor);
        Serial.println(" set!");
        readMainMenu = true;
      }
    }
    break;

  // Set servoMotorsMin[selectedServoMotor]
  case 2:
    if (selectedServoMotor >= numServoMotors)
    {
      // Disable option if it's not a valid motor to set
      readMainMenu = true;
    }
    else if (readMainMenu)
    {
      // prepare to read the value.
      Serial.print("Motor ");
      Serial.print(selectedServoMotor);
      Serial.print(" min (");
      Serial.print(servoMotorsMin[selectedServoMotor]);
      Serial.print("): ");
      readMainMenu = false;
    }
    else if (input > servoMotorsMax[selectedServoMotor])
    {
       Serial.println("NOT set! (value too high)");     
    }
    else
    {
      // read the value.
      servoMotorsMin[selectedServoMotor] = input;
      Serial.print(servoMotorsMin[selectedServoMotor]);
      Serial.println(" set!");
      readMainMenu = true;
    }
    break;

  // Set servoMotorsMax[selectedServoMotor]
  case 3:
    if (selectedServoMotor >= numServoMotors)
    {
      // Disable option if it's not a valid motor to set
      readMainMenu = true;
    }
    else if (readMainMenu)
    {
      // prepare to read the value.
      Serial.print("Motor ");
      Serial.print(selectedServoMotor);
      Serial.print(" max (");
      Serial.print(servoMotorsMax[selectedServoMotor]);
      Serial.print("): ");
      readMainMenu = false;
    }
    else if (input < servoMotorsMin[selectedServoMotor])
    {
      Serial.println("NOT set! (value too low)");
    }
    else
    {
      // read the value.
      servoMotorsMax[selectedServoMotor] = input;
      Serial.print(servoMotorsMax[selectedServoMotor]);
      Serial.println(" set!");
      readMainMenu = true;
    }
    break;

  // Set servoMotorsOn[selectedServoMotor]=true
  case 4:
    // Disable option if it's not a valid motor to set
    if (selectedServoMotor >= numServoMotors)
    {
      break;
    }
    Serial.print("Motor ");
    Serial.print(selectedServoMotor);
    if (servoMotorsOn[selectedServoMotor] == true)
    {
      Serial.println(" is already on!");
    }
    else
    {
      servoMotorsOn[selectedServoMotor] = true;
      Serial.println(" on!");
    }
    break;

  // Set servoMotorsOn[*]=true
  case 5:
    for (uint8_t m = 0; m < numServoMotors; m++)
    {
      Serial.print("Motor ");
      Serial.print(m);
      if (servoMotorsOn[m])
      {
        Serial.print(" is already");
      }
      Serial.println(" off!");
      servoMotorsOn[m] = true;
    }
    Serial.println("");
    showMenu();
    break;

  // Set servoMotorsOn[*]=true
  case 6:
    for (uint8_t m = 0; m < numServoMotors; m++)
    {
      Serial.print("Motor ");
      Serial.print(m);
      Serial.println(" centered!");
      uint16_t center = servoMotorsMin[m] +
                        (servoMotorsMax[m] - servoMotorsMin[m])/2;
      servoMotorsPosition[m] = center;
      servoMotors.setPWM(m, 0, center);
    }
    Serial.println("");
    showMenu();
    break;

  // Set servoMotorsOn[*]=false
  case 7:
    for (uint8_t m = 0; m < numServoMotors; m++)
    {
      Serial.print("Motor ");
      Serial.print(m);
      if (servoMotorsOn[m])
      {
        Serial.print(" is already");
      }
      Serial.println(" off!");
      servoMotorsOn[m] = false;
    }
    Serial.println("");
    showMenu();
    break;

  // Set servoMotorStepDelay
  case 8:
    if (readMainMenu)
    {
      // prepare to read the value.
      Serial.print("Delay (");
      Serial.print(servoMotorStepDelay);
      Serial.print("ms): ");
      readMainMenu = false;
    }
    else
    {
      // read the value.
      servoMotorStepDelay = input;
      Serial.print(servoMotorStepDelay);
      Serial.println("ms set!");
      readMainMenu = true;
    }
    break;

  case 9:
    for (uint8_t m = 0; m < numServoMotors; m++)
    {
      Serial.print("Motor");
      Serial.print((m == selectedServoMotor) ? ">" : " ");
      Serial.print(m);
      Serial.print(" [");
      Serial.print(servoMotorsMin[m]);
      Serial.print(",");
      Serial.print(servoMotorsMax[m]);
      Serial.print("] ");
      Serial.print(servoMotorsOn[m] ? "on  " : "off ");
      Serial.print("pos:");
      Serial.print(servoMotorsPosition[m]);
      Serial.print(" dir:");
      Serial.print(servoMotorsDirection[m] ? "fwd " : "rev ");
      Serial.print("wait:");
      Serial.println(servoMotorsWait[m]);
    }
    Serial.print("delay:");
    Serial.print(servoMotorStepDelay);
    Serial.print(" wait:");
    Serial.print(servoMotorEndDelay);
    Serial.print(" (");
    Serial.print(servoMotorEndDelay / servoMotorStepDelay);
    Serial.println("x)");
    break;

  // Ignore all other menu options
  default:
    Serial.print("Error: invalid menu entry ");
    Serial.print(menuChoice);
    Serial.println("!");
    break;
  } // switch
  DEBUG_PRINT_MAX(" -");
  DEBUG_PRINT_MAX(readMainMenu);
  DEBUG_PRINT_MAX("-");
  DEBUG_PRINT_MAX(menuChoice);
  DEBUG_PRINT_MAX("-> ");
  DEBUG_PRINT_MAX(input);
}
