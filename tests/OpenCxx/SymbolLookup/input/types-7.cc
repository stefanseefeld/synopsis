class X;             // X is an incomplete type
extern X* xp;        // xp is a pointer to an incomplete type
extern int arr[];    // the type of arr is incomplete
typedef int UNKA[];  // UNKA is an incomplete type
UNKA* arrp;          // arrp is a pointer to an incomplete type
UNKA** arrpp;
void foo()
{
  xp++;              // ill-formed:  X is incomplete
  arrp++;            // ill-formed:  incomplete type
  arrpp++;           // okay: sizeof UNKA* is known
}
struct X { int i; }; // now X is a complete type
int  arr[10];        // now the type of arr is complete
X x;
void bar()
{
  xp = &x;           // okay; type is ``pointer to X''
  arrp = &arr;       // ill-formed: different types
  xp++;              // okay:  X is complete
  arrp++;            // ill-formed:  UNKA can't be completed
}
