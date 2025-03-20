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

#include "sdb.h"

#define NR_WP 32

#define EBREAK 0x00100073

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  word_t addr;
  char cond[100];
  bool enable;

} WP;

static WP wp_pool[NR_WP] = {0};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

void clear_wp(WP *wp)
{
  wp->next = NULL;
  wp->addr = 0;
  wp->enable = false;
  memset(wp->cond, 0, sizeof(wp->cond));
}

void add_wp_to_list(WP **list, WP *wp)
{
  WP *p = *list;

  if(!wp) return;

  if(!(*list))
  {
    *list = wp;
    return;
  }

  while(p && p->next)
  {
    p = p->next;
  }

  p->next = wp;
}

void create_watchpoint(char *cond)
{
  WP *wp = free_;
  
  if(free_ == NULL)
  {
    printf("no space for new wp\n");
    return;
  }

  wp->addr = 0;
  wp->enable = true;
  strncpy(wp->cond, cond, sizeof(wp->cond)-1 );
  
  free_ = free_->next;
  wp->next = NULL;
  add_wp_to_list(&head, wp);
}

void del_watchpoint(word_t NO)
{
  WP *p1 = NULL;
  WP *p2 = NULL;

  if(!head)
  {
    printf("No breakpoint number %u\n", NO);
    return;
  }
  
  if(head->NO == NO)
  {
    p1 = head;
    head = head->next;
    clear_wp(p1);
    add_wp_to_list(&free_, p1);
    return;
  }

  p1 = head;
  p2 = p1->next;
  while(p2 && p2->NO != NO)
  {
    p1 = p2;
    p2 = p2->next;
  }

  if(p2)
  {
    p1->next = p2->next;
    clear_wp(p2);
    add_wp_to_list(&free_, p2);
  }
  else
    printf("No breakpoint number %u\n", NO);
}

void show_wp_info()
{
  WP *p = head;
  if(!head)
  {
    printf("No breakpoints or watchpoints.\n");
    return;
  }

  printf("%-8s%-13s%-13s%-13s\n", "Num", "Type", "Disp", "what");
  while(p)
  {
    printf("%-8d%-13s%-13s%-13s\n", p->NO, "watchpoint", p->enable?"keep":"disable", p->cond);
    p = p->next;
  }
}

void check_watchpoint()
{
  // WP *p = head;
  // bool ret = true;
  // word_t a;

  // if(!head) return;

  // while(p)
  // {
  //   bool ret = true;
  //   word_t a = expr(args, &ret);

  //   if(eval)
  //   p = p->next;
  // }
}

void del_watchpoint_all()
{
  for(int i = 0; i < NR_WP; i++)
    del_watchpoint(i);
}