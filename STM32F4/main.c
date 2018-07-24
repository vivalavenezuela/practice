
#include <stdint.h>
#include <stdio.h>
#include <systems.h>
#include <math.h>
#include <stm32f4xx_exti.h>
#include <DFT32.h>
#include <embeddednf.h>
#include <embeddedout.h>
#include "mp45dt02.h"

volatile int wasclicked = 0; 

RCC_ClocksTypeDef RCC_Clocks;



#define CIRCBUFSIZE 256
volatile int last_samp_pos;
int16_t sampbuff[CIRCBUFSIZE];
volatile int samples;

uint16_t delay_count = 0;


void SysTick_Handler()
{
	if (delay_count > 0)
		delay_count--;
}

void delay_ms (uint16_t delay_temp)
{
	delay_count = delay_temp;
	while (delay_count)
	{}
}


void GotSample( int samp )
{
	sampbuff[last_samp_pos] = samp;
	last_samp_pos = ((last_samp_pos+1)%CIRCBUFSIZE);
	samples++;
}

void NewFrame()
{
	int i;
	HandleFrameInfo();
	UpdateAllSameLEDs();

	SendSPI2812( ledOut, NUM_LIN_LEDS );
}


void Configure_PA0(void) {

    GPIO_InitTypeDef GPIO_InitStruct;
    EXTI_InitTypeDef EXTI_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;
    
   
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    
    
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);
    
    EXTI_InitStruct.EXTI_Line = EXTI_Line0;
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_Init(&EXTI_InitStruct);
 
    NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;   
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00; 
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;   
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;  
    NVIC_Init(&NVIC_InitStruct);   
}


void EXTI0_IRQHandler(void)
{
	static int rootoffset;

    if (EXTI_GetITStatus(EXTI_Line0) != RESET)
	{
		if( wasclicked == 0 )
			wasclicked = 10;
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

int main(void)
{  
	uint32_t i = 0;

	RCC_GetClocksFreq( &RCC_Clocks );

	ConfigureLED(); 

	LED_OFF;

	SysTick_Config( RCC_Clocks.HCLK_Frequency / 100);

	float fv = RCC_Clocks.HCLK_Frequency / 1000000.0f;


	InitColorChord();
	Configure_PA0();
	InitMP45DT02();
	InitSPI2812();
	Hello();

	int this_samp = 0;
	int wf = 0;

	while(1)
	{

		if( this_samp != last_samp_pos )
		{
			LED_OFF; //Use led on the board to show us how much CPU we're using. (You can also probe PB15)

			PushSample32( sampbuff[this_samp]/2 ); //Can't put in full volume.

			this_samp = (this_samp+1)%CIRCBUFSIZE;

			wf++;
			if( wf == 128 )
			{
				NewFrame();
				wf = 0; 
			}
			LED_ON;
		}
		LED_ON; //Take up a little more time to make sure we don't miss this.
	}
}

void TimingDelay_Decrement()
{
	static int rootoffset;

	//A way of making sure we debounce the button.
	if( wasclicked )
	{
		if( wasclicked == 10 )
		{
			if( rootoffset++ >= 12 ) rootoffset = 0;
			RootNoteOffset = (rootoffset * NOTERANGE) / 12;
		}

		wasclicked--;
	}

}


void Hello()
{
	for (int z=0; z<4; z++)
	{
		GPIO_SetBits(GPIOD, GPIO_Pin_14)
		delay_ms(150);
		GPIO_ResetBits(GPIOD, GPIO_Pin_14)
		GPIO_SetBits(GPIOD, GPIO_Pin_15)
		delay_ms(150);
		GPIO_ResetBits(GPIOD, GPIO_Pin_15)
		GPIO_SetBits(GPIOD, GPIO_Pin_12)
		delay_ms(150);
		GPIO_ResetBits(GPIOD, GPIO_Pin_12)
		GPIO_SetBits(GPIOD, GPIO_Pin_13)
		delay_ms(150);
		GPIO_ResetBits(GPIOD, GPIO_Pin_13)
	}
}

