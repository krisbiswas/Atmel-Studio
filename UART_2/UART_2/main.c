
#ifndef F_CPU
#define F_CPU 16000000UL
#define val 9
#endif
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

const char* msg = "loop";
char* rData;

void usart_init()
{//
	//********* UART3 **********//
	UBRR3H=(val>>8); //BAUD RATE
	UBRR3L=val;
	UCSR3B|=(1<<RXEN3|1<<TXEN3); //TX AND RX ENABLE
	//********* UART2 **********//
	UBRR2H=(val>>8); //BAUD RATE
	UBRR2L=val;
	UCSR2B|=(1<<RXEN2|1<<TXEN2); //TX AND RX ENABLE
	//********* UART1 **********//
	UBRR1H=(val>>8); //BAUD RATE
	UBRR1L=val;
	UCSR1B|=(1<<RXEN1|1<<TXEN1); //TX AND RX ENABLE
	//********* UART0 **********//
	UBRR0H=(val>>8); //BAUD RATE
	UBRR0L=val;
	UCSR0B|=(1<<RXEN0|1<<TXEN0); //TX AND RX ENABLE
}

void UART_TxChar3(char chh)
{
	while (!(UCSR3A & (1<<UDRE3)));			// Wait for empty transmit buffer*/
	UDR3 = chh;
}
void UART_TxChar2(char ch) //usart2 rs232
{	
	while (! (UCSR2A & (1<<UDRE2)));		// Wait for empty transmit buffer*/
	UDR2 = ch ;
}
void UART_TxChar1(char ch) //usart1 rs4221
{
	while (! (UCSR1A & (1<<UDRE1)));		// Wait for empty transmit buffer*/
	UDR1 = ch ;
} 
void UART_TxChar0(char chh){
	while (!(UCSR0A & (1<<UDRE0)));			// Wait for empty transmit buffer*/
	UDR0 = chh;
} 

unsigned char UART_RxChar3()
{
	while ((UCSR3A & (1 << RXC3)) == 0);	// Wait till data is received */
	return(UDR3);
}
unsigned char UART_RxChar2() //usart2 rs2321
{
	while ((UCSR2A & (1 << RXC2)) == 0);	// Wait till data is received */
	return(UDR2);
}
unsigned char UART_RxChar1() //usart1 rs4221
{
	while ((UCSR1A & (1 << RXC1)));			// Wait till data is received */
	return(UDR1); /* Return the byte*/
}
unsigned char UART_RxChar0()
{
	while ((UCSR0A & (1 << RXC0)) == 0);	// Wait till data is received */
	return(UDR0); /* Return the byte*/
}

void UART_SendString(const char *str)
{
	uint8_t j=0;
	char data;
	while (str[j]!=0) /* Send string till null */
	{
		UART_TxChar3(str[j]);
		data = UART_RxChar3();
		UART_TxChar2(data);
		data = UART_RxChar2();
		UART_TxChar1(data);
		data = UART_RxChar1();
		UART_TxChar0(data);
		*(rData+j) = UART_RxChar0();
		j++;
	}
} 

void loopChar(char c){
	UART_TxChar3(c);
	UART_TxChar2(UART_RxChar3());
	UART_TxChar1(UART_RxChar2());
	UART_TxChar0(UART_RxChar1());
}

int main(void)
{ 
	DDRF |= (1<<PF0);
	PORTF &= ~(1<<PF0);
	usart_init();
	//char c='K';
	
	while(1)
	{
		loopChar('\n');
		_delay_ms(100);
		loopChar('a');
		/*UART_SendString(msg);
		if(strcmp(rData,msg)==0){
			PORTF |= (1<<PF0);
			_delay_ms(1000);
			PORTF &= ~(1<<PF0);
		}*/
		_delay_ms(1000);
	}
}