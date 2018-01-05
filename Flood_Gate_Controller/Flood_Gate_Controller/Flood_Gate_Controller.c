/*
 * exp6.c
 *
 * Created: 4/20/2017 10:34:05 AM
 *  Author: USER
 */ 


#define D4 eS_PORTD4
#define D5 eS_PORTD5
#define D6 eS_PORTD6
#define D7 eS_PORTD7
#define RS eS_PORTC6
#define EN eS_PORTC7

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "lcd.h"


#define WATER_MAX_HEIGHT 15
#define WATER_TO_SONAR 4

static volatile unsigned int pulse = 0;
static volatile int pulse_reached = 0;

typedef enum WATER_STATE {LOW, MEDIUM, HIGH, CRITICAL} water_state;

water_state curr_water_state;
water_state prev_water_state;

/********
 * The higher the distance from sonar to water, lower the water level.
 * So when distance is highest get_water_state(int) returns LOW,
 * when distance is lowest it returns HIGH
 ********/
water_state get_water_state(double height) {
	height -= WATER_TO_SONAR;
	if(height < 0) height = 0; 
	if(height <= WATER_MAX_HEIGHT/4.0 && height >= 0) return CRITICAL;
	else if(height > WATER_MAX_HEIGHT/4.0 && height <= WATER_MAX_HEIGHT/2.0) return HIGH;
	else if(height > WATER_MAX_HEIGHT/2.0 && height <= (3*WATER_MAX_HEIGHT)/4.0) return MEDIUM;
	return LOW;
}



ISR(INT0_vect)
{
	if (pulse_reached == 1)
	{
		TCCR1B = 0;
		pulse=TCNT1;
		TCNT1=0;
		pulse_reached=0;
	}
	if (pulse_reached==0)
	{
		TCCR1B|=(1<<CS10);
		pulse_reached=1;
	}
}





int main(void)
{
    DDRD = 0b11111011;
    DDRC = 0xFF;
	DDRA |= 0x01;
	PORTA &= ~(1 << PINA0);
		
	GICR|=(1<<INT0);
	MCUCR|=(1<<ISC00);
	
	TCCR1A = 0;
	
	double sonar_reading = 0;
	char sonar_reading_display[16];
	sei();
	
	Lcd4_Init();
	
	Lcd4_Clear();
	Lcd4_Set_Cursor(1, 0);
	Lcd4_Write_String("Level: ");
	
	Lcd4_Set_Cursor(2, 0);
	Lcd4_Write_String("Distance: ");
	
	while(1) {
		PORTD|=(1<<PIND0);
		_delay_us(15);
		PORTD &=~(1<<PIND0);
		while(pulse_reached == 0);
		sonar_reading = (pulse*174.0)/10000;
		dtostrf(sonar_reading, 6, 2, sonar_reading_display);
		Lcd4_Set_Cursor(2, 10);
		Lcd4_Write_String(sonar_reading_display);
		Lcd4_Set_Cursor(1, 7);
		if(get_water_state(sonar_reading) == LOW) {
			Lcd4_Write_String("LOW      ");
			PORTA &= ~(1<<PINA0);
		}
		else if(get_water_state(sonar_reading) == MEDIUM) {
			Lcd4_Write_String("MEDIUM   ");
			PORTA &= ~(1<<PINA0);
		}
		else if(get_water_state(sonar_reading) == HIGH) {
			Lcd4_Write_String("HIGH     ");
			PORTA &= ~(1<<PINA0);
		}
		else {
			Lcd4_Write_String("CRITICAL ");
			PORTA |= (1<<PINA0);
		}
		_delay_ms(100);
	}
	
    
}

