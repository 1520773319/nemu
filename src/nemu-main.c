/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <trace.h>
#include <signal.h>

extern int elf_fd;
extern void *elf;
extern char *elf_file;

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

void sigint_handler(int signum)
{
  printf("recv sig int: %d. current pc: 0x%x\n", signum, cpu.pc);
  nemu_state.state = NEMU_STOP;
}

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  signal(SIGINT, sigint_handler);
  /* Load elf symbol-table, for ftrace */
  if(elf_file)
  {
    elf = attach_elf(elf_file);
    readelf(elf);
  }

  /* Start engine. */
  engine_start();

  /* Unload elf */
  if(elf && elf_fd > 0)
  {
    detach_elf(elf_fd, elf);
  }

  return is_exit_status_bad();
}
