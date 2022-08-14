#define F_CPU 8000000
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "LCD.h"


uint16_t TempReading=0; //analog Reading variable
uint16_t ReadADC(uint8_t ADCchannel); //analog reading function
void servoangle(uint8_t pin,uint8_t pinC);
void ultralengthM();

char lcddata[20];
uint8_t PeopleCounter=0;
#define PeopleLimit 10
#define Tempupper 37
#define TempLower 35
volatile uint16_t TimerCalVal=0;// variable for collect echo data
uint16_t ultralength=0;

uint8_t hx711H=0; //Load Scale High Bits
uint16_t hx711L=0;//Load Scale Low Bits
float loadCellRead();
#define Load_data 2
#define Load_clk 3

int main(void)
{
	LCD_Init();
	LCD_String_xy(0,4,"Welcome");
	_delay_ms(500);
	LCD_Clear();
	MCUCR|=(1<<ISC11)|(1<<ISC10)|(1<<ISC01)|(1<<ISC00);
	GICR|=(1<<INT0)|(1<<INT1);
	
	ADCSRA |= ((1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0));   // prescaler 128
	ADMUX |= (1<<REFS0)|(1<<REFS1);					//internal 2.56 v ref
	ADCSRA |= (1<<ADEN);                            // Turn on ADC
	PORTA|=(1<<1);
	
	
	TCCR1B|=(1<<CS12)|(1<<CS10)|(1<<WGM12);//Start timer  prescaler =1024
	TCNT1=0;
	OCR1A=31250;
	
	/*Ultrasonic Timer Part*/
	DDRD|=(1<<4); //D4 as output
	DDRD|=(1<<6); //D6 as output
	DDRD|=(1<<7); //D7 as output
	TCCR0|=(1<<WGM01);//Enable Compare match mode
	TCCR0|=(1<<CS11);//Start timer  prescaler =8
	TCNT0=0;
	OCR0=10;
	/*register value= time*(clock speed/prescale)
	register value=0.000001*(8000000/1)
	register value=10*/
	TIMSK|=(1<<OCIE0);//enable timer Compare inturrept
	
	DDRC|=(1<<Load_clk); //Load cell clock pin
	PORTC&=~(1<<Load_clk);//Clock pin low
	
	DDRA|=(1<<6); //D6 as output for Green LED
	DDRA|=(1<<7); //D7 as output for Red LED
	
	sei();
	while (1)
	{
		if (PINC&(1<<7))
		{
			
			servoangle(1,7);
			_delay_ms(2000);
			servoangle(0,7);
		}
		
		
		if ((!(PINA&(1<<1)))&&(PeopleCounter<PeopleLimit))
		{	
			TempReading=(ReadADC(0)*0.25024438); //calibrated number
			
			if ((TempReading<=Tempupper)&&(TempReading>TempLower))
			{
				LCD_Clear();
				LCD_String_xy(0,0,"Temperature OK");
				PORTA|=(1<<6); //Green LED ON
				_delay_ms(1500);
				PORTA&=~(1<<6);
				LCD_Clear();
				
				LCD_String_xy(0,0,"Make feet");
				
				for(uint8_t i=0;i<100;i++){
				float weight=loadCellRead();
				sprintf(lcddata,"%0.2fKg",weight);
				LCD_String_xy(1,0,lcddata);			
				if (weight>15)
				{
					LCD_Clear();
					LCD_String_xy(0,0,"Please Hand");
					
					for(uint8_t j=0;j<100;j++){
						ultralengthM();
						sprintf(lcddata,"%03ucm",ultralength);
						LCD_String_xy(1,0,lcddata);
						if (ultralength<20)
						{
							_delay_ms(500);
							servoangle(1,6);
							_delay_ms(100);
							servoangle(0,6);
							_delay_ms(1000);
							servoangle(1,7);
							_delay_ms(2000);
							servoangle(0,7);
							
							
							break;
						}
						_delay_ms(100);
					}
					
					
					break;
				}
					_delay_ms(100);
					
				}
				
				
				
			} 
			else
			{	LCD_Clear();
				LCD_String_xy(0,0,"Try Again");
				PORTA|=(1<<7); //Red LED ON
				_delay_ms(2000);
				PORTA&=~(1<<7);
			}
		}
		
		
				
		
		
		sprintf(lcddata,"Count- %02u %03uC  ",PeopleCounter,TempReading);
		LCD_String_xy(0,0,lcddata);
		
		_delay_ms(10);
		if (PeopleCounter>5)
		{LCD_String_xy(1,0,"People limited  ");
		}
		else
		{LCD_String_xy(1,0,"               ");
		}
		
	}
}
ISR(INT0_vect){
	
	
	
	if (TIMSK&(1<<OCIE1A))
	{TIMSK&=~(1<<OCIE1A);
		if (TCNT1>0)
		{PeopleCounter++;
			TCNT1=0;
		}
	}
	else
	{TIMSK|=(1<<OCIE1A);
	}
	
	
}
ISR(INT1_vect){
	if (TIMSK&(1<<OCIE1A))
	{TIMSK&=~(1<<OCIE1A);
		if (TCNT1>0)
		{
			if (PeopleCounter>0)
			{PeopleCounter--;
			}
		}
	}
	else
	{TIMSK|=(1<<OCIE1A);
	}
}
ISR(TIMER1_COMPA_vect){//ultrasonic
	
	
	TIMSK&=~(1<<OCIE1A);//enable timer Compare inturrept
	TCNT1=0;
}

uint16_t ReadADC(uint8_t ADCchannel)
{
	//select ADC channel with safety mask
	ADMUX = (ADMUX & 0xF0) | (ADCchannel & 0x0F);
	//single conversion mode
	ADCSRA |= (1<<ADSC);
	// wait until ADC conversion is complete
	while( ADCSRA & (1<<ADSC) );
	return ADCW;
}

void servoangle(uint8_t pin,uint8_t pinC)
{

	if (pin)
	{
		for(uint8_t j=0;j<100;j++){
			PORTD|=(1<<pinC);
			for(uint8_t i=0;i<15;i++){
				_delay_us(100);
			}
			PORTD&=~(1<<pinC);
			
			
			
			for(uint8_t i=0;i<15;i++){
				_delay_us(100);
			}
		}
	}
	else
	{for(uint8_t j=0;j<100;j++){
		
		PORTD|=(1<<pinC);
		for(uint8_t i=0;i<5;i++){
			_delay_us(100);
		}
		PORTD&=~(1<<pinC);
		
			
		for(uint8_t i=0;i<5;i++){
			_delay_us(100);
		}
	}
}




}



ISR(TIMER0_COMP_vect){//ultrasonic
	TimerCalVal++;
	TCNT0=0;
	
}


float loadCellRead(){
	hx711H=0;hx711L=0;  //clear variables
	for(uint8_t i=0;i<8;i++){  // Load cell data high 8 bits
		PORTC|=(1<<Load_clk); //Clock pin high
		_delay_us(10);
		if ((PINC&(1<<Load_data))>>Load_data)  //read data pin
		{hx711H|=(1<<(7-i));//set hx 711 varible
		}
		else
		{hx711H&=~(1<<(7-i));
		}
		PORTC&=~(1<<Load_clk); //Clock pin low
		_delay_us(5);
	}
	
	
	for(uint8_t i=0;i<16;i++){ // Load cell data low 16 bits
		PORTC|=(1<<Load_clk); //Clock pin high
		_delay_us(10);
		if ((PINC&(1<<Load_data))>>Load_data) //read data pin
		{hx711L|=(1<<(15-i));
		}
		else
		{hx711L&=~(1<<(15-i));
		}
		PORTC&=~(1<<Load_clk); //Clock pin low
		_delay_us(5);
	}
	
	hx711L=hx711L>>1; //shift bits
	
	if (hx711H&1)  //bit setup
	{hx711L|=(1<<15);
	}
	else
	{hx711L&=~(1<<15);
	}
	hx711H=hx711H>>1;
	
	return (hx711H*(65536/18029.6))+hx711L/18029.6; //load cell calibration
}

void ultralengthM(){
	
	PORTD&=~(1<<4);//TRIG pin low
	_delay_us(50);//wait 50 micro sec
	PORTD|=(1<<4);//TRIG pin high
	_delay_us(50);//wait 50 micro sec
	PORTD&=~(1<<4);////TRIG pin low
	while(!(PIND&(1<<5)))//wait for pulse
	TimerCalVal=0;//rest timer
	while((PIND&(1<<5)))////wait for pulse down
	ultralength=TimerCalVal/4.1282;//copy timer value
}


