#pragma once

#include "stdint.h"

typedef void CommandFuncType(void);

typedef struct __Command
{
    const char *command_string;
    CommandFuncType* command_func;
}Command;

int8_t TerminalComInit(void);

int8_t TerminalCommandRegister(const char *command_string, CommandFuncType* command_func);

int8_t terminal_output(void);
