/*********************************************************
  trat09_MPR121Test.ino

  This program uses the library for the MPR121 12-channel
  capacitive touch sensor to detect capactive touch events.

  The MPR121 uses I2C on Metro Mini pads _ and __. It also
  provides an interrupt-driven output that can be tied to a
  pin change or external interrupt to drastically reduce the
  amount of time spend polling the MPR121's state over I2C.
  Huzzah!

  Adapted from the MPR121Test sketch, written by Limor Fried
  (LadyAda) for Adafruit Industries.

  CS342 Fall 2018
  Week 11: Communication
  Caitrin Eaton

**********************************************************/

#include <avr/io.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "USART.h"

float frequency [12] = {16.35, 17.32, 18.35, 19.45, 20.60, 21.83, 23.12, 24.50,
                        25.96, 27.50, 29.14, 30.87
                       };
int octave = 4;
int duration[12] = {500, 500, 250, 250, 250, 1000, 500,
                    500, 750, 250, 250, 750
                   }; // in ms

Adafruit_MPR121 cap = Adafruit_MPR121();

/* For tracking MPR121 state changes (touches and releases) */
uint16_t currTouched = 0;
int note;
char t[100];
int ms = 0;


uint16_t sensedADC=0; 
float mean=0;
char msg[124];
int angle;




ISR(ADC_vect){
  int N=50;
  sensedADC=ADCL;
  sensedADC|=(ADCH &0x03<<8);
  mean=mean *float(N-1)/float(N)+float(sensedADC)/float(N);
  
  
  angle=mean*(360/1023);
  sprintf(msg, "online mean: \t%d\n",int(angle));
  printString(msg);
  
}





ISR(TIMER1_COMPA_vect) {
  TCCR1A = 60;
  TCCR1B = 9;
}

ISR(TIMER1_COMPB_vect){
  
}
int main() {

  /* Ick. I haven't yet tracked down the root of this problem, but
     for now the MPR121 won't work without Arduino's built-in init().
     It must be configuring something that the MPR121 uses, but doesn't
     configure itself because it expects Arduino to have taken care of
     it already. Likely candidates are: timers,  */
  init();

  /* Configure comm over USB */
  initUSART();
 

  /* Configure GPIO
        - I2C SCLK on pad _ shound be an [input / output]
        - I2C SDA on pad _ shound be an [input / output]
        - MPR121's IRQ on pad on pad _ shound be an [input / output] */
  // TODO: Configure the GPIO


  //Configure pad 3 to be an input
  DDRC = 0b00100000;
  DDRD = 0;
  PORTD = 0b00000100;

  // Configuring Timer 1 to generate a sound wave
  DDRB = 0x02; // PB 1 is output
  TCCR1A = 0b01000000; // CTC mode - pg 141 and toggling COM1A?
  //TCCR1B = 0b00001001; // prescale by 1, WGMO[2] = 0
  OCR1AH = 0x77;
  OCR1AL = 0x71;
  //TIMSK1=0b00000100;

  //Configure an analog read from pad 3
ADMUX=0b01000011;
ADCSRA=0b11101111;
ADCSRB=0b00000000;


 
  if ( !cap.begin(0x5A) ) {         // Test that the sensor is up and running
    
    while ( true );
  }

  currTouched = cap.touched();      // Get the currently touched pads
  /* Configure an interrupt -- without an ISR  -- to detect changes in the
     MPR121's state. We can't read the sensor from the ISR because _. But
     we can be smart about how often we query the sensor by using this flag
     in the while loop. Just keep in mind that everything in main() will be
     lower priority than any ISRs!

     To detect when an interrupt has been triggered with an ISR, check the
     value of its flag bit (e.g. INTFn or PCIFn) in the main control loop. */
  // TODO: configure the interrupt (but do not enable its ISR)
  EICRA = 0b00000010;
  /* Global interrupt enable */
  SREG |= 0x80;

  /* Control loop */
  while ( true ) {

    printString("");

    // TODO: If the interrupt flag is high, the MPR121's state has changed.

    // TODO: Outside of an ISR, we have to manually clear the interrupt flag.
    // TODO: Read in the new MPR121 state
    if (EIFR & 0b00000001) {
      currTouched = cap.touched();
      // Print out some helpful information
     
      if ( currTouched > 0 ) { // a capacitive touch pad is being touched
        
        note = int(log(currTouched) / log(2));
        if (note >= 0 && note <= 11) {
          TCCR1B = 0b00001001;
          float t = frequency[note] * pow(2, octave + 1);
          t = 16000000 / t - 1;
          OCR1AH = (uint16_t(t) & 0xFF00) >> 8;
          OCR1AL = (uint16_t(t) & 0x00FF);
        }
       
      } else {                     // all capacitive touch pads have been released
        TCCR1B = 0x00;
       
      }
      EIFR = 0xFF;
    }
  }
  return 0;
}
