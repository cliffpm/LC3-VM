
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <Windows.h>
#include <conio.h> 

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



// defining OP Code

enum {
    OP_BR = 0, // branch
    OP_ADD, // add 
    OP_LD, // load
    OP_ST, // store
    OP_JSR, // jump register
    OP_AND, // bitwise and 
    OP_LDR, // load register
    OP_STR, // store register 
    OP_RTI, // unused 
    OP_NOT, // bitwise not 
    OP_LDI, // load indirect
    OP_STI, // store indirect
    OP_JMP, // jump
    OP_RES, // reserved (unused)
    OP_LEA, // load effective address 
    OP_TRAP // execute trap
};

// conditional flags:

enum {
    FL_POS = 1 << 0,
    FL_ZRO = 1 << 1,
    FL_NEG = 1 << 2
};

enum {
    TRAP_GETCHAR = 0x20, // get char from keyboard
    TRAP_OUTPUTCHAR = 0x21, // output char
    TRAP_PUTS = 0x22, // output a word string 
    TRAP_IN = 0x23, // get character from keyboard and echo onto terminal
    TRAP_PUTSP = 0x24, // output a byte string
    TRAP_HALT = 0x25 // halt program
};


// special memory mapped registers not accessible from normal register table
enum {
    MR_KBSR = 0xFE00, // keyboard status register
    MR_KBDR = 0xFE02 // keyboard data register
};


// our mem has 2^16 locations (max that is addressable by 16 bit unsigned int). Each location stores a 16 bit value. Total capacity is 128 KB.
#define MEMORY_MAX (1 << 16) // effectively is 1 * 2^16
uint16_t memory[MEMORY_MAX]; // our memory with 2^16 possible locations. Each hold max 16 bits.

// storing registers in an array


uint16_t reg[REGISTER_COUNT];

// instruction set




// keyboard i/o for window os

HANDLE hStdin = INVALID_HANDLE_VALUE;
DWORD fdwMode, fdwOldMode;

void disable_input_buffering()
{
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &fdwOldMode); /* save old mode */
    fdwMode = fdwOldMode
            ^ ENABLE_ECHO_INPUT  /* no input echo */
            ^ ENABLE_LINE_INPUT; /* return when one or
                                    more characters are available */
    SetConsoleMode(hStdin, fdwMode); /* set new mode */
    FlushConsoleInputBuffer(hStdin); /* clear buffer */
}

void restore_input_buffering()
{
    SetConsoleMode(hStdin, fdwOldMode);
}

uint16_t check_key()
{
    return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit();
}
//


uint16_t sign_extend(uint16_t x, int bit_count) {
    if ((x >> (bit_count-1))& 1) {
        x |= (0XFFF << bit_count);
    }
    return x;
}

void updateFlags(uint16_t r) {
    if (reg[r] == 0) reg[REGISTER_COND] = FL_ZRO;
    else if (reg[r] >> 15) reg[REGISTER_COND] = FL_NEG;
    else reg[REGISTER_COND] = FL_POS;
}

uint16_t swap16(uint16_t x) {
    return (x << 8) | (x >> 8);
}

// function to read in LC3 image file
void read_image_file(FILE* file) {
    uint16_t origin;
    fread(&origin, sizeof(origin),1,file);
    origin = swap16(origin); // read the origin in little endian (my machine is X86)

    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t* p = memory + origin; // starting address for our load image
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    // after each read, 
    while (read-- > 0) {
        *p = swap16(*p);
        ++p;
    }
}



int read_image(const char* image_path) {
    FILE* file = fopen(image_path, "rb");
    if (!file) {
        return 0;
    }
    read_image_file(file);
    fclose(file);
    return 1;
}

// setter and getter functions for memory mapped registers

void mem_write(uint16_t address, uint16_t val) {
    memory[address] = val;
}

uint16_t mem_read(uint16_t address) {
    if (address == MR_KBSR) {
        if (check_key()) {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }
        else memory[MR_KBSR] = 0;
    } 

    return memory[address];
}

void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}



int main (int argc, const char* argv[]) {


    if (argc < 2) {
        printf("LC-3 [image-file1] ...\n");
        exit(2);
    }

    for (int j = 1; j < argc; ++j) {
        if (!read_image(argv[j])) {
            printf("failed to load image : %s\n", argv[j]);
            exit(1);
        }
    }

    disable_input_buffering();
    signal(SIGINT, handle_interrupt);
    


    reg[REGISTER_COND] = FL_ZRO;
    enum {PC_START = 0x3000};
    reg[REGISTER_PC] = PC_START; 

    int running = 1;



    // processing instructions loop
    while (running) {
        // fetching instruction
        uint16_t instr = mem_read(reg[REGISTER_PC]++);
        uint16_t opcode = instr >> 12;
        
        switch (opcode) {
            case OP_ADD: {
                uint16_t r0 = (instr >> 9) & 0x7;

                uint16_t r1 = (instr >> 6)  &  0x7;

                uint16_t immediate = (instr >> 5) & 0x1;

                if (immediate) {
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[r0] = reg[r1] + imm5;
                }
                else {
                    uint16_t r2 = instr & 0x7;
                    reg[r0] = reg[r1] + reg[r2];
                }
                updateFlags(r0);
            }
            break;
            case OP_LDI: {
                uint16_t rd = (instr >> 9) & 0x7;
                uint16_t pcoffset9 = sign_extend(instr & 0x1FF, 9);
                reg[rd] = mem_read(mem_read(reg[REGISTER_PC] + pcoffset9));

                updateFlags(rd);
            }
            break;
            case OP_NOT:
            {
                uint16_t rd = (instr >> 9) & 0x7;
                uint16_t sr = (instr >> 6) & 0x7;                
                
                reg[rd] = ~(reg[sr]);
                updateFlags(rd);
            }
            break;
            case OP_BR:
            {
                uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                uint16_t cond_flag = (instr >> 9) & 0x7;
                if (cond_flag & reg[REGISTER_COND])
                {
                    reg[REGISTER_PC] += pc_offset;
                }
            }
            break;
            case OP_JMP:
            {
                uint16_t rd = (instr>>6) & 0x7;
                reg[REGISTER_PC] = reg[rd];
            }
            break;
            case OP_JSR:
            {
                reg[REGISTER7] = reg[REGISTER_PC];
                if (!((instr >>11 ) & 1)) { // JSRR instruction
                    uint16_t baseR = (instr >>6) & 0x7;
                    reg[REGISTER_PC] = reg[baseR];
                } 
                else { // JSR instruction
                    uint16_t long_pc_offset = sign_extend(instr & 0x7FF, 11);
                    reg[REGISTER_PC] += long_pc_offset;
                }
            }
            break;
            
            case OP_LD:
            {
                uint16_t rd = (instr >> 9) & 0x7;
                uint16_t pc_offset = sign_extend(instr &  0x1FF, 9);
                reg[rd] = mem_read(reg[REGISTER_PC] + pc_offset);
                updateFlags(rd);
            }
            break;
            case OP_AND:
            {
                uint16_t rd = (instr >> 9) & 0x7;
                uint16_t sr1 = (instr >> 6) & 0x7;
                if (!((instr >> 5) & 0x1)) {
                    uint16_t sr2 = instr & 0x7;
                    reg[rd] = reg[sr1] & reg[sr2];
                }
                else {
                    uint16_t imm5 = sign_extend((instr & 0x1F), 5);
                    reg[rd] = reg[sr1] & imm5;
                }
                updateFlags(rd);
            }
            break;
            case OP_LDR:
            {
                uint16_t rd = (instr >> 9) & 0x7;
                uint16_t baseR = (instr >> 6) & 0x7;
                uint16_t offset = sign_extend(instr & 0x3F,6);
                reg[rd] = mem_read(reg[baseR] + offset);
                updateFlags(rd);
            }
            break;
            case OP_LEA:
            {
                uint16_t rd = (instr >> 9) & 0x7;
                uint16_t pc_offset =  sign_extend(instr & 0x1FF,9);
                reg[rd] = reg[REGISTER_PC] +pc_offset;
                updateFlags(rd);
            }
            break;
            case OP_ST:
            {
                uint16_t sr = (instr >> 9) & 0x7;
                uint16_t offset = sign_extend(instr & 0x1FF,9);
                mem_write(reg[REGISTER_PC] + offset, reg[sr]);
            }
            break;
            case OP_STI:
            {
                uint16_t sr = (instr >> 9) & 0x7;
                uint16_t offset = sign_extend(instr & 0x1FF, 9);
                mem_write(mem_read(reg[REGISTER_PC] + offset), reg[sr]);
            }
            break;
            case OP_STR:
            {
                uint16_t baseR = (instr >> 6) & 0x7;
                uint16_t sr = (instr >> 9) & 0x7;
                uint16_t offset = sign_extend(instr & 0x3F,6);
                mem_write(reg[baseR] + offset, reg[sr]);
            }
            break;

            case OP_TRAP:
                reg[REGISTER7] = reg[REGISTER_PC];
                switch(instr & 0xFF) {
                    case TRAP_GETCHAR:
                        reg[REGISTER0] = (uint16_t)getchar();
                        updateFlags(REGISTER0);
                        break;
                    case TRAP_OUTPUTCHAR:
                        putc((char)(reg[REGISTER0]), stdout);
                        fflush(stdout);
                        break;
                    case TRAP_PUTS:
                    {
                        uint16_t* c = memory + reg[REGISTER0];
                        while (*c) {
                            putc((char)*c, stdout);
                            ++c;
                        }
                        fflush(stdout);
                    }
                    break;
                    case TRAP_IN:
                    {
                        printf("Enter a character: ");
                        char c = getchar();
                        putc(c, stdout);
                        fflush(stdout);
                        reg[REGISTER0] = (uint16_t)c;
                        updateFlags(REGISTER0);
                    }
                    break;
                    case TRAP_PUTSP:
                    {
                        uint16_t* c = memory + reg[REGISTER0];
                        while(*c) {
                            putc((char)(*c & 0xFF), stdout);
                            if ((*c) >> 8) putc((char)((*c)>>8), stdout);
                            ++c;
                        }   
                        fflush(stdout);
                    }
                    break;
                    case TRAP_HALT:
                        puts("Program has been halted");
                        fflush(stdout);
                        running = 0;
                        break;

                }

                break;

            case OP_RES:

            case OP_RTI:
            default:
                abort();
                break;



        }
    }
    restore_input_buffering();
}

