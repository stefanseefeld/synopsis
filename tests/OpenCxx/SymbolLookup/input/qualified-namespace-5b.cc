namespace A 
{
  struct x { };
  int x;
  int y;
}
namespace B 
{
  struct y {};
}
namespace C 
{
  using namespace A;
  using namespace B;
  int i = C::x; // ok, A::x (of type 'int')
  int j = C::y; // ambiguous, A::y or B::y
}

