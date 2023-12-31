/*!
    \file    main.c
    \brief   transmit/receive data using DMA interrupt

    \version 2017-02-10, V1.0.0, firmware for GD32F30x
    \version 2018-10-10, V1.1.0, firmware for GD32F30x
    \version 2018-12-25, V2.0.0, firmware for GD32F30x
    \version 2020-09-30, V2.1.0, firmware for GD32F30x
*/

/*
    Copyright (c) 2020, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "gd32f30x.h"
#include <stdio.h>
#include <string.h>
#include "gd32f307c_eval.h"
#include "driver.h"

__IO FlagStatus g_transfer_complete = RESET;
uint8_t rx_dma_buffer[10];
const uint8_t txbuffer[] = "\nUSART0 DMA transmit\n";
const uint8_t txbuffer1[] = "\nUSART1 DMA transmit\n";
uint32_t system_freq;

uint8_t debug_control_flag = 0;
uint16_t send_time = 0;
uint8_t debug_buf[30];

uint8_t debug_control_flag1 = 0;
uint16_t send_time1 = 0;
uint8_t debug_buf1[30];

void nvic_config(void);

static void updateflag(void)
{
}

static void updateflag1(void)
{
}

static void restart_receive(uint16_t data_len)
{
	send_time ++;
	sprintf((char *)debug_buf, "uart0 recv %d times\n\r", send_time);
    UartSendDMA(&Uart0, debug_buf, strlen((const char *)debug_buf));

	UartReceiveToIdleDMA(&Uart0, rx_dma_buffer, sizeof(rx_dma_buffer));
}

static void restart_receive1(uint16_t data_len)
{
	send_time1 ++;
	sprintf((char *)debug_buf1, "uart1 recv %d times\n\r", send_time1);
    UartSendDMA(&Uart1, debug_buf1, strlen((const char *)debug_buf1));
	UartReceiveToIdleDMA(&Uart1, rx_dma_buffer, sizeof(rx_dma_buffer));
}

/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    UartInitStruct uart_init;
	TimerInitStruct timer_init;
	
	// LED PB12
	rcu_periph_clock_enable(RCU_GPIOB);
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ,GPIO_PIN_12);
    gpio_bit_reset(GPIOB, GPIO_PIN_12);
	
	// debug PA2
	rcu_periph_clock_enable(RCU_GPIOA);
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ,GPIO_PIN_2);
	
    uart_init.baudrate = 115200;
    uart_init.parity = ParityNone;
    uart_init.stop_bit = StopBit_1Bit;
    uart_init.word_length = WordLen_8Bit;

    UartInit(&Uart0, &uart_init);
	UartSendCallbackRegister(&Uart0, &updateflag);
    UartRecvCallbackRegister(&Uart0, &restart_receive);

    UartSendDMA(&Uart0, txbuffer, sizeof(txbuffer));

//    UartInit(&Uart1, &uart_init);
//	UartSendCallbackRegister(&Uart1, &updateflag1);
//    UartRecvCallbackRegister(&Uart1, &restart_receive1);

//    UartSendDMA(&Uart1, txbuffer1, sizeof(txbuffer1));
	
	UartReceiveToIdleDMA(&Uart0, rx_dma_buffer, sizeof(rx_dma_buffer));
//	UartReceiveToIdleDMA(&Uart1, rx_dma_buffer, sizeof(rx_dma_buffer));
	
	timer_init.update_time_us = 1000;
	TimerInit(&Timer0, &timer_init);
	
//	GetSystemClock(&system_freq);
//	while(SetSystemClock(96000000));
//	GetSystemClock(&system_freq);
	
    while(1){
		if(debug_control_flag == 1)
		{
			debug_control_flag = 0;
		}
		if(debug_control_flag1 == 1)
		{
			debug_control_flag1 = 0;
		}
    }
	
}

/*!
    \brief      configure DMA interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
void nvic_config(void)
{
    nvic_irq_enable(DMA0_Channel3_IRQn, 0, 0);
    nvic_irq_enable(DMA0_Channel4_IRQn, 0, 1);
}

/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(EVAL_COM0, (uint8_t)ch);
    while(RESET == usart_flag_get(EVAL_COM0, USART_FLAG_TBE));
    return ch;
}
