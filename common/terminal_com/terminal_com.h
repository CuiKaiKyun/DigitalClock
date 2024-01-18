#pragma once

#include "stdint.h"

typedef void CommandFuncType(void);

typedef struct __Command
{
    const char *command_string;
    CommandFuncType* command_func;
}Command;

int8_t TerminalComInit(void);

void TerminalCommandRegister(const char *command_string, CommandFuncType* command_func);
