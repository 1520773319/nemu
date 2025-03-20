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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "regex.h"
#include "sdb.h"
#include <math.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
word_t hexstr_to_num();

static int cmd_c(char *args);
static int cmd_q(char *args);
static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_p(char *args);
static int cmd_test(char *args);
static int cmd_info(char *args);
static int cmd_memx(char *args);
static int cmd_wp(char *args);
static int cmd_delwp(char *args);
static int cmd_showbre(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Single step", cmd_si},
  { "p", "print expression", cmd_p},
  { "t", "test", cmd_test},
  { "info", "print registers", cmd_info},
  { "x", "print memroy", cmd_memx},
  { "w", "watchpoint", cmd_wp},
  { "del",  "del breakpoint", cmd_delwp},
  { "show", "info breakpoint", cmd_showbre},
  /* TODO: Add more commands */

};
#define NR_CMD ARRLEN(cmd_table)

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args)
{ 
  const char *pattern = "^[1-9][0-9]*$";
  regex_t regex;
  int ret;
  
  if (!args)
  {
    cpu_exec(1);
    return 0;
  }
  
  ret = regcomp(&regex, pattern, REG_EXTENDED);
  if (ret != 0)
  {
    printf("Could not compile regex\n");
    return 0;
  }

  ret = regexec(&regex, args, 0, NULL, 0);
  regfree(&regex);

  if(ret == 0)
  {
    cpu_exec(atoi(args));
  }
  else if (ret == REG_NOMATCH)
  {
    printf("Unknown command '%s'\n", args);
  }
  else
    printf("Regex match failed\n");

  return 0;
}

static int cmd_p(char *args)
{ 
  bool ret = true;
  word_t a = expr(args, &ret);

  if(ret == false)
    printf("p %s false ! \n\n", args);
  else
    printf("%u\n\n", a);

  return 0;
}

static int cmd_test(char *args)
{ 
  int error = 0;
  FILE *file = fopen("/home/wangc/ics-pa/nemu/tools/gen-expr/a.txt", "r");
  if (file == NULL) {
      perror("Failed to open file");
      return EXIT_FAILURE;
  }

  char str1[100], str2[100];

  printf("Read: /home/wangc/ics-pa/nemu/tools/gen-expr/a.txt\n");

  // 按行读取两个字符串
  while (fscanf(file, "%s %s", str1, str2) == 2) {
    bool ret = true;
    word_t a = expr(str2, &ret);
    char tmp[20]={0};
    sprintf(tmp, "%u", a);

    if(ret == true && strcmp(tmp, str1) == 0)
    {
      // printf("%s\n%s\n", str2, str1);
      // printf("%s\n\n", ANSI_FMT("TRUE", ANSI_FG_GREEN));
    }
    else
    {
      printf("%s\n%s\n", str2, str1);
      printf("%s: %s\n\n", ANSI_FMT("FALSE", ANSI_FG_YELLOW ANSI_BG_RED), tmp);
      error++;
    }
  }

  if(!error)
    printf("%s\n\n", ANSI_FMT("ALL IS RIGHT!", ANSI_FG_GREEN));
  else
    printf("error: %d\n\n", error);

  fclose(file);
  return 0;
}

static int cmd_info(char *args)
{
  isa_reg_display();
  return 0;
}

static int cmd_memx(char *args)
{
  if(args == NULL)
  {
    printf("Argument required (starting display address).\n");
    return 1;
  }

  const char *pattern = "^0x[0-9]*$";
  regex_t regex;
  word_t mem;
  int ret;
  
  ret = regcomp(&regex, pattern, REG_EXTENDED);
  if (ret != 0)
  {
    printf("Could not compile regex\n");
    return 0;
  }

  ret = regexec(&regex, args, 0, NULL, 0);
  regfree(&regex);

  if(ret == 0)
  {
    mem = vaddr_read(hexstr_to_num(args), 4);
    printf("0x%08x\n", mem);
  }
  else if (ret == REG_NOMATCH)
  {
    printf("Unknown command '%s'\n", args);
  }
  else
    printf("Regex match failed\n");

  return 0;
}

static int cmd_wp(char *args)
{
  create_watchpoint(args);
  return 0;
}

static int cmd_delwp(char *args)
{
  if(!args) return 0;
    
  if(strcmp(args, "all") == 0)
      del_watchpoint_all();
  else
    del_watchpoint(atoi(args));
  return 0;
}

static int cmd_showbre(char *args)
{
  show_wp_info();
  return 0;
}

word_t hexstr_to_num(char *str)
{
  int len = strlen(str);
  word_t sum = 0;
  int i = 0;

  for(i = len - 1; i > 1; i--)
  {
    if(isdigit(str[i]))
      sum += (str[i] - '0')* pow(16, (len-2-i+1));
    else if(str[i] >= 'a' && str[i] <= 'f')
      sum += (str[i] - 'a' + 10)* pow(16, (len-2-i+1));
    else if(str[i] >= 'A' && str[i] <= 'F')
      sum += (str[i] - 'A' + 10)* pow(16, (len-2-i+1));
  }

  return sum;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}