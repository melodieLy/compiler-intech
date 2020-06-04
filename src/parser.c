#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include "symbol.h"
#include "buffer.h"
#include "parser.h"
#include "ast.h"
#include "utils.h"
#include "stack.h"
#include "lexer.h"

#define DEBUG true

extern symbol_t **pglobal_table;
extern ast_t **past;

int parse_var_type (buffer_t *buffer)
{
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum(buffer);
  if (strcmp(lexem, "entier") == 0) {
    return AST_INTEGER;
  }
  printf("Expected a valid type. exiting.\n");
  exit(1);
}

/**
 * 
 * (entier a, entier b, entier c) => une liste d'ast_t contenue dans un ast_list_t
 */
ast_list_t *parse_parameters (buffer_t *buffer)
{
  ast_list_t *list = NULL;
  buf_skipblank(buffer);
  lexer_assert_openbrace(buffer, "Expected a '(' after function name. exiting.\n");
  
  for (;;) {
    int type = parse_var_type(buffer);

    buf_skipblank(buffer);
    char *var_name = lexer_getalphanum(buffer);
    buf_skipblank(buffer);
    
    ast_list_add(&list, ast_new_variable(var_name, type));

    char next = buf_getchar(buffer);
    if (next == ')') break;
    if (next != ',') {
      printf("Expected either a ',' or a ')'. exiting.\n");
      exit(1);
    }
  }
  return list;
}

int parse_return_type (buffer_t *buffer)
{
  buf_skipblank(buffer);
  lexer_assert_twopoints(buffer, "Expected ':' after function parameters");
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum(buffer);
  if (strcmp(lexem, "entier") == 0) {
    return AST_INTEGER;
  }
  else if (strcmp(lexem, "rien") == 0) {
    return AST_VOID;
  }
  printf("Expected a valid type for a parameter. exiting.\n");
  exit(1);
}

bool parse_is_type (char *lexem)
{
  if (strcmp(lexem, "entier") != 0) { // si le mot-clé n'est pas "entier", on retourne faux
    return false;
  }
  return true;
}

bool stack_is_smaller (void *data, char c) {
  char cStack = *(char *)data;
  char t[1] = {'\0'};
  if(strcmp(data,t) == 0) return true;
  if(isnbr(cStack)) return false;
  if(cStack == '+' || cStack == '-') {
    if(c == '+' || c == '-') return false;
    else return true;
  } else if (cStack == '*' || cStack == '/') {
     if(isnbr(c)) return true;
     return false;
  }
  return false;
}

bool isop (char chr) 
{
  return chr == '+' || chr == '-' || chr == '/' || chr == '*';
}

// ast_t *type_ast (void *c) {
//   if(isnbr(*(char*)c)) return ast_new_integer(*(long*)c);
//   if(isop(*(char*)c)) {
//     if(*(char*)c == "+") return ast_new_unary(*(char*)c, ast)
//   }
// }

ast_t *parse_expression (buffer_t *buffer)
{
  // TODO :
  /**
   * Push la valeur dans la pile
   * Retirer le top de la pile
   * l'affecter a sortie
   * comparer avec sortie et les valeur suivantes (voir algo !)
   * Puis ensuite, transformer ce qui est dans la "sortie" en arbre.
   * */
  char start = '\0';
  void *data = &start;
  mystack_t stack = NULL;
  stack_push(&stack,data);
  mystack_t exit = NULL;
  buf_skipblank(buffer);
  char c = buf_getchar(buffer);
  for (;;) {
    data = stack_top(stack);
    printf("%c VS %c\n",*(char*)data,c);
    buf_print(buffer);
    if(stack_is_smaller(data,c)) {
      char temp = c;
      stack_push(&stack, &temp);
      printf("Add element : '%c' on stack\n", *(char*)stack_top(stack));
      buf_skipblank(buffer);
      c = buf_getchar(buffer);
    } else {
      printf("false\n");
      char *temp = stack_pop(&stack);
      printf("add '%c' on exit\n",*temp);
      stack_push(&exit, temp);
      //data = stack_top(stack);
      printf("%c VS %c\n",*(char*)data, *(char*)stack_top(exit));
      
      // while(!stack_is_smaller(data,*(char*)stack_top(exit))) {
      //   temp = stack_pop(&stack);
      //   stack_push(&exit,temp);
      //   printf("add '%c' on exit\n",*temp);
      //   data = stack_top(stack);
      //   printf("%c VS %c\n",*(char*)data,c);
      // }
      printf("end of comparaison\n");
    }
    if(c == ';') {
      while(stack_count(exit) >= 0) {
        printf("count : %d\n", stack_count(exit));
        char *temp = stack_pop(&exit);
        printf("exit remove : %c\n", *temp);
        
      }
    }
  }
      
  return NULL;
}

ast_t *parse_declaration (buffer_t *buffer)
{
  int type = parse_var_type(buffer);
  buf_skipblank(buffer);
  char *var_name = lexer_getalphanum(buffer);
  if (var_name == NULL) {
    printf("Expected a variable name. exiting.\n");
    exit(1);
  }

  ast_t *var = ast_new_variable(var_name, type);
  buf_skipblank(buffer);
  char next = buf_getchar(buffer);
  if (next == ';') {
    
    return ast_new_declaration(var, NULL);
  }
  else if (next == '=') {
    ast_t *expression = parse_expression(buffer);
    return ast_new_declaration(var, expression);
  }
  printf("Expected either a ';' or a '='. exiting.\n");
  buf_print(buffer);
  exit(1);
}

ast_t *parse_statement (buffer_t *buffer)
{
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum_rollback(buffer);
  if (parse_is_type(lexem)) {
    // ceci est une déclaration de variable
    return parse_declaration(buffer);
  }
  // TODO:
  return NULL;
}

ast_list_t *parse_function_body (buffer_t *buffer)
{
  ast_list_t *stmts = NULL;
  buf_skipblank(buffer);
  lexer_assert_openbracket(buffer, "Function body should start with a '{'");
  char next;
  do {
    ast_t *statement = parse_statement(buffer);
    ast_list_add(&stmts, statement);
    ast_t *exp = parse_expression(buffer);
    //ast_list_new_node(exp);
    buf_skipblank(buffer);
    next = buf_getchar_rollback(buffer);
  } while (next != '}');

  return stmts;
}


ast_t *parse_function (buffer_t *buffer)
{
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum(buffer);
  if (strcmp(lexem, "fonction") != 0) {
    printf("Expected a 'fonction' keyword on global scope.exiting.\n");
    buf_print(buffer);
    exit(1);
  }
  buf_skipblank(buffer);
  // TODO
  char *name = lexer_getalphanum(buffer);
  
  ast_list_t *params = parse_parameters(buffer);
  int return_type = parse_return_type(buffer);
  ast_list_t *stmts = parse_function_body(buffer);
  return ast_new_function(name, return_type, params, stmts);
}

/**
 * This function generates ASTs for each global-scope function
 */
ast_list_t *parse (buffer_t *buffer)
{
  ast_t *function = parse_function(buffer);
  ast_print(function);
  if (DEBUG) printf("** end of file. **\n");
  return NULL;
}
