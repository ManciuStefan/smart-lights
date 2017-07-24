#include <DHT.h>
#include <DHT_U.h>
#include <IRremote.h>
#include <RGBlib.h>
#include <stdint.h>
#include <ffft.h>

#define DHT_PIN 2
#define DHT_TYPE DHT11
#define IR_RECV_PIN 11
#define MIC_INPUT 0

/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO 
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino model, check
  the Technical Specs of your board  at https://www.arduino.cc/en/Main/Products
  
  This example code is in the public domain.

  modified 8 May 2014
  by Scott Fitzgerald
  
  modified 2 Sep 2016
  by Arturo Guadalupi
  
  modified 8 Sep 2016
  by Colby Newman
*/



RGBlib controller(9, 10, 11);
DHT dht(DHT_PIN, DHT_TYPE);
IRrecv irrecv(IR_RECV_PIN);
decode_results results;

volatile  byte  position = 0;
volatile  long  zero = 0;

int16_t capture[FFT_N];      /* Wave captureing buffer */
complex_t bfly_buff[FFT_N];   /* FFT buffer */
uint16_t spektrum[FFT_N/2];   /* Spectrum output buffer */


int c = 0;
int minSound = 1024;
int maxSound = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  analogReference(EXTERNAL);
  adcInit();
  adcCalb();
}

// the loop function runs over and over again forever
void loop() {
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    irrecv.resume(); // Receive the next value
  }

  if (position == FFT_N)
    {
        fft_input(capture, bfly_buff);
        fft_execute(bfly_buff);
        fft_output(bfly_buff, spektrum);

        for (byte i = 0; i < 10; i++){
            Serial.print(spektrum[i]);
            Serial.print("\t");
        }
        Serial.println();
        position = 0;
    } 

  
  
/*
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");

  
  c++;
  c = c % 360;
  controller.setColorHSB(c,100,100);
  delay(2000);
  */
}

ISR(ADC_vect)
{
    if (position >= FFT_N)
        return;

    capture[position] = ADC + zero;
    if (capture[position] == -1 || capture[position] == 1)
        capture[position] = 0;

    position++;
}

void adcInit(){
  /**   REFS0 : VCC use as a ref, 
   *    IR_AUDIO : channel selection, 
   *    ADEN : ADC Enable, 
   *    ADSC : ADC Start, 
   *    ADATE : ADC Auto Trigger Enable, 
   *    ADIE : ADC Interrupt Enable,  
   *    ADPS : ADC Prescaler  
   */
  
    // free running ADC mode, f = ( 16MHz / prescaler ) / 13 cycles per conversion 
    ADMUX = _BV(REFS0) | MIC_INPUT; // | _BV(ADLAR); 
    
    //  ADCSRA = _BV(ADSC) | _BV(ADEN) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) 
    //prescaler 64 : 19231 Hz - 300Hz per 64 divisions
    ADCSRA = _BV(ADSC) | _BV(ADEN) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); 
    
    // prescaler 128 : 9615 Hz - 150 Hz per 64 divisions, better for most music
    sei();
}

void adcCalb(){
    Serial.println("Start to calc zero");
    long midl = 0;
    // get 2 meashurment at 2 sec
    // on ADC input must be NO SIGNAL!!!
    for (byte i = 0; i < 2; i++) {
        position = 0;
        delay(100);
        midl += capture[0];
        delay(900);
    }
    zero = -midl/2;
    Serial.println("Done.");
}
