/*
 * Maze_solver.cpp
 *
 * Created: 02-04-2020 10:55:08 PM
 * Author : Krishnandu Biswas
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
//#define LEFT_MOTOR1 PINB1  //PB1
//#define LEFT_MOTOR2 PINB2  //PB2
//#define RIGHT_MOTOR1 PINB3  //PB3
//#define RIGHT_MOTOR2 PINB4  //PB4
bool available_left,available_right,available_straightl,available_straightr;

void initialize_ADC(){
	ADMUX = 0x60;	//ADC0 enable and Vcc as Vref
	ADCSRA = 0x86;	//ADCEn and 64 Prescaler
	DIDR0 = 0x01;	//Disable Digital input data buffer for PC0 connected to UL_sensor
}
void initialize_IO(){
	DDRD &= ~((1<<PIND4)|(1<<PIND5)|(1<<PIND6)|(1<<PIND7));			//IR inputs
	PORTD |= ((1<<PIND4)|(1<<PIND5)|(1<<PIND6)|(1<<PIND7));			//Pull-up input ports
	DDRB |= (1<<PINB4)|(1<<PINB3)|(1<<PINB2)|(1<<PINB1);	//Motors Output
	//Port C for Analog input -> activate ADCs
	//ADC0 to Ultrasonic Sensor
	initialize_ADC();
}
uint8_t read_UL(){
	//set ADSC to start ADC data conversion
	ADCSRA |= (1 << ADSC);
	//wait while ADSC is cleared i.e. data conversion is done
	while(ADCSRA & (1 << ADSC));
	return ADCH0;
}
int read_IR(){
	return PIND;
}
void check_availablity(){
	available_left = ((read_IR() && (1<<PIND4)) == 1);
	available_straightl = ((read_IR() && (1<<PIND5)) == 1);
	available_straightr = ((read_IR() && (1<<PIND6)) == 1);
	available_right = ((read_IR() && (1<<PIND7)) == 1);
}
void go(char dir){
	switch(dir){
		case 'L':
			PORTB &= ~((1<<PINB1)|(1<<PINB3));
			PORTB = ((1<<PINB2)|(1<<PINB4));
			_delay_ms(400);
			while ((read_IR()&(1<<PIND6)) == 0){//Left indicator
			}
			break;
		case 'R':
			PORTB &= ~((1<<PINB2)|(1<<PINB4));
			PORTB = ((1<<PINB1)|(1<<PINB3));
			_delay_ms(400);
			while ((read_IR()&(1<<PIND6)) == 0){// Right Indicator
			}
			break;
		case 'F':
			PORTB &= ~((1<<PINB1)|(1<<PINB4));
			PORTB = ((1<<PINB2)|(1<<PINB3));
			_delay_ms(400);
			while (!(available_left && available_right) && (available_straightl && available_straightr)) {
				check_availablity();//No Indicator
			}
			break;
		case 'B'://	Rotate Left 2x times
			PORTB &= ~((1<<PINB1)|(1<<PINB3));
			PORTB = ((1<<PINB2)|(1<<PINB4));
			_delay_ms(400);//give some more time
			while ((read_IR()&(1<<PIND4|1<<PIND5)) == 0){//All Indicators ON
			}
			break;
		default:
			PORTB &= ~((1<<PINB1)|(1<<PINB2)|(1<<PINB3)|(1<<PINB4));
			while(read_UL()<=100){
				//Waiting indicator --> Blink
			}
			break;
	}
}

void find_path(bool* found_path,char* path,uint8_t* index){
	if(read_UL() > 100){
		if(available_left && available_right && available_straightl && available_straightr){
			//step ahead and verify the end
			PORTB &= ~((1<<PINB1)|(1<<PINB4));
			PORTB = ((1<<PINB2)|(1<<PINB3));
			_delay_ms(5);
			check_availablity();
			*found_path = (available_left && available_right && available_straightl && available_straightr);
		}else{
			uint8_t i = *index;
			if(available_left){
				*(path+i) = 'L';*index=i+1;
				go('L');
			}else if(available_straightl || available_straightr){
			//	Keep on track
				if(available_straightl && !available_straightr){
					PORTB &= ~((1<<PINB1)|(1<<PINB3));
					PORTB = ((1<<PINB2)|(1<<PINB4));
					_delay_ms(10);
				}
				if(!available_straightl && available_straightr){
					PORTB &= ~((1<<PINB2)|(1<<PINB4));
					PORTB = ((1<<PINB1)|(1<<PINB3));
					_delay_ms(10);
				}
				if(available_right){
					*(path+i) = 'F';*index=i+1;;
				}
				go('F');
			}else if(available_right){
				*(path+i) = 'R';*index=i+1;;
				go('R');
			}else{
				*(path+i) = 'B';*index=i+1;;
				go('B');
			}
		}
	}else{
		go('S');
	}
}

bool check_reversible(char* path_index){
	string sub_path = "";
	sub_path.append(*(path_index));
	sub_path.append(*(path_index+1));
	sub_path.append(*(path_index+2));
	//bool status;
	switch(sub_path){
		case "LBR":*(path_index) = 'B';return true;
		case "LBL":*(path_index) = 'S';return true;
		case "LBS":*(path_index) = 'R';return true;
		case "SBL":*(path_index) = 'R';return true;
		case "SBS":*(path_index) = 'B';return true;
		case "RBL":*(path_index) = 'B';return true;
		default:return true;
	}
	//return status;
}

int optimize_path(int length,char* path_index){
	//Stop before optimization and wait for restart button -> PD0
	int pathLength = length;
	for (int i =0;i<length;i++){
		if(*(path_index+1) == 'B'){
			if(check_reversible(path_index)){
				//shifting if path reduced
				for(int j=i+1;j<length-2;j++){
					*(path_index+j) = *(path_index+j+2);
				}
				pathLength-=2;
			}
			
		}
	}
	return pathLength;
}

bool follow_path(char* path,uint8_t* index){
	check_availablity();
	if(read_UL() > 100){
		uint8_t i = *index;
		switch(*(path+i)){
			case 'L':
				if (!available_left){
					return false;
				}
				go('L');*index = i+1;
				break;
			case 'F':
				if (!(available_straightl && available_straightr)){
					return false;
				}
				go('F');*index = i+1;
				break;
			case 'R':
				if (!available_right){
					return false;
				}
				go('R');*index = i+1;
				break;
			case 'B':
				if (available_left || available_straightl || available_straightr || available_right){
					return false;
				}
				go('B');*index = i+1;
				break;
		}
	}
	else{
		go('S');
	}
	return true;
}

int main(void)
{
	initialize_IO();
	char path[100];
	int pathlength=0;
	uint8_t index = 0;
    bool found_path = false;
	
	while (1) 
    {
//		LEFT HAND VISUAL
		while (!found_path)
		{
			check_availablity();
			find_path(&found_path,&path[0],&index);		//make index as char pointer and pass the current junction index only
														//(&path[0],&index) => path_index
		}
		pathlength = index;
		pathlength = optimize_path(pathlength,&path[0]);
		index = 0;
		while (found_path)
		{
			check_availablity();
			found_path = index < pathlength?follow_path(&path[0],&index):false;
		}
		
/*		WITH TRACE
		if(!(available_left && available_right) && available_straightl){
			go(FORWARD);
		}
		else if(!take_uturn){
			if(available_left){
				path.push(LEFT);
				go(LEFT);
				}else if(available_straightl){
				path.push(FORWARD);
				go(FORWARD);
				}else if(available_right){
				path.push(RIGHT);
				go(RIGHT);
				}else{
				take_uturn = true;
				go(UTURN);
			}
		}
		else{
			if(available_left){
				if(path.top()==RIGHT){
					path.pop();
					go(LEFT);
				}else if(path.top()==LEFT){
					take_uturn = false;
					go(LEFT);
				//}
			}else if(available_straightl){
				go(FORWARD);
				take_uturn = false;
			}else if(available_right){
				if(path.top()==LEFT){
					path.pop();
					go(RIGHT);
				}else if(path.top()==LEFT){
					take_uturn = false;
					go(RIGHT);
				}
			}
			if(!(available_left && available_right) && available_straightl){
				go(FORWARD);
			}
		}*/
    }
}

