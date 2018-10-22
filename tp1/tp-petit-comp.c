/* fichier: "petit-comp-print.c" */

/* Un petit compilateur et machine virtuelle pour un sous-ensemble de C.  */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

/*---------------------------------------------------------------------------*/

/* Analyseur lexical. */

enum { DO_SYM, ELSE_SYM, IF_SYM, WHILE_SYM, LBRA, RBRA, LPAR,
       PRINT_SYM, RPAR, PLUS, MINUS, LESS, SEMI, EQUAL, INT, ID, EOI,
       LESS_EQUAL, GREAT, GREAT_EQUAL, DOUBLE_EQUAL, NOT,
       MULT, MODULO};

char *words[] = { "do", "else", "if", "while", "print", NULL };

int ch = ' ';
int sym;
int int_val;
char id_name[100];

void syntax_error() { fprintf(stderr, "syntax error\n"); exit(1); }

void next_ch() { ch = getchar(); }

// DEBUG PRINTS
void printch() { printf("%c\n", ch);}
void print_int_val() { printf("%d\n", int_val);}

void next_sym()
{
  while (ch == ' ' || ch == '\n') next_ch();
  printch();
  switch (ch)
    { case '{': sym = LBRA;  next_ch(); printch();break;
      case '}': sym = RBRA;  next_ch(); printch();break;
      case '(': sym = LPAR;  next_ch(); printch();break;
      case ')': sym = RPAR;  next_ch(); printch();break;
      case '+': sym = PLUS;  next_ch(); printch();break;
      case '-': sym = MINUS; next_ch(); printch(); break;
      case '<': sym = LESS;  next_ch(); printch();break;
      case ';': sym = SEMI;  next_ch(); printch();break;
      case '=': sym = EQUAL; next_ch(); printch();break;
      case EOF: sym = EOI;   next_ch(); printch();break;
      default:
      // ASSIGNATION DE LA VALEUR DE LA VARIABLE
        if (ch >= '0' && ch <= '9')
          {
            int_val = 0; /* overflow? */
      
            while (ch >= '0' && ch <= '9')
              {
                int_val = int_val*10 + (ch - '0');
                next_ch();
              }
            print_int_val();
            sym = INT;
          }

        // ASSIGNATION DU NOM DE LA VARIABLE
        else if (ch >= 'a' && ch <= 'z')
          {
            int i = 0; /* overflow? */
      
            while ((ch >= 'a' && ch <= 'z') || ch == '_')
              {
                id_name[i++] = ch;
                next_ch();
              }
      
            id_name[i] = '\0';
            sym = 0;

            // COMPARE AVEC LES MOTS EXCLISIF AU LANGAGE
            while (words[sym]!=NULL && strcmp(words[sym], id_name)!=0)
              sym++;
            
            // NE LAISSE PAS AVOIR DES VARIABLE DE PLUS DE UNE LETTRE!
            if (words[sym] == NULL)
              {
                if (id_name[1] == '\0') sym = ID; else syntax_error();
              }
          }
        else syntax_error();
    }
}

/*---------------------------------------------------------------------------*/

/* Analyseur syntaxique. */

enum { VAR, CST, ADD, SUB, LT, ASSIGN,
       IF1, IF2, WHILE, DO, PRINT, EMPTY, SEQ, EXPR, PROG };

struct node
  {
    int kind;
    struct node *o1;
    struct node *o2;
    struct node *o3;
    int val;
  };

typedef struct node node;

node *new_node(int k)
{
  node *x = malloc(sizeof(node));
  x->kind = k;
  return x;
}

node *paren_expr(); /* forward declaration */

node *term() /* <term> ::= <id> | <int> | <paren_expr> */
{
  node *x;

  if (sym == ID)           /* <term> ::= <id> */
    {
      x = new_node(VAR);
      x->val = id_name[0]-'a';
      next_sym();
    }
  else if (sym == INT)     /* <term> ::= <int> */
    {
      x = new_node(CST);
      x->val = int_val;
      next_sym();
    }
  else                     /* <term> ::= <paren_expr> */
    x = paren_expr();

  return x;
}

node *sum() /* <sum> ::= <term>|<sum>"+"<term>|<sum>"-"<term> */
{
  node *x = term();

  while (sym == PLUS || sym == MINUS)
    {
      node *t = x;
      x = new_node(sym==PLUS ? ADD : SUB);
      next_sym();
      x->o1 = t;
      x->o2 = term();
    }

  return x;
}

node *test() /* <test> ::= <sum> | <sum> "<" <sum> */
{
  node *x = sum();

  if (sym == LESS)
    {
      node *t = x;
      x = new_node(LT);
      next_sym();
      x->o1 = t;
      x->o2 = sum();
    }

  return x;
}

node *expr() /* <expr> ::= <test> | <id> "=" <expr> */
{
  node *x;

  if (sym != ID) return test();

  x = test();

  if (sym == EQUAL)
    {
      node *t = x;
      x = new_node(ASSIGN);
      next_sym();
      x->o1 = t;
      x->o2 = expr();
    }

  return x;
}

node *paren_expr() /* <paren_expr> ::= "(" <expr> ")" */
{
  node *x;

  if (sym == LPAR) next_sym(); else syntax_error();

  x = expr();

  if (sym == RPAR) next_sym(); else syntax_error();

  return x;
}

node *statement()
{
  node *x;

  if (sym == IF_SYM)       /* "if" <paren_expr> <stat> */
    {
      x = new_node(IF1);
      next_sym();
      x->o1 = paren_expr();
      x->o2 = statement();
      if (sym == ELSE_SYM) /* ... "else" <stat> */
        { x->kind = IF2;
          next_sym();
          x->o3 = statement();
        }
    }
  else if (sym == WHILE_SYM) /* "while" <paren_expr> <stat> */
    {
      x = new_node(WHILE);
      next_sym();
      x->o1 = paren_expr();
      x->o2 = statement();
    }
  else if (sym == DO_SYM)  /* "do" <stat> "while" <paren_expr> ";" */
    {
      x = new_node(DO);
      next_sym();
      x->o1 = statement();
      if (sym == WHILE_SYM) next_sym(); else syntax_error();
      x->o2 = paren_expr();
      if (sym == SEMI) next_sym(); else syntax_error();
    }
  else if (sym == PRINT_SYM)  /* "print" <paren_expr> ";" */
    {
      x = new_node(PRINT);
      next_sym();
      x->o1 = paren_expr();
      if (sym == SEMI) next_sym(); else syntax_error();
    }
  else if (sym == SEMI)    /* ";" */
    {
      x = new_node(EMPTY);
      next_sym();
    }
  else if (sym == LBRA)    /* "{" { <stat> } "}" */
    {
      x = new_node(EMPTY);
      next_sym();
      while (sym != RBRA)
        {
          node *t = x;
          x = new_node(SEQ);
          x->o1 = t;
          x->o2 = statement();
        }
      next_sym();
    }
  else                     /* <expr> ";" */
    {
      x = new_node(EXPR);
      x->o1 = expr();
      if (sym == SEMI) next_sym(); else syntax_error();
    }

  return x;
}

node *program()  /* <program> ::= <stat> */
{
  node *x = new_node(PROG);
  next_sym();
  x->o1 = statement();
  if (sym != EOI) syntax_error();
  return x;
}

/*---------------------------------------------------------------------------*/

/* Generateur de code. */

enum { ILOAD, ISTORE, BIPUSH, DUP, POP, IADD, ISUB, IPRINT,
       GOTO, IFEQ, IFNE, IFLT, RETURN };

typedef signed char code;

code object[1000], *here = object;

void gen(code c) { *here++ = c; } /* overflow? */

#ifdef SHOW_CODE
#define g(c) do { printf(" %d",c); gen(c); } while (0)
#define gi(c) do { printf("\n%s", #c); gen(c); } while (0)
#else
#define g(c) gen(c)
#define gi(c) gen(c)
#endif

void fix(code *src, code *dst) { *src = dst-src; } /* overflow? */

void c(node *x)
{ switch (x->kind)
    { case VAR   : gi(ILOAD); g(x->val); break;

      case CST   : gi(BIPUSH); g(x->val); break;

      case ADD   : c(x->o1); c(x->o2); gi(IADD); break;

      case SUB   : c(x->o1); c(x->o2); gi(ISUB); break;

      case LT    : gi(BIPUSH); g(1);
                   c(x->o1);
                   c(x->o2);
                   gi(ISUB);
                   gi(IFLT); g(4);
                   gi(POP);
                   gi(BIPUSH); g(0); break;

      case ASSIGN: c(x->o2);
                   gi(DUP);
                   gi(ISTORE); g(x->o1->val); break;

      case IF1   : { code *p1;
                     c(x->o1);
                     gi(IFEQ); p1 = here++;
                     c(x->o2); fix(p1,here); break;
                   }

      case IF2   : { code *p1, *p2;
                     c(x->o1);
                     gi(IFEQ); p1 = here++;
                     c(x->o2);
                     gi(GOTO); p2 = here++; fix(p1,here);
                     c(x->o3); fix(p2,here); break;
                   }

      case WHILE : { code *p1 = here, *p2;
                     c(x->o1);
                     gi(IFEQ); p2 = here++;
                     c(x->o2);
                     gi(GOTO); fix(here++,p1); fix(p2,here); break;
                   }

      case DO    : { code *p1 = here; c(x->o1);
                     c(x->o2);
                     gi(IFNE); fix(here++,p1); break;
                   }

      case PRINT : { c(x->o1);
                     gi(IPRINT); break;
                   }

      case EMPTY : break;

      case SEQ   : c(x->o1);
                   c(x->o2); break;

      case EXPR  : c(x->o1);
                   gi(POP); break;

      case PROG  : c(x->o1);
                   gi(RETURN); break;
    }
}

/*---------------------------------------------------------------------------*/

/* Machine virtuelle. */

int globals[26];

void run()
{
  int stack[1000], *sp = stack; /* overflow? */
  code *pc = object;

  for (;;)
    switch (*pc++)
      {
        case ILOAD : *sp++ = globals[*pc++];             break;
        case ISTORE: globals[*pc++] = *--sp;             break;
        case BIPUSH: *sp++ = *pc++;                      break;
        case DUP   : sp++; sp[-1] = sp[-2];              break;
        case POP   : --sp;                               break;
        case IADD  : sp[-2] = sp[-2] + sp[-1]; --sp;     break;
        case ISUB  : sp[-2] = sp[-2] - sp[-1]; --sp;     break;
        case IPRINT: printf("%d\n", *--sp);              break;
        case GOTO  : pc += *pc;                          break;
        case IFEQ  : if (*--sp==0) pc += *pc; else pc++; break;
        case IFNE  : if (*--sp!=0) pc += *pc; else pc++; break;
        case IFLT  : if (*--sp< 0) pc += *pc; else pc++; break;
        case RETURN: return;
    }
}

/*---------------------------------------------------------------------------*/

/* Programme principal. */

int main()
{
  int i;

  c(program());
#ifdef SHOW_CODE
  printf("\n");
#endif

  for (i=0; i<26; i++)
    globals[i] = 0;

  run();


  for (i=0; i<26; i++)
    if (globals[i] != 0)
      printf("%c = %d\n", 'a'+i, globals[i]);


  return 0;
}

/*---------------------------------------------------------------------------*/