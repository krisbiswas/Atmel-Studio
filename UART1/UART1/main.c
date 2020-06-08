/*
 * UART1.c
 *
 * Created: 14-01-2020 10:08:38 PM
 * Author : Krishnandu Biswas
 */ 

#ifndef F_CPU
#define F_CPU 16000000
#endif

#include <avr/io.h>
#include <util/delay.h>

#define BAUD 103

char* str = "a b c d";

void UART_init(){
	UBRR0L = (BAUD);		//Baud to 9600
	UBRR0H = (BAUD>>8);
	UCSR0B = (0x18);		//Tx and Rx enable, Tx and Rx and UDR empty Interrupt disabled
	UCSR0C = (0x06);		//8 bit data, 1 stop bit, parity disabled
}

void UART_TxChar(char ch){
	while(!(UCSR0A &(1<<UDRE0)));
	UDR0 = ch;
}

void send_string(){
	for(uint8_t i =0;*(str+i) != '\0';i++){
		UART_TxChar(*(str+i));
		_delay_ms(500);
	}
}

int main(void)
{
	UART_init();
	DDRG |= (1<<PG2);
	PORTG |= (1<<PG0);
	PORTG &= ~(1<<PG2);
	
	//char* received;
	
    while (1) 
    {
		send_string();
		_delay_ms(2000);
		/*if(PG0 == 1){
			PORTG &= ~(1<<PG2);
		}else{
			PORTG |= (1<<PG2);
		}*/
    }
}

