
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <Windows.h>
#include <conio.h>  // _kbhit

// our mem has 2^16 locations (max that is addressable by 16 bit unsigned int). Each location stores a 16 bit value. Total capacity is 128 KB.
#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX];


// defining registers. Reg 0-7 are general purpose. One register for PC counter and one for Conditional Flags.

enum {
    REGISTER0 = 0,
    REGISTER1,
    REGISTER2,
    REGISTER3,
    REGISTER4,
    REGISTER5,
    REGISTER6,
    REGISTER7,
    REGISTER_PC,
    REGISTER_COND,
    REGISTER_COUNT
};

// storing registers in an array


uint16_t reg[REGISTER_COUNT];

// instruction set

// defining OP Code

enum {
    OP_BR = 0,
    OP_ADD,
    OP_LD,
    OP_ST,
    OP_JSR,
    OP_AND,
    OP_LDR,
    OP_STR,
    OP_RTI,
    OP_NOT,
    OP_LDI,
    OP_STI,
    OP_JMP,
    OP_RES,
    OP_LEA,
    OP_TRAP
};

// conditional flags:

enum {
    FL_POS = 1 << 0,
    FL_ZRO = 1 << 1,
    FL_NEG = 1 << 2
};