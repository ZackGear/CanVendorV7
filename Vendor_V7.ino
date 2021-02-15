//   ==================================
//   |       Created 9FEB2021         |
//   |       BY "Blackfin"            |
//   |    an Edison Member on         |
//   |     forum.arduino.cc           |
//   |    Last Modified 9FEB2021      |
//   ==================================

#include <Servo.h>

Servo servo;    // Create servo object to control a servo

// Constant variables that won't change, used to assign hardware connected pins
const uint8_t SERVO_PIN = 9;    // Arduino pin connected to servo motor's pin
const uint8_t BUTTON_PIN = 7;   // Arduino pin connected to DISPENSE button
const uint8_t BACKLIGHT_PIN = 2;  // Arduino pin connected to backlight NPN transistor
const uint8_t BUTTON_LIGHT_PIN = 12;  // Arduino pin connected to button

enum eVendingStates
{
    ST_VEND_IDLE=0,
    ST_VEND_WAITBLNK,
    ST_VEND_TSRVO1,
    ST_VEND_TSRVO2
};

enum eButtonLightStates
{
    ST_BL_IDLE=0,
    ST_BL_ON,
    ST_BL_OFF
      
};

enum eBacklightStates
{
    ST_BKL_IDLE=0,
    ST_BKL_TIME
      
};

uint8_t
    g_lastBTNState;
bool
    g_bBLFlash,
    g_bBKLTrigger;    

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(9600); // Initialize serial communication at 9600 bits per second:
    servo.attach(SERVO_PIN);
    servo.write( 180 );
    
    // initialize the pushbutton pin as an pull-up input
    // the pull-up input pin will be HIGH when the switch is open and LOW when the switch is closed.
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    g_lastBTNState = digitalRead( BUTTON_PIN );
    
    servo.detach(); // Detach all Servo's attached, to save power and prevent heat/noise from coil buildup

    g_bBLFlash = false;
    g_bBKLTrigger = false;
    
}//setup

void loop()
{
    Vending_FSM();
    ButtonLight_FSM();
    Backlight_FSM();    
        
}//loop


void Vending_FSM( void )
{
    static uint8_t
        stateVend = ST_VEND_IDLE;
    static uint32_t
        timeVend;
    uint32_t
        timeNow = millis();
    uint8_t
        currentBTNState;
    
    switch( stateVend )
    {
        case    ST_VEND_IDLE:
            //as original; if pin transitions from low to high, process            
            currentBTNState = digitalRead(BUTTON_PIN); // read the state of the switch/button:
            if( g_lastBTNState != currentBTNState )
            {
                Serial.println("The state changed from LOW to HIGH");
                g_lastBTNState = currentBTNState;
                if( currentBTNState == HIGH )
                {
                    Serial.println("Start Vending");
                    //start button light blinking
                    g_bBLFlash = true;
                    //and move to state to wait for blinking to end
                    stateVend = ST_VEND_WAITBLNK;
                    
                }//if
                
            }//if        
            
        break;

        case    ST_VEND_WAITBLNK:
            //when blinking done, blink FSM sets this flag false
            if( g_bBLFlash == false )
            {
                //when blinking done, start timing backlight
                g_bBKLTrigger = true;
                //mov servo
                servo.attach(SERVO_PIN);                    
                servo.write( 0 );
                //get time
                timeVend = timeNow;
                //and move to time servo in this position
                stateVend = ST_VEND_TSRVO1;
                                    
            }//if
            
        break;

        case    ST_VEND_TSRVO1:
            //when servo has been here for 2-sec...
            if( timeNow - timeVend >= 2000ul )
            {
                //...switch it back
                servo.write( 180 );
                //get time
                timeVend = timeNow;
                //and move to time this position
                stateVend = ST_VEND_TSRVO2;
                
            }//if
            
        break;

        case    ST_VEND_TSRVO2:
            //after 1-sec...
            if( timeNow - timeVend >= 1000ul )
            {
                //detach (original logic)
                servo.detach();
                //and go back to idle state
                stateVend = ST_VEND_IDLE;
                Serial.println("End Vending");

            }//if
            
        break;
                
    }//switch
    
}//Vending_FSM

void ButtonLight_FSM( void )
{
    static uint8_t
        stateBL = ST_BL_IDLE,
        countBL;
    static uint32_t
        timeBL;
    uint32_t
        timeNow = millis();

    switch( stateBL )
    {
        case    ST_BL_IDLE:
            //wait for flag to be set indicating start of vending cycle
            if( g_bBLFlash == true )
            {                
                //set the # of flashes we want
                countBL = 5;
                //turn light on
                digitalWrite(BUTTON_LIGHT_PIN, HIGH);
                //get time
                timeBL = timeNow;
                //and move to time on-state
                stateBL = ST_BL_ON;        
                
            }//if
            
        break;

        case    ST_BL_ON:
            //after 100mS...
            if( timeNow - timeBL >= 100ul )
            {
                //turn off the LED
                digitalWrite(BUTTON_LIGHT_PIN, LOW);
                //get time
                timeBL = timeNow;
                //decrement counter
                countBL--;
                //if count hits zero...
                if( countBL == 0 )
                {
                    //we're done flashing; clear flag and go back to idle state
                    g_bBLFlash = false;
                    stateBL = ST_BL_IDLE;
                    
                }//if
                else
                    //not done yet; time off-state
                    stateBL = ST_BL_OFF;
                
            }//if
            
        break;

        case    ST_BL_OFF:
            //been off for 50mS?
            if( timeNow - timeBL >= 50ul )
            {
                //yep; turn on the light again
                digitalWrite(BUTTON_LIGHT_PIN, HIGH);
                //get time
                timeBL = timeNow;
                //and move to time the on-state again
                stateBL = ST_BL_ON;
                
            }//if
            
        break;
            
    }//switch
    
}//ButtonLight_FSM

void Backlight_FSM( void )
{
    static uint8_t
        stateBKL = ST_BKL_IDLE;
    static uint32_t
        timeBKL;
    uint32_t
        timeNow = millis();

    switch( stateBKL )
    {
        case    ST_BKL_IDLE:
            //trigger flag goes true when we want to turn on the backlight
            if( g_bBKLTrigger == true )
            {
                //clear the flag
                g_bBKLTrigger = false;
                //turn on the backlight
                digitalWrite(BACKLIGHT_PIN, HIGH);
                //get the time
                timeBKL = timeNow;
                //and time the backlight
                stateBKL = ST_BKL_TIME;
                
            }//if
            
        break;

        case    ST_BKL_TIME:
            //if the backlight trigger goes true when we're already timing...
            if( g_bBKLTrigger == true )
            {                
                //clear the flag again and...
                g_bBKLTrigger = false;            
                //...reset the timer to give a new 10-sec count
                timeBKL = timeNow;
                
            }//if
            else
            {
                //if 10-sec elapsed
                if( timeNow - timeBKL >= 10000ul )
                {
                    //turn off the backlight
                    digitalWrite(BACKLIGHT_PIN, LOW);
                    //and go back to idle
                    stateBKL = ST_BKL_IDLE;
                    
                }//if
                
            }//else
            
        break;
        
    }//switch
    
}//Backlight_FSM
