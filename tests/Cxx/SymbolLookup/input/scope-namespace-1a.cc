char k;

namespace N 
{
  int i;
  int g(int a) { return a; }
  int j();
  void q();
}
namespace { int l=1; }
// the potential scope of l is from its point of declaration
// to the end of the translation unit
namespace N 
{
  int g(char a)         // overloads N::g(int)
  {
    return k+a;   // k is from unnamed namespace
  }
  //  int i;                // error: duplicate definition
  int j();              // ok: duplicate function declaration
  int j()               // ok: definition of N::j()
  {
    return g(i);  // calls N::g(int)
  }
  //  int q();              // error: different return type
}
