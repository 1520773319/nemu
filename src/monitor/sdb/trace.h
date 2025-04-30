#ifndef __TRACE_H__
#define __TRACE_H__

#include <common.h>

typedef struct inst_trace
{
    word_t addr;
    char inst[128];
    word_t opcode;
}itrace_t;

#define IRINFBUF_SIZE 16
void itrace(char *inst, word_t opcode, word_t addr);
void iringbuf_show();

#endif