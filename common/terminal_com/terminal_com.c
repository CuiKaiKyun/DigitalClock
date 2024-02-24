#include "terminal_com.h"
#include "driver.h"
#include <stdio.h> 
#include "string.h"
#include "elog.h"


#ifndef TERMINAL_UART
#error "please define TERMINAL_UART first!"
#endif

#define MAX_COMMAND_NUM         10

#define TERMINAL_BAUDRATE           115200U
#define TERMINAL_DMA_TX_BUF_SIZE    32
#define TERMINAL_DMA_RX_BUF_SIZE    32
#define TERMINAL_CMD_MAX_LEN        32

#define CHAR_TAB                0X09
#define CHAR_ENTER              0x0d
#define CHAR_BACKSPACE          0x7f
#define CHAR_SPACE              ' '

static Command command[MAX_COMMAND_NUM];
static uint16_t command_num = 0;

static uint8_t terminal_rx_dma_buf[TERMINAL_DMA_RX_BUF_SIZE];
static struct
{
    uint8_t buffer[TERMINAL_CMD_MAX_LEN + 1];
    uint16_t num;
}unformed_command;

static struct 
{
    uint8_t buffer[TERMINAL_DMA_TX_BUF_SIZE];
    uint8_t dma_buffer[TERMINAL_DMA_TX_BUF_SIZE];
    uint16_t num;
}unsent_obj;

static void terminal_input_process(uint16_t size);
static int8_t compare_command(Command **command);
static int8_t complete_command(void);
static void command_list(void);

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

    UartReceiveToIdleDMA(TERMINAL_UART, terminal_rx_dma_buf, TERMINAL_DMA_RX_BUF_SIZE);
    
    TerminalCommandRegister("cmd_list", &command_list);

    return ret;
}

int8_t TerminalCommandRegister(const char *command_string, CommandFuncType *command_func)
{
    if(strlen(command_string) > TERMINAL_CMD_MAX_LEN) {
        return -1;
    }
    if(command_num < MAX_COMMAND_NUM){
        command[command_num].command_string = command_string;
        command[command_num].command_func = command_func;
        command_num ++;

        return 0;
    }

    return -1;
}

static void terminal_input_process(uint16_t size)
{
    for(uint8_t i = 0; i < size; i ++)
    {
        if(terminal_rx_dma_buf[i] == CHAR_ENTER)
        {
            Command *_command;
            if(unformed_command.num == 0)
            {
                // 清空输入缓存区
                memset(unformed_command.buffer, 0, unformed_command.num);
                unformed_command.num = 0;
                // 输出缓存区添加一个换行符
                unsent_obj.buffer[unsent_obj.num] = '\n';
                unsent_obj.num ++;
                
                break;
            }
            
            if(compare_command(&_command) == 0)
            {
                // 清空输入缓存区
                memset(unformed_command.buffer, 0, unformed_command.num);
                unformed_command.num = 0;
                // 输出缓存区添加一个换行符
                unsent_obj.buffer[unsent_obj.num] = '\n';
                unsent_obj.num ++;
                
                // 要不就直接执行该函数
                // 要不就自己创建一个任务，在任务中发送队列
                if(_command->command_func != NULL)
                {
                    _command->command_func();
                }
            }
        }
        else if(terminal_rx_dma_buf[i] == CHAR_TAB)
        {
            complete_command();
        }
        else if(terminal_rx_dma_buf[i] == CHAR_BACKSPACE)
        {
            // 在输入缓存区删除一个字符
            if(unformed_command.num > 0)
            {
                unformed_command.num --;
                unformed_command.buffer[unformed_command.num] = 0;
            }
            // 给发送区缓存添加"回退，空格，回退"
            unsent_obj.buffer[unsent_obj.num] = CHAR_BACKSPACE;
            unsent_obj.num ++;
            unsent_obj.buffer[unsent_obj.num] = CHAR_SPACE;
            unsent_obj.num ++;
            unsent_obj.buffer[unsent_obj.num] = CHAR_BACKSPACE;
            unsent_obj.num ++;
        }
        else
        {
            // 预留一个字符的位置给'\0'
            if(unformed_command.num >= TERMINAL_CMD_MAX_LEN) {
                break;
            }
                
            // 在输入缓存区添加这个字符
            unformed_command.buffer[unformed_command.num] = terminal_rx_dma_buf[i];
            unformed_command.num ++;
            // 直接在发送区缓存添加这个字符
            unsent_obj.buffer[unsent_obj.num] = terminal_rx_dma_buf[i];
            unsent_obj.num ++;
        }
    }

    UartReceiveToIdleDMA(TERMINAL_UART, terminal_rx_dma_buf, TERMINAL_DMA_RX_BUF_SIZE);
}

/**
 * @brief 将接收到的指令和已有的指令比较，如果存在相同指令，则返回0
 * 
 * @param command 传出参数，相匹配的指令的指针
 * @return int8_t 
 */
static int8_t compare_command(Command **_command)
{
    *_command = NULL;
    // 逐个命令比对，遇到相同则返回对应函数指针
    for(uint8_t i = 0; i < MAX_COMMAND_NUM; i ++)
    {
        if(strcmp((const char *)unformed_command.buffer, (const char *)command[i].command_string) == 0)
        {
            *_command = &command[i];
            return 0;
        }
    }

    return -1;
}

/**
 * @brief 指令联想，自动补齐
 * 
 * @return int8_t 
 */
int8_t complete_command(void)
{
    while(1) {
        uint8_t match_flag = 0;
        uint8_t char_diff_flag = 0;
        uint8_t next_char = 0;
        uint8_t i = 0;

        // 逐个取指令的前面几个字符来比对
        for(i = 0; i < MAX_COMMAND_NUM; i ++)
        {
            // 输入的代码如果并不比该指令长，则跳过
            if(unformed_command.num >= strlen((const char *)command[i].command_string)) {
                continue;
            }

            // 如果输入的代码和指令的开头一致，则记录该指令的下一个字符
            if(memcmp((const char *)unformed_command.buffer, (const char *)command[i].command_string, unformed_command.num) == 0) {
                // 如果还有其它指令的开头也和输入的相匹配，则需要看两者的下一个字符是否一致
                if(match_flag == 1) {
                    // 如果两个指令的下一个字符不一致，即：若输入的是ab，存在abc和abd两个指令，则无法联想，直接退出
                    if(next_char != command[i].command_string[unformed_command.num]) {
                        char_diff_flag = 1;
                        break;
                    }
                }
                else {
                    // 记录该指令的下一个字符
                    match_flag = 1;
                    next_char = command[i].command_string[unformed_command.num];
                }
            }
        }

        // 如果输入的和某个指令相匹配且下一个字符一致
        if(match_flag == 1 && char_diff_flag != 1) {
            
            // 在输入缓存区添加这个字符
            unformed_command.buffer[unformed_command.num] = next_char;
            unformed_command.num ++;
            // 直接在发送区缓存添加这个字符
            unsent_obj.buffer[unsent_obj.num] = next_char;
            unsent_obj.num ++;
        }
        else {
            break;
        }
    }

    return 0;
}

/**
 * @brief 终端输出的函数
 * 
 * @return int8_t 
 */
int8_t terminal_output(void)
{
    if(TERMINAL_UART->send_info.send_busy != 0)
        return -1;

    if(unsent_obj.num != 0)
    {
        memcpy(unsent_obj.dma_buffer, unsent_obj.buffer, unsent_obj.num);
        UartSendDMA(TERMINAL_UART, unsent_obj.dma_buffer, unsent_obj.num);
        unsent_obj.num = 0;
    }
    
    return 0;
}

void command_list(void)
{
    for(uint8_t i = 0; i < command_num; i ++)
    {
        elog_i("terminal", "cmd %d %s", i, command[i].command_string);
    }
}
