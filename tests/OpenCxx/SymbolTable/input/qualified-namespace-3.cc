namespace A 
{
  int a;
}
namespace B 
{
  using namespace A;
}
namespace C 
{
  using namespace A;
}
namespace BC 
{
  using namespace B;
  using namespace C;
}
void f()
{
  BC::a++;  // ok: S is { A::a, A::a }
}
namespace D 
{
  using A::a;
}
namespace BD 
{
  using namespace B;
  using namespace D;
}

void g()
{
  BD::a++;  // ok: S is { A::a, A::a }
}
