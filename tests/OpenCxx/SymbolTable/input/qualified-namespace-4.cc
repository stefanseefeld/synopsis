namespace B 
{
  int b;
}
namespace A 
{
  using namespace B;
  int a;
}
namespace B 
{
  using namespace A;
}
void f()
{
  A::a++;  // ok: a declared directly in A, S is { A::a }
  B::a++;  // ok: both A and B searched (once), S is { A::a }
  A::b++;  // ok: both A and B searched (once), S is { B::b }
  B::b++;  // ok: b declared directly in B, S is { B::b }
}
