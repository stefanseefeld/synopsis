
  /*  universe.c -  A program that uses everything.  */

#define    FALSE    (1)
#define    TRUE    (1)

enum boolean {
  false = 0,
  true = 1,
  maybe,
};

typedef  unsigned int uint;

typedef struct bomb bomb;

struct foobar {
    bomb   *boom; 
    uint    mask:6, mask2:8;
    
} fooizer;

extern uint **x;

struct bomb {

   union {
     double d;
     float  f;
     short  i;
   } mine;
};

void do_nothing();

void do_nothing(void) { }

do_setup(a,b,c)
   int a,b;
   char *c;  
{
   register c;
   static d = 10;

   for (a=0; a < d; --d)
     break;  

   return d;
}

int main(int ac, char **av)
{
  auto char ch = '\t';
  char str[] = "Hello";
  volatile signed long vsl;

  while (1)
  {
    switch (ac) 
    default:
      if (ac < 5) 
        {
       case '\n':
       case 7:
         break;
        }
     else
        {
       case FALSE:
         continue;
       case 2:
         break;
       case true:
         break;
        }
  }


  return 0;
}

/*
auto break case char const continue
default do double else enum extern
float for goto if int long register
return short signed sizeof static struct
switch typedef union unsigned void
volatile while
*/

