class B {};
namespace M 
{
  namespace N 
  {
    int i;
    class X : public B 
    {
      void f();
    };
  }
}
void M::N::X::f() 
{
  i = 16;
}
// The following scopes are searched for a declaration of i:
// 1) outermost block scope of M::N::X::f, before the use of i
// 2) scope of class M::N::X
// 3) scope of M::N::X's base class B
// 4) scope of namespace M::N
// 5) scope of namespace M
// 6) global scope, before the definition of M::N::X::f
