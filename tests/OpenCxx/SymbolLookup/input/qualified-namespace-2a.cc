int x;
namespace Y 
{
  void f(float) {}
  void h(int) {}
}
namespace Z 
{
  void h(double) {}
}
namespace A 
{
  using namespace Y;
  void f(int) {}
  void g(int) {}
  int i;
}
namespace B 
{
  using namespace Z;
  void f(char) {}
  int i;
}
namespace AB 
{
  using namespace A;
  using namespace B;
  void g() {}
}

void h()
{
  AB::g();     // g is declared directly in AB,
               // therefore S is { AB::g() } and AB::g() is chos en
  AB::f(1);    // f is not declared directly in AB so the rules are
               // applied recursively to A and B;
               // namespace Y is not searched and Y::f(float)
               // is not considered;
               // S is { A::f(int), B::f(char) } and overload
               // resolution chooses A::f(int)
  AB::f('c');  // as above but resolution chooses B::f(char)

  // AB::x++;  // x is not declared directly in AB, and
               // is not declared in A or B, so the rules are
               // applied recursively to Y and Z,
               // S is { } so the program is ill-formed
  // AB::i++;  // i is not declared directly in AB so the rules are
               // applied recursively to A and B,
               // S is { A::i, B::i } so the use is ambiguous
               // and the program is ill-formed
  AB::h(16.8); // h is not declared directly in AB and
               // not declared directly in A or B so the rules are
               // applied recursively to Y and Z,
               // S is { Y::h(int), Z::h(double) } and overload
               // resolution chooses Z::h(double)
}
