#include "trace.h"
#include <isa.h>
#include <unistd.h>

iringbuf_t iringbuf[IRINFBUF_SIZE] = {0};
int iring = 0;

static inline void format_inst(char *inst)
{
    if(!inst) return;

    for (int i = 0; i < strlen(inst); i++)
        if(inst[i] == '\t')
            inst[i] = ' ';


    char *c = " ";
    char *token = strtok(inst, c);
    char new_inst[128] = {0};
    int offset = 0;
    int flag = 1;

    while (token != NULL)
    {
        char temp[64] = {0};

        if(flag)
            sprintf(temp, "%-7s", token);
        else
            sprintf(temp, "%-5s", token);
        
        strcpy(new_inst+offset, temp);
        offset += strlen(temp);

        token = strtok(NULL, c);

        if(flag)
            flag = 0;
    }

    strcpy(inst, new_inst);
}

void itrace(char *inst, word_t opcode, word_t addr)
{
    iringbuf_t trace;
    memset(&trace, 0, sizeof(iringbuf_t));

    trace.addr = addr;
    trace.opcode = opcode;
    strcpy(trace.inst, inst);

    format_inst(trace.inst);

    iringbuf[iring] = trace;
    iring = (iring + 1) % IRINFBUF_SIZE ;
}

void iringbuf_show(word_t pc)
{
    // extern CPU_state cpu;

    for (int i = 0; i < IRINFBUF_SIZE; i++)
    {
        int index = (iring + i) % IRINFBUF_SIZE;
        word_t opcode = iringbuf[index].opcode;

        if(iringbuf[index].addr == 0)
            continue;

        if(pc != iringbuf[index].addr)
            printf("%-3s 0x%x: %-5s %-27s %02x %02x %02x %02x\n", "", iringbuf[index].addr, "", iringbuf[index].inst,
                opcode >> 24 & 0xFF,
                opcode >> 16 & 0xFF,
                opcode >> 8 & 0xFF,
                opcode & 0xFF);
        else
            printf("--> 0x%x: %-5s %-27s %02x %02x %02x %02x\n", iringbuf[index].addr, "", iringbuf[index].inst,
                opcode >> 24 & 0xFF,
                opcode >> 16 & 0xFF,
                opcode >> 8 & 0xFF,
                opcode & 0xFF);
            
    }
}