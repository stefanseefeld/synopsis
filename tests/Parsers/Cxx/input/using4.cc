// From C++WD'96 7.4.3.6 Example
namespace D
{
  int d1;
  void f(char);
}

using namespace D;
int d1; // ok, no conflict with D::d1
namespace E
{
  int e;
  void f(int);
}

namespace D  // namespace extension
{
  int d2;
  using namespace E;
  void f(int);
}

void f()
{
  d1++;    // error: ambiguous: ::d1 or D::d1
  ::d1++;  // ok
  D::d1++; // ok
  d2++;    // ok D::d2
  e++;     // ok E::e
  f(1);    // error: ambiguous: D::f(int) or E::f(int)
  f('a');  // ok: D::f(char)
}
