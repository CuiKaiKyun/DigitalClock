/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */
 
#include <stdio.h> 
#include "stdarg.h"
#include "string.h"

#include "elog.h"

#include "driver.h"
#include "system_timer.h"
// #include "FreeRTOS.h"
// #include "semphr.h"

#define USE_UART_LOG

#ifdef USE_UART_LOG
	#ifndef TERMINAL_UART
	#error "please define TERMINAL_UART first!"
	#endif
#endif // #define USE_UART_LOG

// 这里是默认就是使用buffer模式了
const char overflow_string[] = "\nuart overflow\n";
#define UART_DMA_BUFFER_LEN	    ELOG_BUF_OUTPUT_BUF_SIZE + sizeof(overflow_string)
#define LOG_BAUDRATE       115200U

static uint8_t dma_send_buffer[UART_DMA_BUFFER_LEN];
static uint8_t overflow_flag = 0;

static int8_t log_uart_init(void);
static int8_t log_uart_printf(const char * _format, ...);

/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;

    /* add your code here */
    // OutputLockMutex = xSemaphoreCreateMutex();
    log_uart_init();
    
    return result;
}

/**
 * EasyLogger port deinitialize
 *
 */
void elog_port_deinit(void) {

    /* add your code here */

}

/**
 * output log port interface
 * 增加output状态返回值，并且修改elog_buf文件对该函数的使用
 * @param log output of log
 * @param size log size
 */
int8_t elog_port_output(const char *log, size_t size) {
    /* output to terminal */
    int8_t ret;
    
    ret = log_uart_printf("%.*s", size, log);
    if(ret < 0 && size == ELOG_BUF_OUTPUT_BUF_SIZE)
    {
        // 如果某一包发送失败，且需要发送的数据大小为最大值（即缓存区也已经满了）
        // 则需要标记缓存区已经溢出了，发送的时候会在这个日志后面加一句溢出警告
        overflow_flag = 1;
    }
    return ret;
    /* add your code here */
    
}

/**
 * output lock
 */
void elog_port_output_lock(void) {
    
    /* add your code here */
	// xSemaphoreTake(OutputLockMutex, portMAX_DELAY);
    
}

/**
 * output unlock
 */
void elog_port_output_unlock(void) {
    
    /* add your code here */
	// xSemaphoreGive(OutputLockMutex);
    
}

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void) {
    static char time_string[12];
	uint64_t running_time_ms;
    float running_time_s;

	running_time_ms = GetSystemTimer_ms();
    running_time_s = running_time_ms / 1000.f;

    sprintf(time_string, "%8.3f", running_time_s);

    return (const char *)time_string;
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    
    return "";
    
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
    
    return "";
    
}

int8_t log_uart_init(void)
{
    UartInitStruct init;

    init.baudrate = LOG_BAUDRATE;
    init.word_length = WordLen_8Bit;
    init.stop_bit = StopBit_1Bit;
    init.parity = ParityNone;
    
    return UartInit(TERMINAL_UART, &init);
}

/**
 * @brief 打印函数
 * 
 * @param _format 待打印的字符串
 * @param ... 参数
 * @return int8_t 
 */
int8_t log_uart_printf(const char * _format, ...)
{
    va_list ap;
    
    if(TERMINAL_UART->send_info.send_busy != 0)
        return -1;

    va_start(ap, _format);
    vsnprintf((char *)dma_send_buffer, ELOG_BUF_OUTPUT_BUF_SIZE, _format, ap);
    va_end(ap);

    if(overflow_flag == 1)
    {
        memcpy(&dma_send_buffer[ELOG_BUF_OUTPUT_BUF_SIZE], overflow_string, sizeof(overflow_string));
        UartSendDMA(TERMINAL_UART, dma_send_buffer, UART_DMA_BUFFER_LEN);
    }
    else
    {
        UartSendDMA(TERMINAL_UART, dma_send_buffer, strlen((const char *)dma_send_buffer));
    }
    
    return 0;
}
