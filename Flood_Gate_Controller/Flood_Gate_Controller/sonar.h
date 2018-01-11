#ifndef SONAR_H
#define SONAR_H


#include <util/delay.h>
#include <avr/io.h>


static volatile unsigned int pulse = 0;
static volatile int pulse_reached = 0;

//interrupt for when sonar is triggered and echo is received
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


//triggers the sonar
void trigger_sonar() {
	PORTD |= (1<<PIND0);
	_delay_us(15);
	PORTD &= ~(1<<PIND0);
}


//waits for echo to arrive after triggering the sonar
void wait_for_echo() {
	while(pulse_reached == 0);
}


//calculate obstacle distance from sonar after echo arrives
double get_distance_from_sonar() {
	double sonar_reading = (pulse*174.0)/10000;
	return sonar_reading;
}




#endif //SONAR_H
