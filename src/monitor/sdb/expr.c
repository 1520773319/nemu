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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include "sdb.h"

enum {
  TK_NOTYPE = 256, 
  TK_EQ,
  TK_PLUS, 
  TK_SUB,
  TK_MULTI,
  TK_DIV,
  TK_LP,
  TK_RP,
  TK_NUM,
  TK_HEXNUM,
  TK_REG,
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +",  TK_NOTYPE},               // spaces
  {"\\+", TK_PLUS},                 // plus
  {"==",  TK_EQ},                   // equal
  {"-",   TK_SUB},                  // subtraction
  {"\\*", TK_MULTI},                // multiply
  {"\\/", TK_DIV},                  // division
  {"\\(", TK_LP},                   // open paren
  {"\\)", TK_RP},                   // close paren
  {"0x[0-9a-fA-F]+",   TK_HEXNUM},  // Hex number
  {"[0-9]*\\.?[0-9]+", TK_NUM},     // number
  {"\\$[\\$0-9a-z]+",  TK_REG},     // register
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

enum EXPR_TYPE
{
  SUCCESS = 0,
  NO_SURROUNDED,
  NO_MATCH,
  BAD,
};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;
sword_t eval(Token *p, Token *q, bool *success);
enum EXPR_TYPE check_parentheses(Token *p, Token *q);
Token* get_operation_main(Token *p, Token *q);

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  /* could be influenced by previous exepression */ 
  memset(tokens, 0, sizeof(tokens));

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          default:
            assert(nr_token < sizeof(tokens)/sizeof(Token));
            
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            nr_token++;
            
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  // for(int i=0; i < nr_token; i++)
  //   printf("%d %s\n", tokens[i].type, tokens[i].str);

  /* TODO: Insert codes to evaluate the expression. */
  return eval(&(tokens[0]), &(tokens[nr_token-1]), success);
  
}

sword_t eval(Token *p, Token *q, bool *success)
{
  Token *op = NULL;
  sword_t val1 = 0, val2 = 0;

  if(p > q)
  {
    return 0;
  }
  else if(p == q)
  {
    switch(p->type)
    {
      case TK_NUM:    return atoi(p->str);break;
      case TK_HEXNUM: return hexstr_to_num(p->str);break;
      case TK_REG:    return isa_reg_str2val(p->str, success);break;
      default:        *success = false;return 0;
    }
  }
  else if(check_parentheses(p, q) == SUCCESS)
  {
    return eval(p+1, q-1, success);
  }
  else if(check_parentheses(p, q) == BAD)
  {
    *success = false;
    return 0;
  }
  else
  {
    op = get_operation_main(p, q);
    // assert(op != NULL);
    if(op == NULL)
      return 0;

    val1 = eval(p, op - 1, success);
    val2 = eval(op + 1, q, success);

    if(*success == false)
      return 0;

    switch (op->type) {
      case TK_EQ:    /*printf("%d * %d = %d\n", val1, val2, val1==val2)*/;return val1 == val2;
      case TK_PLUS:  /*printf("%d + %d = %d\n", val1, val2, val1+val2)*/; return val1 + val2;
      case TK_SUB:   /*printf("%d - %d = %d\n", val1, val2, val1-val2)*/; return val1 - val2;
      case TK_MULTI: /*printf("%d * %d = %d\n", val1, val2, val1*val2)*/; return val1 * val2;
      case TK_DIV:   /*printf("%d / %d = %d\n", val1, val2, val1/val2)*/; 
        if(val2 == 0){
          *success = false;
          printf("Division by zero\n");
          return 0;
        }
        return val1 / val2;
      default: assert(0);
    }
  }
  return 0;
}

Token* get_operation_main(Token *p, Token *q)
{
  Token *index = NULL;
  Token *pos = NULL;
  int LP = 0;
  
  /* 先取"==", 且“==”不在括号里 */
  for(index = p; index < q; index++)
  {
    if(index->type == TK_LP)
    {
      LP++;
      while(LP)
      {
        index++;
        if(index->type == TK_LP)
          LP++;
        else if(index->type == TK_RP)
          LP--;
      }
    }
    else if(index->type == TK_EQ)
    {
      pos = index;
      return pos;
    }
  }

  /* 再取其他符号,且符号不在括号里 */
  LP = 0;
  for(index = p; index < q; index++)
  {
    if(index->type == TK_LP)
    {
      LP++;
      while(LP)
      {
        index++;
        if(index->type == TK_LP)
          LP++;
        else if(index->type == TK_RP)
          LP--;
      }
    }
    else if(index->type == TK_MULTI || index->type == TK_DIV)
    {
      if(!pos || (pos && (pos->type == TK_MULTI || pos->type == TK_DIV)))
        pos = index;
    }
    else if(index->type == TK_SUB || index->type == TK_PLUS)
    {
      pos = index;
    }
  }

  return pos;
}

enum EXPR_TYPE check_parentheses(Token *p, Token *q)
{
  Token *index = NULL;
  int LP = 0;

  /* "(4 + 3)) * ((2 - 1)" false, bad expression */
  for(index = p; index <= q; index++)
  {
    if(index->type == TK_LP)
      LP++;
    else if(index->type == TK_RP)
      LP--;
  }
  if(LP < 0 || LP > 0)
    return BAD;

  /* "(4 + 3) * (2 - 1)"   
       false, the leftmost '(' and the rightmost ')' are not matched
  */
  LP = 0;
  for(index = p; index <= q; index++)
  {
    if(index->type == TK_LP)
      LP++;
    else if(index->type == TK_RP)
      LP--;

    if(LP == 0 && index > p && index < q)
      return NO_MATCH;
  }

  if(p->type != TK_LP || q->type != TK_RP)
    return NO_SURROUNDED;

  return SUCCESS;
}