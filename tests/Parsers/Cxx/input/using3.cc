// From C++WD'96 7.4.3.3 Example 2
namespace A
{
  int i;
}

namespace B
{
  int i;
  int j;
  namespace C
  {
    namespace D
    {
      using namespace A;
      int j;
      int k;
      int a = i; // B::i hides A::i
    }
    using namespace D;
    int k = 89; // no problem yet
    int l = k; // ambiguous: C::k or D::k
    int m = i; // B::i hides A::i
    int n = j; // D::j hides B::j
  }
}
