struct X                     // defines X
{
  int x;                     // defines nonstatic data member x
  static int y;              // declares static data member y
  X(): x(0) { }              // defines a constructor of X
};
namespace N { int d; }       // defines N and N::d    
/************************************************************************
  Definitons above from 3.1.2-1 are necessary and sufficient for the
  proper compilation of code below.
************************************************************************/

extern int a;                // declares a
extern const int c;          // declares c
int f(int);                  // declares f
struct S;                    // declares S
typedef int Int;             // declares Int
extern X anotherX;           // declares anotherX
using N::d;                  // declares N::d

