/*
 * Projetista: Elder Gabriel Alves Santos
 * Matricula: 118210555
 */ 

#define F_CPU 16000000UL
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include "nokia5110.h"

//Definições de macros para o trabalho com bits
#define tst_bit(y,bit) (y&(1<<bit))//retorna 0 ou 1 conforme leitura do bit

typedef enum
{
	Display1, Display2, Display3, Display4 ,size_enumSelet
} enum_selet;

// Variaveis Globais
uint8_t tempo_green = 6;
uint8_t tempo_yellow = 2;
uint8_t tempo_red = 4;
uint32_t tempo_ms = 0;
uint16_t num_carro = 0;
enum_selet Select_Display = 0;
uint8_t Modo = 0;
uint8_t Rendezira_Display = 0;
uint8_t flag_LUX = 0;
uint16_t LDR_LUX = 0;
uint8_t SOS = 0;

// Protótipos das funções
void Anima_Semaforo_LED (uint32_t tempo);
void Anima_LCD (uint8_t *tela_atualizada);
void LDR_LUX_(uint8_t *flag_lux);

ISR(TIMER0_COMPA_vect)
{
	tempo_ms++;  // Acressento 1 milisegundo
	
	if (tempo_ms % 50)
	Rendezira_Display = 1;
	
	if (tempo_ms % 300)
	flag_LUX = 1;
}

ISR(PCINT2_vect)
{
	/*Como a interrupção externa PCINT2 só possui um endereço na memória de
	programa, é necessário testar qual foi o pino responsável pela interrupção.*/
	
	if(!tst_bit(PIND,PD0)){ // Simula um sensor de veiculo de emergência
		
		static uint8_t controle_s = 0; // Variavel para controlar o momento de acionamento do botão na borda 

		if(controle_s){
			
			PORTD ^= 0b00000010; // Altera o nível do pino PD1 todo vez que o sesor for acionado 
			SOS = !SOS;          // Funciona como uma variável boleana que controla a msg do LCD e o estado do LED verde
			
		}
		controle_s = !controle_s;
	}
	
	if(!tst_bit(PIND,PD3)) // sensor de carros
	{
		static uint32_t carro_anterior = 0;
		num_carro = 60000/((tempo_ms - carro_anterior));
		carro_anterior = tempo_ms;
	}
	else if(!tst_bit(PIND,PD4)) // Botão (+)
	{
		switch (Select_Display){
			case Display1:
			Modo = !Modo;
			break;
			case Display2:
			if(tempo_green<9)
			tempo_green++;
			break;
			case Display3:
			if(tempo_red<9)
			tempo_red++;
			break;
			case Display4:
			if(tempo_yellow<9)
			tempo_yellow++;
			break;
		}
		_delay_ms(200);
	}
	else if(!tst_bit(PIND,PD5)) // Botão (-)
	{
		switch (Select_Display){
			case Display1:
			Modo = !Modo;
			break;
			case Display2:
			if(tempo_green>1)
			tempo_green--;
			break;
			case Display3:
			if(tempo_red>1)
			tempo_red--;
			break;
			case Display4:if(tempo_yellow>1)
			tempo_yellow--;
			break;
		}
		_delay_ms(200);
	}
	else if(!tst_bit(PIND,PD6)) // Botão de seleção (S)
	{
		static uint8_t flag_tela = 0;
		if (Modo)
		{
			if (flag_tela)
			{
				if (Select_Display < (size_enumSelet-1))
				Select_Display++;
				else
				Select_Display = 0;
			}
			flag_tela = !flag_tela;
		}
		_delay_ms(200);
	}
	
}

int main(void){
	
	// Config. GPIO
	DDRB = 0b11111111; // Habilita os pinos PB0 ao PB7 todos como saida
	DDRD = 0b10000010; // Habilita os pinos PD1 e PD7 como saida
	PORTD = 0b01111001; // Habilita o resistor de pull up dos pinos PD0, PD3, PD4, PD5 e PD6
	DDRC = 0b1000000; // Habilita o pino PC6 como saida
	
	//Configurações das interrupções
	PCICR  = 0b00000100;
	PCMSK2 = 0b01111001;
	
	// Configuração da TCT
	TCCR0A = 0b00000010; // habilita o CTC
	TCCR0B = 0b00000011; // Liga TC0 com prescaler = 64
	OCR0A  = 249;        // ajusta o comparador para TC0 contar até 249
	TIMSK0 = 0b00000010; // habilita a interrupção na igualdade de comparação com OCR0A. A interupção ocorre a cada 1ms = (65*(249+1))/16MHz

	//Configura ADC
	ADMUX = 0b01000000; // Vcc com referencia canal PC0
	ADCSRA= 0b11100111; // Habilita o AD, habilita interrupção, modo de conversão continua, prescaler = 128
	ADCSRB= 0b00000000; // Modo de conversão contínua
	DIDR0 = 0b00000000; // habilita pino PC0 e PC1 como entrada digitais


	sei(); //Habilita interrupção globais, ativando o bit I do SREG
	
	nokia_lcd_init();
	
	while (1){
		
		Anima_Semaforo_LED (tempo_ms); // Leva os valores do tempo de cada led para ser atualizado
		Anima_LCD(&Rendezira_Display);
		LDR_LUX_(&flag_LUX);
	}

}

void Anima_Semaforo_LED (uint32_t tempo){ // Atualiza os tempo dos leds
	
	const uint16_t estados[9] = {0b000001111, 0b000000111, 0b000000011, 0b000000001, 0b100000000, 0b011110000, 0b001110000, 0b000110000, 0b000010000};
	static int8_t i = 0;
	static uint32_t tempo_anterior_ms = 0;
	
	if (SOS == 1){
		
		PORTB = 0b00001111; // Deira o led verde para passagem do veiculo de emergência
		
	}
	else{
		
		PORTB = estados[i] & 0b011111111;
		
		if (estados[i] & 0b100000000)
		PORTD |= (1<<7);
		else
		PORTD &= 0b01111111;

		if (i<=3) // Led Verde rotina de tempo
		{
			if ((tempo - tempo_anterior_ms) >= (tempo_green*250))
			{

				i++;
				tempo_anterior_ms += (tempo_green*250);
			}
		}
		else
		{


			if(i<=4) // Led Amarelo rotina de tempo
			{
				if((tempo - tempo_anterior_ms) >= (tempo_yellow*1000))
				{
					i++;
					tempo_anterior_ms += (tempo_yellow*1000);
					
				}
			}
			
			else
			{
				if(i<=8) // Led Vermelho rotina de tempo
				{
					
					if((tempo - tempo_anterior_ms) >= (tempo_red*250))
					{
						i++;
						tempo_anterior_ms += (tempo_red*250);
					}
				}
				else
				{
					i=0;
					tempo_anterior_ms = tempo;
				}
			}
		}
	}
}

void Anima_LCD (uint8_t *tela_atualizada){ // Atualiza o display de acordo com o tempo de cada led
	
	char verde_strig[3];
	char verme_strig[3];
	char amare_strig[3];
	char num_carro_strig[5];
	char inten_lux[4];
	static char modo_string;

	if (SOS == 1){ // Caso SOS seja = 1, imprimir no Display a mensagem quem tem um veiculo de emergência no farol 
		nokia_lcd_clear();
		nokia_lcd_set_cursor(20,10);
		nokia_lcd_write_string("VEICULO",1);
		nokia_lcd_set_cursor(35,20);
		nokia_lcd_write_string("DE",1);
		nokia_lcd_set_cursor(13,30);
		nokia_lcd_write_string("EMERGENCIA",1);
		nokia_lcd_render();
		
	}
	else{ // Caso contrário, continua normalmente
		if (Modo){
			modo_string = 'M';
			
		}
		else{
			modo_string = 'A';
			tempo_green = 9 - (num_carro/60);
			tempo_red = (num_carro/60) + 1;
			
		}
		
		sprintf (&verde_strig, "%u", tempo_green);   // *****************************************************************
		sprintf (&verme_strig, "%u", tempo_red);     // *****************************************************************
		sprintf (&amare_strig, "%u", tempo_yellow);  // Converte as variaveis inteiras em char para imprimir no display
		sprintf (&num_carro_strig, "%u", num_carro); // *****************************************************************
		sprintf (&inten_lux, "%u", LDR_LUX);             // *****************************************************************

		if(*tela_atualizada)
		{
			switch(Select_Display){
				case Display1:
				nokia_lcd_clear();
				nokia_lcd_draw_Hline(0,47,50);
				
				nokia_lcd_set_cursor(0, 5);
				nokia_lcd_write_string("Modo", 1);
				nokia_lcd_set_cursor(30, 5);
				nokia_lcd_write_char(modo_string, 1);
				nokia_lcd_set_cursor(42, 5);
				nokia_lcd_write_string("<", 1);
				
				nokia_lcd_set_cursor(0, 15);
				nokia_lcd_write_string("T.Vd", 1);
				nokia_lcd_set_cursor(30,15);
				nokia_lcd_write_string(verde_strig, 1);
				nokia_lcd_set_cursor(35,15);
				nokia_lcd_write_string("s", 1);
				
				nokia_lcd_set_cursor(50,0);
				nokia_lcd_write_string(inten_lux, 2);
				nokia_lcd_set_cursor(50,15);
				nokia_lcd_write_string("lux", 1);
				
				nokia_lcd_set_cursor(50,25);
				nokia_lcd_write_string(num_carro_strig, 2);
				nokia_lcd_set_cursor(50,40);
				nokia_lcd_write_string("c/min", 1);
				
				nokia_lcd_set_cursor(0, 25);
				nokia_lcd_write_string("T.Vm", 1);
				nokia_lcd_set_cursor(30,25);
				nokia_lcd_write_string(verme_strig, 1);
				nokia_lcd_set_cursor(35,25);
				nokia_lcd_write_string("s", 1);
				
				nokia_lcd_set_cursor(0,35);
				nokia_lcd_write_string("T.Am", 1);
				nokia_lcd_set_cursor(30,35);
				nokia_lcd_write_string(amare_strig, 1);
				nokia_lcd_set_cursor(35,35);
				nokia_lcd_write_string("s", 1);
				
				nokia_lcd_render();
				break;
				
				case Display2:
				nokia_lcd_clear();
				nokia_lcd_draw_Hline(0,47,50);
				
				nokia_lcd_set_cursor(0, 5);
				nokia_lcd_write_string("Modo", 1);
				nokia_lcd_set_cursor(30, 5);
				nokia_lcd_write_char(modo_string, 1);
				
				nokia_lcd_set_cursor(0, 15);
				nokia_lcd_write_string("T.Vd", 1);
				nokia_lcd_set_cursor(30,15);
				nokia_lcd_write_string(verde_strig, 1);
				nokia_lcd_set_cursor(35,15);
				nokia_lcd_write_string("s", 1);
				nokia_lcd_set_cursor(42, 15);
				nokia_lcd_write_string("<", 1);
				
				nokia_lcd_set_cursor(50,0);
				nokia_lcd_write_string(inten_lux, 2);
				nokia_lcd_set_cursor(50,15);
				nokia_lcd_write_string("lux", 1);
				
				nokia_lcd_set_cursor(50,25);
				nokia_lcd_write_string(num_carro_strig, 2);
				nokia_lcd_set_cursor(50,40);
				nokia_lcd_write_string("c/min", 1);
				
				nokia_lcd_set_cursor(0, 25);
				nokia_lcd_write_string("T.Vm", 1);
				nokia_lcd_set_cursor(30,25);
				nokia_lcd_write_string(verme_strig, 1);
				nokia_lcd_set_cursor(35,25);
				nokia_lcd_write_string("s", 1);
				
				nokia_lcd_set_cursor(0,35);
				nokia_lcd_write_string("T.Am", 1);
				nokia_lcd_set_cursor(30,35);
				nokia_lcd_write_string(amare_strig, 1);
				nokia_lcd_set_cursor(35,35);
				nokia_lcd_write_string("s", 1);
				
				nokia_lcd_render();
				break;
				
				case Display3:
				nokia_lcd_clear();
				nokia_lcd_draw_Hline(0,47,50);
				
				nokia_lcd_set_cursor(0, 5);
				nokia_lcd_write_string("Modo", 1);
				nokia_lcd_set_cursor(30, 5);
				nokia_lcd_write_char(modo_string, 1);

				nokia_lcd_set_cursor(0, 15);
				nokia_lcd_write_string("T.Vd", 1);
				nokia_lcd_set_cursor(30,15);
				nokia_lcd_write_string(verde_strig, 1);
				nokia_lcd_set_cursor(35,15);
				nokia_lcd_write_string("s", 1);
				
				nokia_lcd_set_cursor(50,0);
				nokia_lcd_write_string(inten_lux, 2);
				nokia_lcd_set_cursor(50,15);
				nokia_lcd_write_string("lux", 1);
				
				nokia_lcd_set_cursor(50,25);
				nokia_lcd_write_string(num_carro_strig, 2);
				nokia_lcd_set_cursor(50,40);
				nokia_lcd_write_string("c/min", 1);
				
				nokia_lcd_set_cursor(0, 25);
				nokia_lcd_write_string("T.Vm", 1);
				nokia_lcd_set_cursor(30,25);
				nokia_lcd_write_string(verme_strig, 1);
				nokia_lcd_set_cursor(35,25);
				nokia_lcd_write_string("s", 1);
				nokia_lcd_set_cursor(42, 25);
				nokia_lcd_write_string("<", 1);
				
				nokia_lcd_set_cursor(0,35);
				nokia_lcd_write_string("T.Am", 1);
				nokia_lcd_set_cursor(30,35);
				nokia_lcd_write_string(amare_strig, 1);
				nokia_lcd_set_cursor(35,35);
				nokia_lcd_write_string("s", 1);
				
				nokia_lcd_render();
				break;
				
				case Display4:
				nokia_lcd_clear();
				nokia_lcd_draw_Hline(0,47,50); //(Y_inicial, X, Y_final)
				
				nokia_lcd_set_cursor(0, 5);
				nokia_lcd_write_string("Modo", 1);
				nokia_lcd_set_cursor(30, 5);
				nokia_lcd_write_char(modo_string, 1);
				
				nokia_lcd_set_cursor(0, 15);
				nokia_lcd_write_string("T.Vd", 1);
				nokia_lcd_set_cursor(30,15);
				nokia_lcd_write_string(verde_strig, 1);
				nokia_lcd_set_cursor(35,15);
				nokia_lcd_write_string("s", 1);
				
				nokia_lcd_set_cursor(50,0);
				nokia_lcd_write_string(inten_lux, 2);
				nokia_lcd_set_cursor(50,15);
				nokia_lcd_write_string("lux", 1);
				
				nokia_lcd_set_cursor(50,25);
				nokia_lcd_write_string(num_carro_strig, 2);
				nokia_lcd_set_cursor(50,40);
				nokia_lcd_write_string("c/min", 1);
				
				nokia_lcd_set_cursor(0, 25);
				nokia_lcd_write_string("T.Vm", 1);
				nokia_lcd_set_cursor(30,25);
				nokia_lcd_write_string(verme_strig, 1);
				nokia_lcd_set_cursor(35,25);
				nokia_lcd_write_string("s", 1);
				
				nokia_lcd_set_cursor(0,35);
				nokia_lcd_write_string("T.Am", 1);
				nokia_lcd_set_cursor(30,35);
				nokia_lcd_write_string(amare_strig, 1);
				nokia_lcd_set_cursor(35,35);
				nokia_lcd_write_string("s", 1);
				nokia_lcd_set_cursor(42, 35);
				nokia_lcd_write_string("<", 1);
				
				nokia_lcd_render();
				break;
			}
		}
	*tela_atualizada = !*tela_atualizada;}
}

void LDR_LUX_(uint8_t *flag_lux){
	static float Raw_ADC = 0;
	if(*flag_lux)
	{
		Raw_ADC = ADC;
		LDR_LUX = (1023000/Raw_ADC)-1000;
		
		if (LDR_LUX < 300)
		PORTC |= 0b1000000;
		else
		PORTC &= 0b0111111;
	}
	*flag_lux = 0;
}
