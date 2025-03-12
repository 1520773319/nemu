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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

unsigned int getRandomSeed() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (unsigned int)(tv.tv_sec * 1000000 + tv.tv_usec);
}

unsigned int randomNumber() {
  srand(getRandomSeed());
  unsigned int highBits = (unsigned int)rand() << 16;
  unsigned int lowBits = (unsigned int)rand();
  unsigned int result = highBits | lowBits;

  return result & 0xFF;
}

#define MAXTOKENS 28
static void gen(char *left, char *right, int tokens)
{ 
  int llen = left?strlen(left):0;
  int rlen = right?strlen(right):0;
  char s[512] = {0};
  char op[4] = {'+', '-', '*', '/'};
  char sign = op[randomNumber()%4];
  int isParentheses=randomNumber()%2;
  unsigned int l = randomNumber();
  unsigned int r = randomNumber();

  if(tokens >= MAXTOKENS)
  {
    if(left)
      sprintf(buf, "%s", left);
    else
      sprintf(buf, "%s", right);

    buf[strlen(buf)] = '\0';
    return;
  }

  if(sign == '/')
  {
    rlen = 0; // 如果是除法， 右表达式用单个数字替代
    while(r == 0)
      r = randomNumber();
  }

  if(llen != 0 && rlen != 0)
  {
    if(isParentheses)
    {
      sprintf(s, "(%s%c%s)",left, sign, right);
      tokens += 1+2;
    }
    else 
    {
      sprintf(s, "%s%c%s", left, sign, right);
      tokens += 1;
    }
  }
  else if(llen == 0 && rlen == 0)
  {
    if(isParentheses)
    {
      sprintf(s, "(%d%c%d)", l, sign, r);
      tokens += 3+2;
    }
    else 
    {
      sprintf(s, "%d%c%d", l, sign, r);
      tokens += 3;
    }
  }
  else if(llen != 0 && rlen == 0)
  {     
    if(isParentheses)
    {
      sprintf(s, "(%s%c%d)", left, sign, r);
      tokens += 2 + 2;
    }
    else 
    {
      sprintf(s, "%s%c%d", left, sign, r);
      tokens += 2;
    }
  }
  else if(llen == 0 && rlen != 0)
  { 
    if(isParentheses)
    {
      sprintf(s, "(%d%c%s)", l, sign, right);
      tokens += 2 + 2;
    }
    else
    {
      sprintf(s, "%d%c%s", l, sign, right);
      tokens += 2;
    }
  }

  if(randomNumber()%2)
    gen(s, NULL, tokens);
  else
    gen(NULL, s, tokens);
}

static void gen_rand_expr() {
  gen(NULL, NULL, 0);
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 2000;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
