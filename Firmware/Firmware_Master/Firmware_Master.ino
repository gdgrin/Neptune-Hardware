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
 *                    Equation-->    mL = A/2[1-cos(θ)] + mL0
 *          
 */

      
/////////////////////     Import Libraries as needed      /////////////////////
#include <avr/interrupt.h>              // interupts
#include <avr/io.h>                     // interupts
#include <Wire.h>                       // motor control shield, needed for I2C
#include <Adafruit_PWMServoDriver.h>    // import motor control shield library


/////////////////////     Initalize Servo Motor Shield      /////////////////////

// called this way, it uses the default address 0x40
// you can also call it with a different address you want --> Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x41);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();


//////////////////////////     System Variables      //////////////////////////

/* Timer Variables
 *  ONLY applies to fluid dispensers
 *  #currentPWMs = Stores current value of pump pwm, needed to calculate trigger with given goal PWM and total time to get there
 *  #counters = Keeps track of how many interrupts were fired 
 *  #max_count = Value of count to dispense more liquid 
 *  #dispensers = When true, main loop will move one more PWM step 
 *  #dispense_counter = Keeps track of how many times max_count was reached
 *  #dispense_limit  = Stores number of times dispense_counter hits before turning off desensing
*/
    int j; // iterates through loops in trigger
    int i; // iterates through loops in main
    int currentPWMs[5]       = {307,307,307,307}; // start at 1.5ms pulses to center (for analog! 50Hz refresh)
    int counters[5]          = {0};
    int max_count[5]         = {0};
    bool dispensers[5]       = {false};
    int dispense_counter[5]   = {0};
    int dispense_limit[5]    = {0};
    
    // LED variables 
    unsigned int LEDtoggle = 0;  //used to keep the state of the LED
    unsigned int LEDcount = 0;   //used to keep count of how many interrupts were fired


/* Values to store GUI inputs 
*/
int IN_pump;            // stores pump number from GUI
int IN_PWM;             // stores PWM from GUI
float IN_Flow;            // stores Flow rate from GUI


/* Serial input variables
 * 
 */
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete


/////////////////////     Timer2 Function      /////////////////////

//Timer2 Overflow Interrupt Vector, called every 1ms
ISR(TIMER2_OVF_vect) {
  LEDcount++;               //Increments the interrupt counter
  if(LEDcount > 999){
    LEDtoggle = !LEDtoggle;    //toggles the LED state
    LEDcount = 0;           //Resets the interrupt counter
  }
  digitalWrite(53,LEDtoggle);


  // Quick loop to set dispense variables to true or false
  for (j=0; j < 5; j++) {
    if (max_count[j] == 0)             //if max_count is not set (equal to 0) check next trigger
      continue;
      
    counters[j]++;                    //increment counter
    
    if (counters[j] >= max_count[j])   //checks if count is equal to the max_count value
      dispensers[j] = true;           // dispense variable is set to true
      // reset counter and increment dispense_counter OUTSIDE of this loop to save time   
  }
  
   
  TCNT2 = 130;           //Reset Timer to 130 out of 255
  TIFR2 = 0x00;          //Timer2 INT Flag Reg: Clear Timer Overflow Flag
};  


/////////////////////     Setup Function      /////////////////////

void setup() {
  // blink an LED for error checking
  pinMode(53,OUTPUT);
  
  // initialize seria l communication
  Serial.begin(9600);
  inputString.reserve(200);   // reserve 200 bytes for the inputString

  // Initalize PWM for motor control shield
  pwm.begin(); 
  pwm.setPWMFreq(50);  // Analog servos run at ~50 Hz updates, 20ms
  //pwm.setPWMFreq(300);
  yield();
  
  //Setup Timer2 to fire every 1ms
  TCCR2B = 0x00;        //Disbale Timer2 while we set it up
  TCNT2  = 130;         //Reset Timer Count to 130 out of 255 
  TIFR2  = 0x00;        //Timer2 INT Flag Reg: Clear Timer Overflow Flag
  TIMSK2 = 0x01;        //Timer2 INT Reg: Timer2 Overflow Interrupt Enable
  TCCR2A = 0x00;        //Timer2 Control Reg A: Normal port operation, Wave Gen Mode normal
  TCCR2B = 0x05;        //Timer2 Control Reg B: Timer Prescaler set to 128


  // Print initalize statement
  Serial.println("Hello! Welcome to ServoServer. Ready to accept values");
}

/////////////////////     Looped Function      /////////////////////

void loop() { 
    if (stringComplete) {
      
        // Get inputs
        IN_pump = inputString.substring(0,4).toInt();
        IN_PWM = inputString.substring(4,8).toInt();
        IN_Flow = inputString.substring(8,12).toFloat();

        if (IN_pump == 5555) {
          for (i=0; i < 5; i++) {
            dispense_limit[i] = IN_PWM-currentPWMs[i];            //set dispense_limit (number of steps to increment PWM by one)
            max_count[i] = 30*1000/abs(dispense_limit[i]);   //Calculate max_count (number of ms between each PWM step)
          }  
        }
        
        // Valve control (no set flow rate)
        if (IN_Flow == 0) {
          currentPWMs[IN_pump] = IN_PWM;
          pwm.setPWM(IN_pump,0,currentPWMs[IN_pump]);
        }

        // Despenser control initiation
        else {
          dispense_limit[IN_pump] = IN_PWM-currentPWMs[IN_pump];            //set dispense_limit (number of steps to increment PWM by one)
          max_count[IN_pump] = IN_Flow*1000/abs(dispense_limit[IN_pump]);   //Calculate max_count (number of ms between each PWM step)    
        }

        // clear the string:
        inputString = "";
        stringComplete = false;
        for (i=0; i < 5; i++) {
          Serial.print(i);
          Serial.print(" curentPWMs: ");
          Serial.println(currentPWMs[i]);
        }
  } 

    // Take care of dispensors here, fires each loop
    // Checks each dispensor for True and set back to false
    for (i=0; i < 5; i++) {
      // if dispensers[i] is true, time to change PWM by one
      if (dispensers[i]) {
        // if our counter exceeds our limit, stop this process!
        if (abs(dispense_counter[i]) >= abs(dispense_limit[i])) {
          //turn off this dispenser, stop everything
          max_count[i] = 0; //turning off max_count will stop the interupt from touching this
          dispense_counter[i] = 0;
          dispense_limit[i] = 0;
        }
        // if counter bellow limit, change PWM by one
        else {
          if (dispense_limit[i] < 0) {
             currentPWMs[i]--;                 // set current PWM to new value
             dispense_counter[i]--;            // decrese dispense_counter  
             pwm.setPWM(i,0,currentPWMs[i]); //Send PWM to servo      
          }
          else {
            currentPWMs[i]++;                 // set current PWM to new value
            dispense_counter[i]++;            // increment dispense_counter   
            pwm.setPWM(i,0,currentPWMs[i]); //Send PWM to servo            
          }  
          Serial.print("fired "); Serial.print(i); Serial.println(currentPWMs[i]);             
        }
        counters[i] = 0;                  // counter is reset
        dispensers[i] = false;            // reset dispenser boolean
      }
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
