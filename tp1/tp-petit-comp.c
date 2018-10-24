/* fichier: "petit-comp-print.c" */

/* Un petit compilateur et machine virtuelle pour un sous-ensemble de C.  */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


/*
Tous allocation de memoire sera ajouter a un node qui fera parti de une liste
Quand on veut terminer l`execution  on passera par tous les nodes
de la liste et on `free` tous les elements
*/
typedef struct cleaning_node_t {
	int val;
	struct cleaning_node_t * next;
} cleaning_node_t;

cleaning_node_t * head = NULL;

/*
On utiliserai cette fonction au lieu de malloc
La seul difference est que on conserve un pointeur
vers l`element que on a cree et que on check pour des erreur avec malloc
*/
void* customMemAllo(long size)
{

	void *p;
	if (((p = (void *)malloc(size)) == NULL))
	{
		//TODO: Throw an exeption and free all memory and shut down program
	}


	push(head, p);

	return p;
}
/*---------------------------------------------------------------------------*/

/* Analyseur lexical. */

enum { DO_SYM, ELSE_SYM, IF_SYM, WHILE_SYM, LBRA, RBRA, LPAR,
       PRINT_SYM, RPAR, PLUS, MINUS, LESS, SEMI, EQUAL, INT, ID, EOI,
       LESS_EQUAL, GREAT, GREAT_EQUAL, DOUBLE_EQUAL, NOT_EQUAL,
       MULT, DIV, MOD, TAG};

char *words[] = { "do", "else", "if", "while", "print", NULL };

int ch = ' ';
int sym;
int int_val;
char id_name[100];
//List to keep track of names of tags
char tag_name[256];
//List to keep track of of tags that are inside loops
char loop_tag_name[256];

void syntax_error() { fprintf(stderr, "syntax error\n"); exit(1); }

void next_ch() { ch = getchar(); }

// DEBUG PRINTS
void print_ch() { printf("%c\n", ch);}
void print_int_val() { printf("%d\n", int_val);}

void next_sym()
{
  while (ch == ' ' || ch == '\n') next_ch();
  switch (ch)
    { case '{': sym = LBRA;  next_ch(); break;
      case '}': sym = RBRA;  next_ch(); break;
      case '(': sym = LPAR;  next_ch(); break;
      case ')': sym = RPAR;  next_ch(); break;
      case '+': sym = PLUS;  next_ch(); break;
      case '-': sym = MINUS; next_ch(); break;
      case ';': sym = SEMI;  next_ch(); break;
      case '*': sym = MULT;  next_ch(); break;
      case '/': sym = DIV;   next_ch(); break;
      case '%': sym = MOD;   next_ch(); break;
      case ':': sym = TAG;   next_ch(); break;

      case '<':
        next_ch();
        if(ch == '='){
          sym = LESS_EQUAL;
          next_ch();
          break;
        }
        else{
          sym = LESS;
          break;
        }

      case '>':
        next_ch();
        if(ch == '='){
          sym = GREAT_EQUAL;
          next_ch();
          break;
        }
        else{
          sym = GREAT;
          break;
        }

      case '=':
        next_ch();
        if(ch == '='){
          sym = DOUBLE_EQUAL;
          next_ch();
          break;
        }
        else{
          sym = EQUAL;
          break;
        }

      case '!':
        next_ch();
        if(ch == '='){
          sym = NOT_EQUAL;
          next_ch();
          break;
        }
        else{
          // TODO CHANGE ERROR LOG!!!
          syntax_error();
        }

      case EOF: sym = EOI;   next_ch(); break;
      default:
        if (ch >= '0' && ch <= '9')
          {
            int_val = 0; /* overflow? */

            while (ch >= '0' && ch <= '9')
              {
                int_val = int_val*10 + (ch - '0');
                next_ch();
              }
            sym = INT;
          }

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

            while (words[sym]!=NULL && strcmp(words[sym], id_name)!=0)
              sym++;

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

/* TODO, ajouter dans le enum les commandes manquantes */

enum { VAR, CST, ADD, SUB, LT, LEQ, GT, EQ, NEQ, GEQ, ASSIGN,
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
  node *x = customMemAllo(sizeof(node));
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
  /* <sum> */
  node *x = sum();

  /* <sum> "<" <sum> */
  if (sym == LESS)
    {
      node *t = x;
      x = new_node(LT);
      next_sym();
      x->o1 = t;
      x->o2 = sum();
    }

    /* <sum> "<=" <sum> */
    else if(sym == LESS_EQUAL){
      node *t = x;
      x = new_node(LEQ);
      next_sym();
      x->o1 = t;
      x->o2 = sum();
    }

    /* <sum> ">" <sum> */
    else if(sym == GREAT){
      node *t = x;
      x = new_node(GT);
      next_sym();
      x->o1 = t;
      x->o2 = sum();
    }

    /* <sum> ">=" <sum> */
    else if(sym == GREAT_EQUAL){
      node *t = x;
      x = new_node(GEQ);
      next_sym();
      x->o1 = t;
      x->o2 = sum();
    }

    /* <sum> "==" <sum> */
    else if(sym == DOUBLE_EQUAL){
      node *t = x;
      x = new_node(EQ);
      next_sym();
      x->o1 = t;
      x->o2 = sum();
    }

    /* <sum> "!=" <sum> */
    else if(sym == NOT_EQUAL){
      node *t = x;
      x = new_node(NEQ);
      next_sym();
      x->o1 = t;
      x->o2 = sum();
    }

    else if(sym == TAG)
    {

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



//Push a node onto a liste
void push(cleaning_node_t * head, int val)
{
	cleaning_node_t * current = head;
	while (current->next != NULL) {
		current = current->next;
	}

	/* now we can add a new variable */
	current->next = head;
	current->next->val = val;
	current->next->next = NULL;
}

//elimine le dernier element d`une liste chainer
int remove_last(cleaning_node_t * head)
{
	int retval = 0;
	/* if there is only one item in the list, remove it */
	if (head->next == NULL) {
		retval = head->val;
		free(head);
		return retval;
	}

	/* get to the second to last node in the list */
	cleaning_node_t * current = head;
	while (current->next->next != NULL) {
		current = current->next;
	}

	/* now current points to the second to last item of the list, so let's remove current->next */
	retval = current->next->val;
	free(current->next);
	current->next = NULL;
	return retval;
}

//Une loop qui va free tous les nodes que on a cree et donc free tous la memoire
//en commencer avec le dernier element dans la liste
void freeAllMemory(cleaning_node_t * head)
{
	while (head->next != NULL)
	{
		remove_last(head);
	}
}


/*---------------------------------------------------------------------------*/

/* Generateur de code. */
/* TODO create new instructions for the LEQ, GTE, NOT, MULT, DIV, MOD*/

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

      case LEQ   : gi(BIPUSH); g(1);
                   c(x->o1);
                   c(x->o2);
                   gi(ISUB);
                   gi(IFEQ); g(11);
                   c(x->o1);
                   c(x->o2);
                   gi(ISUB);
                   gi(IFLT); g(4);
                   gi(POP);
                   gi(BIPUSH); g(0); break;

      case GT    : gi(BIPUSH); g(0);
                   c(x->o1);
                   c(x->o2);
                   gi(ISUB);
                   gi(IFEQ); g(11);
                   c(x->o1);
                   c(x->o2);
                   gi(ISUB);
                   gi(IFLT); g(4);
                   gi(POP);
                   gi(BIPUSH); g(1); break;

      case GEQ   : gi(BIPUSH); g(0);
                   c(x->o1);
                   c(x->o2);
                   gi(ISUB);
                   gi(IFLT); g(4);
                   gi(POP);
                   gi(BIPUSH); g(1); break;

      case EQ   : gi(BIPUSH); g(1);
                   c(x->o1);
                   c(x->o2);
                   gi(ISUB);
                   gi(IFEQ); g(4);
                   gi(POP);
                   gi(BIPUSH); g(0); break;

      case NEQ   : gi(BIPUSH); g(1);
                   c(x->o1);
                   c(x->o2);
                   gi(ISUB);
                   gi(IFNE); g(4);
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
        case GOTO  :
          if((pc > *pc) && (pc > (*pc+256)))
          {
            //TODO throw error
          }
          else if((pc < *pc) && (pc < (*pc-256)))
          {
            //TODO throw error
          }
        pc += *pc;
        break;
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
  head = malloc(sizeof(1));
	if (head == NULL)
	{
    //Error code 1 for memory error
		return 1;
	}
	head->val = NULL;
	head->next = NULL;


  int i;

  node *asa_nodes = program();
  c(asa_nodes);

#ifdef SHOW_CODE
  printf("\n");
#endif

  for (i=0; i<26; i++)
    globals[i] = 0;

  run();


  for (i=0; i<26; i++)
    if (globals[i] != 0)
      printf("%c = %d\n", 'a'+i, globals[i]);


  //TODO clear asa_nodes



  return 0;
}

/*---------------------------------------------------------------------------*/
