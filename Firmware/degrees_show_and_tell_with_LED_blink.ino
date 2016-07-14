/*                
 *           Easy Show and Tell style code 
 *           -Zach Lasiuk
 *                
 *                
 *                What does it do:
 *                   -moves servos on shield pins 0-3 to a specified degree (1-180)
 *                   -blinks LED on pin 53 to show the 1 second timmer is uninterupted by serial communication and internal processing
 *                
 *                How to run code:
 *                    -Need Arduino Mega 2560 with servo motor shield attached
 *                    -Plug in servos (should be FiTec FS5106B) to pins 0-3 of Arduino shield
 *                      -To use another servo, calibrate SERVOMIN and SERVOMAX to be the PWM values to make the servo horn 0 and 180 degrees respectivly
 *                    -Upload script
 *                    -Open Serial Monitor
 *                    -Type in number in degrees you want the servo horn to rotate to, eg. 102
 * 
 * 
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
 *                    Servo 1 and 2: SERVOMIN=180 SERVOMAX=540
 *                      -FiTec FS5106B
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

int SERVOMIN = 180;              // this is the 'minimum' pulse length count (out of 4096)
int SERVOMAX = 540;              // this is the 'minimum' pulse length count (out of 4096)
int input_degrees = 0;           // user inputed number in degrees, 3 digits
int pulselength = 0;             // length that PWM signal is on for

uint8_t servo_num = 0;           // our servo # counter

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

unsigned int toggle = 0;  //used to keep the state of the LED
unsigned int count = 0;   //used to keep count of how many interrupts were fired



/////////////////////     Timer2 Function      /////////////////////

//Timer2 Overflow Interrupt Vector, called every 1ms
ISR(TIMER2_OVF_vect) {
  count++;               //Increments the interrupt counter
  if(count > 999){
    toggle = !toggle;    //toggles the LED state
    count = 0;           //Resets the interrupt counter
  }
  digitalWrite(53,toggle);
  TCNT2 = 130;           //Reset Timer to 130 out of 255
  TIFR2 = 0x00;          //Timer2 INT Flag Reg: Clear Timer Overflow Flag
};  


/////////////////////     Setup Function      /////////////////////

void setup() {
  // blink an LED for error checking
  pinMode(53,OUTPUT);
  
  // initialize serial communication
  Serial.begin(9600);
  inputString.reserve(200);   // reserve 200 bytes for the inputString

  // Initalize PWM for motor control shield
  pwm.begin(); 
  pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates
  yield();
  
  //Setup Timer2 to fire every 1ms
  TCCR2B = 0x00;        //Disbale Timer2 while we set it up
  TCNT2  = 130;         //Reset Timer Count to 130 out of 255 
  TIFR2  = 0x00;        //Timer2 INT Flag Reg: Clear Timer Overflow Flag
  TIMSK2 = 0x01;        //Timer2 INT Reg: Timer2 Overflow Interrupt Enable
  TCCR2A = 0x00;        //Timer2 Control Reg A: Normal port operation, Wave Gen Mode normal
  TCCR2B = 0x05;        //Timer2 Control Reg B: Timer Prescaler set to 128
}


/////////////////////     Looped Function      /////////////////////

void loop() { 
  if (stringComplete) {

      input_degrees = inputString.substring(0,3).toInt();
   
      pulselength = map(input_degrees, 0, 180, SERVOMIN, SERVOMAX); 
      pwm.setPWM(0,0,pulselength);
      pwm.setPWM(1,0,pulselength);
      pwm.setPWM(2,0,pulselength);
      pwm.setPWM(3,0,pulselength);     
      
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
