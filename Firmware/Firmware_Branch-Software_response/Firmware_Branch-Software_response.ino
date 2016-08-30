/*          
 *              -Zach Lasiuk
 *                
 *                
 *                What does it do:
 *                
 *                How to run code:
 *                    -Need Arduino Mega 2560 with servo motor shield attached
 *                    -Plug in servos (should be FiTec FS5106B) to pins 0-3 of Arduino shield
 *                    -Upload script
 *                    -Open Serial Monitor 
 * 
 *                Library functions:     
 *                    setPWMFreq(freq)
 *                      -Adjusts PWM frequency which determines how many full 'pulses' per second are generated by the IC.
 *                       -Must be between 40-1000 (in Hz)
 *                       -Check servo to see if number violates speed at which servo can read pulses
 *        
 *                    setPWM(channel, on, off)
 *                        channel:    The channel that should be updated with the new values (0..15)
 *                        on:         The tick (between 0..4095) when the signal should transition from low to high
 *                        off:        The tick (between 0..4095) when the signal should transition from high to low
 *                        
 * 
 *                 Notes while working:             
 *                    Simply input pump # followed by desired PWM, and it will take care of everything for you! ie. move to that PWM position
 *          
 */

      
/////////////////////     Import Libraries as needed      /////////////////////
#include <Wire.h>                       // motor control shield, needed for I2C
#include <Adafruit_PWMServoDriver.h>    // import motor control shield library


/////////////////////     Initalize Servo Motor Shield      /////////////////////

// called this way, it uses the default address 0x40
// you can also call it with a different address you want --> Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x41);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();


//////////////////////////     System Variables      //////////////////////////
// Values to store GUI inputs 
  int IN_pump;            // stores pump number from GUI
  int IN_PWM;             // stores PWM from GUI


// Serial input variables
  String inputString = "";         // a string to hold incoming data
  boolean stringComplete = false;  // whether the string is complete 


/////////////////////     Setup Function      /////////////////////
void setup() {
  // initialize serial communication
  Serial.begin(9600);
  inputString.reserve(200);   // reserve 200 bytes for the inputString

  // Initalize PWM for motor control shield
  pwm.begin(); 
  pwm.setPWMFreq(50);  // Analog servos run at ~50 Hz updates, 20ms
  yield();

  // Print initalize statement
  Serial.println("Hello! Welcome to ServoServer. Ready to accept values");
}

/////////////////////     Looped Function      /////////////////////
void loop() { 
    if (stringComplete) {      
        // Get inputs
        IN_pump = inputString.substring(0,4).toInt();
        IN_PWM = inputString.substring(4,8).toInt();    

        // Pump control
          pwm.setPWM(IN_pump,0,IN_PWM);
          Serial.print(IN_pump); Serial.print(" move to "); Serial.println(IN_PWM);

        // clear the string:
        inputString = "";
        stringComplete = false;
  } 
}


/////////////////////     Read Serial Function      /////////////////////
/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}