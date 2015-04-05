#include <MicroView.h>		// include MicroView library

int loopCnt = 0;
int lightPIN = 3;        // set light control pin
int dialPin = A1;        // select the input pin for the potentiometer
int dialValue = 0;       // variable to store the value coming from the pot
int vBatPin = A2;        // input pin for the battery voltage sense
int vBatValue = 0;       // variable to store the battery voltage
float avgDialValue = 0;  // variable to store the average sensor value
float dialSetpoint = 0;  // the setpoint of the dial
float dialMoving = 0;    // is someone turning the dial?
unsigned long dialSetTime = 0;
float avgBatValue = 793; // variable to store the average battery value (793=12V)
float pwmSetting = 0.0;  // the pwm to supply to the light (0 to 255)
int   lowBatCount = 0;   // count the number of loops with low battery
float batPercent = 100;  // The battery charge in percent
unsigned long   batPercentTime = 0;// the time of the last battery percentage estimate
int   batteryGood = 1;   // flag to set when the battery is dead 
int   initComplete = 0;  // flag to set after measurements have stabelized 
unsigned long time = 0;  // Count absolute time (long is big enough to not loop within many battery lives!)

#define NUM_AVG_POT 12.0
#define NUM_AVG_BAT 30.0
#define MUM_AVG_BAT_PERCENT = 100.0

#define CALIBRATE 0

MicroViewWidget *widget;	// declare widget pointer

void setup() {                
        // initialize serial communication at 9600 bits per second:
        Serial.begin(9600);
        
  	uView.begin();			// start MicroView
	uView.clear(PAGE);		// clear page
	pinMode(lightPIN, OUTPUT);		// initialize the digital pin as an output.
	//widget = new MicroViewSlider(0,10,0,100,WIDGETSTYLE0);		// set widget as gauge STYLE1
	uView.clear(PAGE);		// clear page

	setPwmFrequency(lightPIN,256);	// set PWM frequency to about 31K

	pinMode(dialPin, INPUT);			// make pin as INPUT
	pinMode(vBatPin, INPUT);			// make pin as INPUT

}

void loop() {
        // Read analog values
  	dialValue = analogRead(dialPin);	// read Potentiometer
  	vBatValue = analogRead(vBatPin);	// read the battery Voltage
        // Calculated moving averages on the inputs
        avgDialValue = (avgDialValue*(NUM_AVG_POT-1.0) + float(dialValue)) / NUM_AVG_POT;
        avgBatValue = (avgBatValue*(NUM_AVG_BAT-1.0) + float(vBatValue)) / NUM_AVG_BAT;
        
        // Check if the dial is moving
        float dialValue = avgDialValue/1020.0*100;              // The dial value from 0 to 100
        // Check when dial has moved far enough to change setpoint 
        if(abs(dialSetpoint-dialValue) > 3){
          dialMoving = 1;
          dialSetpoint = dialValue;
          dialSetTime = time;
  	  //uView.clear(PAGE);		// clear page
          //widget->drawFace();
        }
        if(dialMoving && (time-dialSetTime) > 33){   // If it hasn't moved for 1/2 second
          dialMoving = 0;
  	  //uView.clear(PAGE);		// clear page
        }

        // Calculate battery voltage
        float batteryVoltage = avgBatValue/1024.0*5.0*3.098765; // The battery voltage in V
        // Override the dial during calibration
        if(CALIBRATE){
          dialValue = 0;
          if(loopCnt < 8000) dialValue = 20;
          if(loopCnt < 7000) dialValue = 40;
          if(loopCnt < 6000) dialValue = 60;
          if(loopCnt < 5000) dialValue = 80;
          if(loopCnt < 4000) dialValue = 100;
        }
        // Calculate the pwm value
        pwmSetting = pow(2, dialSetpoint / 100.0 * 8.0); 
        if(pwmSetting > 255){
          pwmSetting = 255;
        }
        // Turn off power if the battery is dead
        if(batteryGood == 0){
  	  pwmSetting = 0;
        }

        // Calculate the open circuit battery voltage (no load applied) in V
        //  Assume 100 mOhm internal resistance and 5A current draw at max power
        float ocBatVoltage = batteryVoltage + 0.12*5*(pwmSetting/255); 
        // If the power setting has been stable for over a minute, 
        //    use the ocBatteryVoltage to calculate the battery charge 
        if(((time-dialSetTime) > 4000) && ((time-batPercentTime)>1000))
        {
          batPercent = (ocBatVoltage - 12.0) * 100;
          if(batPercent > 100.0){
            batPercent = 100.0;
          } else if (batPercent < 0.0){
            batPercent = 0.0;
          }
          batPercentTime = time;
        }
        if(batteryGood == 0){
          batPercent = 0;
        }
      
        // Draw the outline of the battery
        uView.setColor(1);
        uView.rect(10,0,37,12); 
        uView.rectFill(46,4,4,4);
        // Draw the full side
        uView.rectFill(11,1,batPercent*35/100,10);
        // Erase the empty side
        uView.setColor(0);
        uView.rectFill(11+batPercent*35/100,1,35-batPercent*35/100,10); 
        uView.setColor(1);
        
        // Display power
        uView.setCursor(0, 8);	
        //if(dialMoving){
  	  //widget->setValue(dialValue);		// display the power in the guage widget
          //uView.print("Pwr:");
  	  //uView.print(dialValue, 0);
          //if(dialValue < 99.5){
          //  uView.print(" ");
          //}
        //} else {
          // Display battery voltage
          uView.print("\nBat:");
          uView.print(ocBatVoltage);
          uView.print("V ");
     
          if(batteryGood)
          {
            uView.print(" ");
            uView.print(batPercent);
            uView.print("% ");
          } else {
            uView.print(" Dead Bat");
          }          
     
          // Display time remaining
          uView.print("\nTime:\n");
          // Assume battery lasts 40 mins on max, current draw proportional to pwm
          int displayMins = 0;
          int displayHrs = 1;
          float hours = (batPercent/100) * (0.66) / (pwmSetting/255.0);
          if(hours < 4){
            displayMins = 1;
          }
          if(hours < 59.0/60.0){
            displayHrs = 0;
          }
          float mins = (hours - int(hours))*60;
          if(displayHrs){
            uView.print(int(hours));
            if(displayMins){
              uView.print(":");
            }
          }
          if(displayMins)
          {
            if(int(mins) < 10){
              uView.print("0");
            }
            uView.print(int(mins));
            uView.print(" ");
            if(displayHrs == 0){
              uView.print("mins");
            }
          } else {
            uView.print(" hrs");
          }
        //}
        
  	uView.display();
        
        // Check the battery voltage
        // Count the number of consecutive low battery loops
        if(initComplete && ocBatVoltage < 12.0)
        {
          lowBatCount = lowBatCount + 1;
        } else {
          // reset the counter if the battery isn't low
          lowBatCount = 0;
        }
        // Mark the battery as dead if it was low too long
        if(lowBatCount > 2000)
        {
          batteryGood = 0;
        }
        
        // Set light power
  	analogWrite(lightPIN, pwmSetting);	// set the DUTY cycle of the motorPIN
        
        loopCnt = loopCnt + 1;
        if(loopCnt > 50){
          initComplete = 1;
        }
        // Log data to the serial port for each power setting during calibration 
        if(CALIBRATE && batteryGood)
        {
          if(loopCnt == 3990){
            Serial.print(batteryVoltage);
            Serial.print(", ");
            Serial.print(pwmSetting);
            Serial.print(", ");
          }
          if(loopCnt == 4990){
            Serial.print(batteryVoltage);
            Serial.print(", ");
            Serial.print(pwmSetting);
            Serial.print(", ");
          }
          if(loopCnt == 5990){
            Serial.print(batteryVoltage);
            Serial.print(", ");
            Serial.print(pwmSetting);
            Serial.print(", ");
          }
          if(loopCnt == 6990){
            Serial.print(batteryVoltage);
            Serial.print(", ");
            Serial.print(pwmSetting);
            Serial.print(", ");
          }
          if(loopCnt == 7990){
            Serial.print(batteryVoltage);
            Serial.print(", ");
            Serial.print(pwmSetting);
            Serial.print(", ");
          }
          if(loopCnt == 8990){
            Serial.print(batteryVoltage);
            Serial.print(", ");
            Serial.print(pwmSetting);
            Serial.print(", ");
          }
          if(loopCnt == 9990){
            Serial.print(batteryVoltage);
            Serial.print(", ");
            Serial.println(pwmSetting);
          }
        } 
        if(loopCnt == 10000){
          //Serial.print(batteryVoltage);
          //Serial.print(", ");
          //Serial.println(pwmSetting);
          loopCnt = 0;
        }

  	delay(5);    				// delay 200 ms  
        time = time + 1;
}


// function to set the frequency of the PWM pin
// adapted from http://playground.arduino.cc/Code/PwmFrequency
void setPwmFrequency(int pin, int divisor) {
	byte mode;
	if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
		switch(divisor) {
		case 1: mode = 0x01; break;   //  31 kHz
		case 8: mode = 0x02; break;   //   4 kHz
		case 64: mode = 0x03; break;  // 500 Hz
		case 256: mode = 0x04; break; // 122 Hz
		case 1024: mode = 0x05; break;//  30 Hz
		default: return;
		}
		if(pin == 5 || pin == 6) {
			TCCR0B = TCCR0B & 0b11111000 | mode;
		} else {
			TCCR1B = TCCR1B & 0b11111000 | mode;
		}
	} else if(pin == 3 || pin == 11) {
		switch(divisor) {
		case 1: mode = 0x01; break;
		case 8: mode = 0x02; break;
		case 32: mode = 0x03; break;
		case 64: mode = 0x04; break;
		case 128: mode = 0x05; break;
		case 256: mode = 0x06; break;
		case 1024: mode = 0x7; break;
		default: return;
		}
		TCCR2B = TCCR2B & 0b11111000 | mode;
	}
}
