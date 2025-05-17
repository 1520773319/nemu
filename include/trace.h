#ifndef __TRACE_H__
#define __TRACE_H__

#include <common.h>

typedef struct inst_ringbuf
{
    word_t addr;
    char inst[128];
    word_t opcode;
}iringbuf_t;

#define IRINFBUF_SIZE 16
void itrace(char *inst, word_t opcode, word_t addr);
void iringbuf_show();

void *attach_elf(char *);
void detach_elf(int, void *);
void readelf(void *);
void ftrace(word_t);

#endif