namespace M 
{
  class B { };
}

namespace N 
{
  class Y : public M::B 
  {
    class X 
    {
      int a[i];
    };
  };
}
// The following scopes are searched for a declaration of i:
// 1) scope of class N::Y::X, before the use of i
// 2) scope of class N::Y, before the definition of N::Y::X
// 3) scope of N::Y's base class M::B
// 4) scope of namespace N, before the definition of N::Y
// 5) global scope, before the definition of N

