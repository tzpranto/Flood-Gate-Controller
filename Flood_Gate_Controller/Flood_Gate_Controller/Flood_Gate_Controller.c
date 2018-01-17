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
#define F_CPU 1000000
#include <util/delay.h>
#include "lcd.h"
#include "motor.h"
#include "sonar.h"


#define WATER_MAX_HEIGHT 30
#define WATER_TO_SONAR 4

#define OFF_TIME 100
#define ROTATION_TIME 1000

int checkManualGateControl();

typedef enum Height_State {LOW, MEDIUM, HIGH, CRITICAL} Height_State;


//keeping track of the last 3 states of water height and the state of the gate
Height_State water_state1;
Height_State water_state2;
Height_State water_state3;
volatile Height_State gate_state;




//manually rotate the gate anti-clockwise using interrupt 1

ISR(INT1_vect)
{
	_delay_ms(500);
	GIFR |= (1<<INTF1);
	if(checkManualGateControl()) {
		int pinD = PIND;
		int pinD3 = pinD & (1<<PIND3);
		
		//problem with the AtMega32 chip, so explicitly check if the INT1 pin has high input
		if(gate_state != CRITICAL && pinD3 != 0) {
			DC_Motor_Off();
			_delay_ms(OFF_TIME);
			DC_Motor_AntiClockwiseRotation();
			_delay_ms(ROTATION_TIME);
			DC_Motor_Off();
			gate_state++;
		}
	}
}


//manually rotate the gate clockwise using interrupt 2

ISR(INT2_vect)
{
	_delay_ms(500);
	GIFR |= (1<<INTF2);
	if(checkManualGateControl()) {
		int pinB = PINB;
		int pinB2 = pinB & (1<<PINB2);
		
		//problem with the AtMega32 chip, so explicitly check if the INT1 pin has high input
		if(gate_state != LOW && pinB2 != 0) {
			DC_Motor_Off();
			_delay_ms(OFF_TIME);
			DC_Motor_ClockwiseRotation();
			_delay_ms(ROTATION_TIME);
			DC_Motor_Off();
			gate_state--;
		}
	}
}



/****
 * check if the manual gate control is enabled or disabled
****/
int checkManualGateControl() {
	int pinA = PINA;
	int pinA3 = pinA & (1<<PINA3);
	if(pinA3 != 0) return 1;
	return 0;
}


/*****
 * dc motor controller, it checks whether all three water state are same and
 * if the gate needs to be closed or opened and controls the motor accordingly
*****/
void control_DC_Motor() {
	if(checkManualGateControl() == 1) return;
	if(water_state1 == water_state2 && water_state2 == water_state3 && water_state1 != gate_state) {
		DC_Motor_Off();
		_delay_ms(OFF_TIME);
		if(water_state1 < gate_state) {
			DC_Motor_ClockwiseRotation();
			if(gate_state - water_state1 == 1) _delay_ms(ROTATION_TIME);
			else if(gate_state - water_state1 == 2) _delay_ms(2*ROTATION_TIME);
			else if(gate_state - water_state1 == 3) _delay_ms(3*ROTATION_TIME);
		}
		else {
			DC_Motor_AntiClockwiseRotation();
			if(gate_state - water_state1 == -1) _delay_ms(ROTATION_TIME);
			else if(gate_state - water_state1 == -2) _delay_ms(2*ROTATION_TIME);
			else if(gate_state - water_state1 == -3) _delay_ms(3*ROTATION_TIME);
		}
		gate_state = water_state1;
		DC_Motor_Off();
	}
}



//sounds the buzzer if water level critical
void control_Buzzer() {
	if(water_state1 == water_state2 && water_state2 == water_state3 && water_state1 == CRITICAL)
		PORTA |= (1<<PINA0);
	else if(water_state1 == water_state2 && water_state2 == water_state3 && water_state1 != CRITICAL)
		PORTA &= ~(1<<PINA0);
}



/********
 * The higher the distance from sonar to water, lower the water level.
 * So when distance is highest get_water_state(int) returns LOW,
 * when distance is lowest it returns HIGH
 ********/
Height_State get_water_state(double height) {
	if(height < 0) height = 0; 
	if(height <= WATER_MAX_HEIGHT/4.0 && height >= 0) return LOW;
	else if(height > WATER_MAX_HEIGHT/4.0 && height <= WATER_MAX_HEIGHT/2.0) return MEDIUM;
	else if(height > WATER_MAX_HEIGHT/2.0 && height <= (3*WATER_MAX_HEIGHT)/4.0) return HIGH;
	return CRITICAL;
}



//keeps track of the last 3 states of water height
void state_tracker(Height_State state) {
	water_state1 = water_state2;
	water_state2 = water_state3;
	water_state3 = state;
}




int main(void)
{
	//initial water level
	water_state1 = water_state2 = water_state3 = gate_state = LOW;

	//initialize pins
    	DDRD = 0b11110011;
    	DDRC = 0xFF;
	DDRA |= (1<<PINA0);
	DDRA &= ~(1<<PINA3);
	
	//buzzer turned off
	PORTA &= ~(1 << PINA0);
	
	DDRB &= ~(1<<PINB2);
	
	//interrupt related instructions	
	GICR |= (1<<INT0);
	GICR |= (1<<INT1);
	GICR |= (1<<INT2);
	MCUCR |= (1<<ISC00);
	MCUCR |= (1<<ISC10);
	MCUCR |= (1<<ISC11);
	MCUCSR |= (1<<ISC2);
	
	TCCR1A = 0;
	

	double sonar_reading = 0;
	double water_height;
	char water_height_display[16];
	
	sei();
	
	
	//initialize LCD and display initial strings
	Lcd4_Init();
	
	Lcd4_Clear();
	Lcd4_Set_Cursor(1, 0);
	Lcd4_Write_String("Level: ");
	
	Lcd4_Set_Cursor(2, 0);
	Lcd4_Write_String("Height: ");
	
	
	//Initialize DC Motor	
	DC_Motor_Config();


	while(1) {
	
		//trigger sonar
		trigger_sonar();

		//wait for echo to arrive
		wait_for_echo();

		//get distance of water	
		sonar_reading = get_distance_from_sonar();

		
		//calculate water height
		water_height = (WATER_MAX_HEIGHT + WATER_TO_SONAR) - sonar_reading;
		if(water_height < 0) water_height = 0;

		//convert double water height to string
		dtostrf(water_height, 6, 2, water_height_display);
		
		//display height in LCD
		Lcd4_Set_Cursor(2, 10);
		Lcd4_Write_String(water_height_display);
		
	
		//get state according to height and display state on LCD
		Lcd4_Set_Cursor(1, 7);
		Height_State currentState = get_water_state(water_height);
		if(currentState == LOW) {
			Lcd4_Write_String("LOW      ");
		}
		else if(currentState == MEDIUM) {
			Lcd4_Write_String("MEDIUM   ");
		}
		else if(currentState == HIGH) {
			Lcd4_Write_String("HIGH     ");
		}
		else {
			Lcd4_Write_String("CRITICAL ");
		}


		//updated last 3 water states
		state_tracker(currentState);

		//buzzer controller
		control_Buzzer();
		
		//motor controller
		control_DC_Motor();
		
		_delay_ms(100);
	}
}

