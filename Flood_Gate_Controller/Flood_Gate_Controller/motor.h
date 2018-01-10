#ifndef MOTOR_H
#define MOTOR_H

#include <util/delay.h>
#include <avr/io.h>


//configure DC Motor
void DC_Motor_Config() {
	DDRA |= (1 << PINA1);
	DDRA |= (1 << PINA2);
	PORTA &= ~(1 << PINA1);
	PORTA &= ~(1 << PINA2);
}


//Rotate the DC Motor Clockwise
void DC_Motor_ClockwiseRotation() {
	PORTA &= ~(1 << PINA1);
	PORTA |= (1 << PINA2);
}

//Rotate the DC Motor AntiClockwise
void DC_Motor_AntiClockwiseRotation() {
	PORTA |= (1 << PINA1);
	PORTA &= ~(1 << PINA2);
}


//stop rotating the motor
void DC_Motor_Off() {
	PORTA &= ~(1 << PINA1);
	PORTA &= ~(1 << PINA2);
}


#endif //MOTOR_H
