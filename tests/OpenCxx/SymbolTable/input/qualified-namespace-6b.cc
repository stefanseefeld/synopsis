namespace A 
{
  namespace B 
  {
    void f1(int);
  }
}
namespace C 
{
  namespace D 
  {
    void f1(int);
  }
}
using namespace A;
using namespace C::D;
void B::f1(int){}  // okay, defines A::B::f1(int)
