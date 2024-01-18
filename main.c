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
#include "driver.h"
#include "elog.h"
#include "system_timer.h"

__IO FlagStatus g_transfer_complete = RESET;
uint8_t rx_dma_buffer[10];
const uint8_t txbuffer[] = "\nUSART0 DMA transmit\n";
const uint8_t txbuffer1[] = "\nUSART1 DMA transmit\n";
uint8_t i2c_write_buf[] = {0x00, 0x50, 0x59, 0x13, 0x10, 0x18, 0x01, 0x24};
uint8_t i2c_read_buf[7];
uint32_t system_freq;

uint8_t debug_control_flag = 0;
uint16_t send_time = 0;
uint8_t debug_buf[30];

uint8_t debug_control_flag1 = 0;
uint16_t send_time1 = 0;
uint8_t debug_buf1[30];

static void elog(void);

static void updateflag1(void)
{
    
}

static void restart_receive1(uint16_t data_len)
{
    send_time1 ++;
    sprintf((char *)debug_buf1, "uart1 recv %d time\b\bs\n\r", send_time1);
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
    I2cInitStruct i2c_init;
    uint8_t rd_wr_addr = 0;
    
    // LED PB12
    rcu_periph_clock_enable(RCU_GPIOB);
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ,GPIO_PIN_12);
    gpio_bit_reset(GPIOB, GPIO_PIN_12);
    
    uart_init.baudrate = 115200;
    uart_init.parity = ParityNone;
    uart_init.stop_bit = StopBit_1Bit;
    uart_init.word_length = WordLen_8Bit;
    
    SystemTimerInit();

    elog();
    
    UartInit(&Uart1, &uart_init);
    UartSendCallbackRegister(&Uart1, &updateflag1);
    UartRecvCallbackRegister(&Uart1, &restart_receive1);

    UartSendDMA(&Uart1, txbuffer1, sizeof(txbuffer1));
    UartReceiveToIdleDMA(&Uart1, rx_dma_buffer, sizeof(rx_dma_buffer));
    
    i2c_init.speed = 100000;
    i2c_init.local_addr = 0x47;
    I2cInit(&I2c0, &i2c_init);
    while(I2cWrite(&I2c0, 0x64, i2c_write_buf, sizeof(i2c_write_buf)) != 0);
    while(I2cWrite(&I2c0, 0x64, &rd_wr_addr, 1) != 0);
    while(I2cRead(&I2c0, 0x64, i2c_read_buf, sizeof(i2c_read_buf)) != 0);
    
//  GetSystemClock(&system_freq);
//  while(SetSystemClock(96000000));
//  GetSystemClock(&system_freq);
    
    while(1){
        static uint64_t last_flush_time = 0;
        uint64_t time = GetSystemTimer_us();
        static uint8_t led = 0;

        if(time - last_flush_time > 500000)
        {
            last_flush_time = time;
            elog_flush();		// 500ms调用一次即可
            if(led == 0)
            {
                led = 1;
                gpio_bit_set(GPIOB, GPIO_PIN_12);
            }
            else
            {
                led = 0;
                gpio_bit_reset(GPIOB, GPIO_PIN_12);
            }
        }
        
        if(debug_control_flag == 1)
        {
            while(I2cWrite(&I2c0, 0x64, &rd_wr_addr, 1) != 0);
            while(I2cRead(&I2c0, 0x64, i2c_read_buf, sizeof(i2c_read_buf)) != 0);
            debug_control_flag = 0;
        }
    }
    
}

static void elog(void)
{
      elog_init();
      /* set EasyLogger log format */
      elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_TAG | ELOG_FMT_TIME);
      elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_TAG | ELOG_FMT_TIME);
      elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_TAG | ELOG_FMT_TIME);
      elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_TAG | ELOG_FMT_TIME);
      elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_TAG | ELOG_FMT_TIME);
      elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_TAG | ELOG_FMT_TIME);
      /* start EasyLogger */
      elog_start();
}
