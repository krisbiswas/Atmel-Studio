/*
 * SPI_Flash1.c
 *
 * Created: 15-01-2020 08:26:35 PM
 * Author : Krishnandu Biswas
 */ 

#define F_CPU 16000000L
#include <avr/io.h>
#define UBRR_value 104

void MCU_UART_init();
void UART_TxChar(char);
void MCU_SPI_init();
void SPI_send_byte(unsigned char);
unsigned char SPI_get_byte();
void wait_until_busy();
void set_write_enable();
void SST_ChipErase();
void write_data();
void read_data();
void verify();
void print_msg(uint8_t);

const char* msg = "SPI write and read test"; //message to be written in Flash Memory
char* recevied_msg;							 //to store message received from Flash Memory

int main(void)
{
	MCU_SPI_init();
	MCU_UART_init();
	SST_ChipErase();
	while (1)
	{
		write_data();
		//read_data();
		//verify();
	}
}

void MCU_UART_init(){
	UBRR2H=(UBRR_value>>8); //Setting Baud Rate to 9600
	UBRR2L=UBRR_value;
	UCSR2C&=~(1<<UMSEL20|1<<UMSEL21); //Aync Mode
	UCSR2C&=~(1<<USBS2); //1 Stop Bit
	UCSR2C&=~(1<<UPM20); //Even Parity
	UCSR2C|=(1<<UPM21);
	UCSR2C|=(1<<UCSZ20|1<<UCSZ21); //8 Bit Char
	UCSR2B|=(1<<TXEN2); //Tx Enable
}

void UART_TxChar(char ch)
{
	while (!(UCSR2A & (1<<UDRE2))){}; // Wait for empty transmit buffer
	UDR2 = ch ;
}

void UART_SendString(char *str)
{
	unsigned char j=0;
	while (str[j]!=0) // Send string till null
	{
		UART_TxChar(str[j]);
		j++;
	}
}

void MCU_SPI_init(){
	SPCR |= (1<<MSTR | 1<<SPE); //Setting MSTR Bit and SPI_EN Bit
	SPCR &= ~(1<<SPIF); //Disable SPI Interrupt
	SPSR |= (1<<SPI2X); //SPR2X = 1 --> set freq will be fosc/2 = 8MHz
	DDRB |= (1<<PB1|1<<PB2); //MCU_SCK and MCU_MOSI as output
	DDRB &= ~(1<<PB3); //MCU_MISO as input
	PORTB |= (1<<PB3); //PULL UP for input
	DDRJ |= (1<<PJ3|1<<PJ6); //MCU_CE and MCU_HOLD as output
	PORTJ |= (1<<PJ6); //Disable hold state (active low signal)
	DDRG |= (1<<PG0); //MCU_WP as output
	PORTG |= (1<<PG0); //Disable Write Protection (MCU_WP to high)
}

void send_byte(unsigned char data){
	SPDR = data; //Load data into the SPDR register for transmission
	while(!(SPSR & (1<<SPIF))); //Wait until SPIF Flag is set to 1 ((SPSR & (1<<SPIF)) != 1)
}

unsigned char get_byte(){
	while(!(SPSR & (1<<SPIF))); //Waiting while SPDR is set with the received data byte and SPIF is set
	return (SPDR);
}

void wait_until_busy(){ //check BUSY bit of status register
	PORTJ &= ~(1<<PJ3); //set CE to = 0 --> to enable chip
	char status;
	do{
		send_byte(0x05);
		status = get_byte();
	}while((status &(0x01)) == 0x01); //wait till BUSY == 0
	PORTJ |= (1<<PJ3); //set CE to = 1 --> to disable chip
}

void set_write_enable(){
	PORTJ &= ~(1<<PJ3);
	char status;
	do{
		send_byte(0x06); //Set Write Enable
		//wait_until_busy();
		send_byte(0x05); //read status register
		status = get_byte();
	}while ((status &(0x02)) != 0x02);
	PORTJ |= (1<<PJ3);
}

void SST_ChipErase(){
	PORTJ &= ~(1<<PJ3); //set CE to = 0 --> to enable chip
	set_write_enable();
	send_byte(0x60); //Chip Erase Command also disables WEL
	wait_until_busy();
	PORTJ |= (1<<PJ3); //set CE to = 1 --> to disable chip
}

void AAI_write_start(){
	PORTJ &= ~(1<<PJ3); //set CE to = 0 --> to enable chip
	set_write_enable();
	send_byte(0xAD); //AAI write command
	send_byte(0x00); //initial write address --> 000000h
	send_byte(0x00);
	send_byte(0x00);
	send_byte(*msg);
	send_byte(*(msg+1));
	wait_until_busy();
	PORTJ |= (1<<PJ3);
}

void AAI_remaining(){
	uint8_t i=2;
	while (*(msg+i) != '\0')
	{
		PORTJ &= ~(1<<PJ3); //set CE to = 0 --> to enable chip
		send_byte(0xAD);
		send_byte(*(msg+i));i++;
		send_byte(*(msg+i));i++;
		wait_until_busy();
		PORTJ |= (1<<PJ3); //set CE to = 1 --> to disable chip
	}

}

void write_data(){ //write value from 0 to 255 @ address 000000h to 0000ffh

	//set_write_enable();

	AAI_write_start();
	AAI_remaining();

	PORTJ &= ~(1<<PJ3); //set CE to = 0 --> to enable chip
	char status;
	do{
		send_byte(0x04); //Disable WEL
		//wait_until_busy();
		send_byte(0x05); //read status register
		status = get_byte();
	}while ((status &(0x02)) == 0x02);
	PORTJ |= (1<<PJ3); //set CE to = 1 --> to disable chip
}

void read_data(){
	PORTJ &= ~(1<<PJ3); //set CE to = 0 --> to enable chip
	uint8_t i=0;
	send_byte(0x03); //send read command
	send_byte(0x00); //initial read address --> 000000h
	send_byte(0x00);
	send_byte(0x00);

	while (*(recevied_msg+i-1) != '\0') //read data until null char is received
	{
		*(recevied_msg+i) = get_byte(); //storing data in array
		i++;
	}
	PORTJ |= (1<<PJ3); //set CE to = 1 --> to disable chip
}

void verify() {
	uint8_t stat = 1;
	for(uint8_t i=0;*(msg+i)!='\0';i++){
		if(*(recevied_msg+i) != *(msg+i)){
			stat = 0;break;
		}
	}
	print_msg(stat);
}

void print_msg(uint8_t status){
	if(status == 1){
		UART_SendString("Message Reception Successful\n");
		}else{
		UART_SendString("Message Reception Failed\n");
	}
}