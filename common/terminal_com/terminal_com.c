#include "terminal_com.h"
#include "driver.h"


#ifndef TERMINAL_UART
#error "please define TERMINAL_UART first!"
#endif

#define MAX_COMMAND_NUM         10

#define TERMINAL_BAUDRATE       115200U
#define TERMINAL_DMA_BUF_SIZE   32

#define CHAR_TAB                0X09
#define CHAR_ENTER              0x0d

static Command command[MAX_COMMAND_NUM];
static uint16_t command_num = 0;

static uint8_t terminal_rx_dma_buf[TERMINAL_DMA_BUF_SIZE];

static void terminal_input_process(uint16_t size);
static int8_t process_command(void);

int8_t TerminalComInit(void)
{
    UartInitStruct init;
    uint8_t ret;

    init.baudrate = TERMINAL_BAUDRATE;
    init.word_length = WordLen_8Bit;
    init.stop_bit = StopBit_1Bit;
    init.parity = ParityNone;
    
    ret = UartInit(TERMINAL_UART, &init);

    UartRecvCallbackRegister(TERMINAL_UART, &terminal_input_process);

    UartReceiveToIdleDMA(TERMINAL_UART, terminal_rx_dma_buf, TERMINAL_DMA_BUF_SIZE);

}

void TerminalCommandRegister(const char *command_string, CommandFuncType *command_func)
{
    if(command_num < MAX_COMMAND_NUM)
    {
        command[command_num].command_string = command_string;
        command[command_num].command_func = command_func;
        command_num ++;
    }
}

static void terminal_input_process(uint16_t size)
{
    
    for(uint8_t i; i < size; i ++)
    {
        if(terminal_rx_dma_buf[i] == CHAR_ENTER)
        {
            if(process_command() == 0)
            {
                // 要不就直接执行该函数
                // 要不就自己创建一个任务，在任务中发送队列
            }
            // 清空输入缓存区
            // 直接在发送区缓存添加'\n'
        }
        else if(terminal_rx_dma_buf[i] == CHAR_TAB)
        {
            if(complete_command() == 0)
            {
                // 在输入缓存区加上补齐的指令
                // 给发送区缓存添加补齐的指令
            }
        }
        else
        {
            // 在输入缓存区添加这个字符
            // 直接在发送区缓存添加这个字符
        }
    }
}

static int8_t process_command(void)
{
    

    return 0;
}

