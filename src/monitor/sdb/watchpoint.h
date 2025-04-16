#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

void create_watchpoint(char *);
void del_watchpoint(word_t);
void create_breakpoint(char *);
void show_wp_info();
void check_watchpoint();
void del_watchpoint_all();
void disable_bp(vaddr_t);
void enable_bp(vaddr_t);
bool is_valid_bp(vaddr_t);

#endif