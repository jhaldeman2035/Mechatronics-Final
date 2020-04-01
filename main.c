//Jacob Haldeman, April 2019
//MEMS 1049 Mechatronics

#include <asf.h>
#include <avr/interrupt.h>

#define RFID_outside PIND0
#define RFID_inside PIND1
#define lock_interrupt PIND3

#define PWM_fwd PORTD5
#define PWM_rev PORTD7

#define stepper_1a PORTC5
#define stepper_1b PORTC4
#define stepper_2a PORTC3
#define stepper_2b PORTC2


void wait(volatile int multiple, volatile char time_choice);
void delay_T_msec_timer1(volatile char choice);

char phase_step = 1;

ISR(INT1_vect) {
	TCCR0B = 0x00; //Stop TIMER0;
	
	PORTC = PORTC & (0 << PORTC1);
	
	while(1) {
		if(PIND & (1 << lock_interrupt)) break; //Leave interrupt only when door is unlocked
	}
	
	PORTC = PORTC | (1 << PORTC1);
	
	EIFR = EIFR | 1 << INT1; //Clear any duplicate interrupts
	TCCR0B =  1<<CS01 | 1<<CS00;  // Restart TIMER0 
}

unsigned char check_temp() {
	ADCSRA |= (1<<ADSC);
	while((ADCSRA & (1<<ADIF)) == 0);
	
	unsigned char temp_sensor = ADCH;
	
	//Safe range is -10C to 35C, or 0.4V to 0.85 V from the sensor
	//Converts to 93 to 197 value in temp_sensor with 1.1V reference
	//133 is approximately room temp (use for demonstration purposes)
	
	if(temp_sensor > 197 || temp_sensor < 93) return 0;
	else return 1;
	
}

void step_CW() {
	switch (phase_step) {
		case 1:
		// step to 4
		PORTC = 0 << stepper_1a | 1 << stepper_2a;
		phase_step = 4;
		break;
		case 2:
		// step to 1
		PORTC = 1 << stepper_1a | 0 << stepper_2b;
		phase_step = 1;
		break;
		case 3:
		// step to 2;
		PORTC = 0 << stepper_1b | 1 << stepper_2b;
		phase_step = 2;
		break;
		case 4:
		// step to 3;
		PORTC = 1 << stepper_1b | 0 << stepper_2a;
		phase_step = 3;
		break;
	}
}

void step_CCW() {
	switch (phase_step) {
		case 1:
		// step to 2
		PORTC = 0 << stepper_1a | 1 << stepper_2b;
		phase_step = 2;
		break;
		case 2:
		// step to 3
		PORTC = 1 << stepper_1b | 0 << stepper_2b;
		phase_step = 3;
		break;
		case 3:
		// step to 4
		PORTC = 0 << stepper_1b | 1 << stepper_2a;
		phase_step = 4;
		break;
		case 4:
		// step to 1
		PORTC = 1 << stepper_1a | 0 << stepper_2a;
		phase_step = 1;
		break;
	}
}

int cycle_door() {
	//not quite 2 full rotations of stepper motor to turn handle
	//200 steps/rev for stepper motor (1.8 deg steps)
	
	cli(); //disable interrupts
	
	//Open door
	//Turn handle
	for(int i = 0; i < 350; i++) { //400 steps = 2 full revolutions
		step_CW();
		wait(25, 2);
	}
	
	
	//Set motor direction forward
	PORTD = PORTD | (1 << PWM_fwd) & ~(1 << PWM_rev);
	OCR0A = 0x1A; //10% duty cycle
	wait(2000, 2); //wait 2 seconds
	OCR0A = 0x00; //0% duty cycle
	PORTD = PORTD & ~(1 << PWM_fwd) & ~(1 << PWM_rev);
	
	//Door is now open
	wait(8000, 2); //wait 10 seconds
	
	//Close door
	
	//Set motor direction reverse
	PORTD = PORTD | (1 << PWM_rev) & ~(1 << PWM_fwd);
	OCR0A = 0x40; //25% duty cycle
	wait(2000, 2); //wait 2 seconds
	OCR0A = 0x00;  //0% duty cycle
	PORTD = PORTD & ~(1 << PWM_fwd) & ~(1 << PWM_rev);
	
	//Return handle to resting position
	for(int i = 0; i < 350; i++) { //800 steps = 4 full revolutions
		step_CCW();
		wait(25, 2);
	}
	
	sei(); //re-enable interrupts
	return 0;
} 


int main (void) {
	
	//Initialize pin directions
	DDRD = 0xf0;
	DDRC = 0b00111110;

	//Set up A/D conversion
	PRR = 0x00; //clear power reduction bit
	ADCSRA = 1 << ADEN | 1 << ADPS2 | 1 << ADPS1 | 1 << ADPS0; //prescaler = 128
	ADMUX = 1 << REFS1 | 1 << REFS0 | 1 << ADLAR | 0 << MUX3 | 0 << MUX2 | 0 << MUX1 | 0 << MUX0; //1.1V reference, left-justify, PC0
	
	wait(10, 2); //Give A/D time to initialize
	
	//Set up PWM
	OCR0A = 0x00; //Set inital duty cycle to 0
	TCCR0A = 1 << COM0A1 | 1 << COM0A0 | 1 << WGM01 | 1 << WGM00; //Non-inverting mode, fast PWM
	TCCR0B = 0 << CS02 | 1 << CS01 | 1 << CS00; //Set PWM frequency to approx. 1 kHz
	
	PORTC = PORTC | (1 << PORTC1);
	
	//Set up interrupts
	PORTD = PORTD | 1 << PORTD3; //Enable pull up resistor for interrupt
	EICRA = 0x00; //Set to trigger on low level
	EIMSK = EIMSK | 1 << INT1;
	sei();
	
	unsigned char safe_temp = 0;
	
	
	
	while(1) {
		safe_temp = check_temp();
		
		if(PIND & (1 << RFID_outside)) {
			cycle_door(); //Open door from outside regardless of temperature 
		} else if((safe_temp == 1) && (PIND & (1 << RFID_inside))) {
			cycle_door(); //Open door from inside only if temperature is within safe range
		}
		
	}
}

void wait(volatile int multiple, volatile char time_choice) {
	/* This subroutine calls others to create a delay.
	Total delay = multiple*T, where T is in msec and is the delay
	created by the called function.
	Inputs: multiple = number of multiples to delay, where multiple is
	the number of times an actual delay loop is called.
	Outputs: None
	*/
	while (multiple > 0) {
		delay_T_msec_timer1(time_choice);
		multiple--;
	}
} // end wait()

void delay_T_msec_timer1(volatile char choice) {
	//*** delay T ms **
	/* This subroutine creates a delay of T msec using TIMER0 with prescaler on
	clock, where, for a 16MHz clock:
	T = .0156 msec for no prescaler and count of 250 (preload counter
	with 5)
	T = 0.125 msec for prescaler set to 8 and count of 250 (preload
	counter with 5)
	T = 1 msec for prescaler set to 64 and count of 250 (preload counter
	with 5)
	T = 4 msec for prescaler set to 256 and count of 250 (preload
	counter with 5)
	T = 16 msec for prescaler set to 1,024 and count of 250 (preload
	counter with 5)
	Inputs: None
	Outputs: None
	*/
	TCCR1A = 0x00; // clears WGM00 and WGM01 (bits 0 and 1) to ensure Timer/Counter is in normal mode.
	TCNT1 = 5; // preload load TIMER0 (count must reach 255?5 = 250)
	switch ( choice ) { // choose prescaler
		case 1:
			TCCR1B = 1<<CS11; //TCCR0B = 0x02; // Start TIMER0, Normal mode, crystal clock, prescaler = 8
			break;
		case 2:
			TCCR1B = 1<<CS11 | 1<<CS10; //TCCR0B = 0x03; // Start TIMER0, Normal mode, crystal clock, prescaler = 64
			break;
		case 3:
			TCCR1B = 1<<CS12; //TCCR0B = 0x04; //Start TIMER0, Normal mode, crystal clock, prescaler = 256
			break;
		case 4:
			TCCR1B = 1<<CS12 | 1<<CS10; //TCCR0B = 0x05; // Start TIMER0, Normal mode, crystal clock, prescaler = 1024
			break;
		default:
			TCCR1B = 1<<CS10; //TCCR0B = 0x01; Start TIMER0, Normal mode, crystal clock, no prescaler
			break;
	}
	
	while (TCNT1 < 0xfa); // exits when count = 250
	TCCR1B = 0x00; // Stop TIMER1
	TIFR1 = 0x1<<TOV1; // Clear TOV1 (note that this is an odd bit in that it is cleared by writing a 1 to it)
	
	//while ((TIFR1 & (0x1<<TOV1)) == 0); // wait for TOV1 to roll over:
	//while (TCNT0 < 0xfa); // Alternate loop. Exits when count = 250 (requires preload of
	// TCNT0=0
	
	
	//TCCR1B = 0x00; // Stop TIMER1
	//TIFR1 = 0x1<<TOV1; // Clear TOV1 (note that this is an odd bit in that it
	//is cleared by writing a 1 to it)	

	/*
	int loop;
	while (loop); // wait for TOV0 to roll over and trigger TOV0 interrupt
	loop = 1; // prepare for next loop
	*/
} // end delay_T_msec_timer1()
