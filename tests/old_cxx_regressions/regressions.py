## regressions.py
## Copyright (c) 2002 by Stephen Davies
##
## This file contains the regression tests for the C++ parser
## Each one is in it's own class.
## The tests are threefold:
## 1. That the test parses w/o errors
## 2. Testing of name resolution using link tests
##    eg; x = i; // test i = "::i"
##    will check that the generated i is linked to the global i variable
## 3. Testing of AST structure, using separate list of checks
##    eg: class Foo has method "instance" returns "Foo*" params ()
##    will check that there is a class Foo, with a method called instance
##    that returns a Foo* and has no parameters

from test import *

class Comments (Regression, Test):
  test = """// 1. Comments Test: One line cpp test

// 2. Two line cpp test
// Second line of test

/* 3. One line c test */
/* 4. Another c test */
/* 5. Two on one */ /* 6. line */
/* 7. Multi
 * line
 */
"""

class IfTest (Regression, Test):
  test = """// If test
void func() {
  int x = 0;
  if (x) cout << "Hi";           // test x = "`func()::x"
  if (x == 2) cout << "hi";      // test x = "`func()::x"
  if (!x) cout << "foo";         // test x = "`func()::x"
  if (x) cout << "one"; else cout << "two";
  if (x) {cout << "one"; cout << "two"; } else { cout<<"three"; }
}
"""

class ForTest (Regression, Test):
  test = """
void func() {
  int x;
  for (int x = 3, y=10; x < y; x++) {
    cout << x;
  }
}
"""

class UsingTest (Regression, Test):
  test = """
namespace Foo {
  int x;
}
void func() {
  using namespace Foo;
  x;
}
void func2() {
  using namespace Foo = Bar;
  Bar::x;
}
void func3() {
  Foo::x;
}
void func4() {
  using Foo::x;
  x;
}
"""

class UsingTest2 (Regression, Test):
  test = """// From C++WD'96 7.4.3.2 Example
namespace A {
  int i;
  namespace B {
    namespace C {
      int i;
    }
    using namespace A::B::C;
    void f1() {
      i = 5; // C::i hides A::i
    }
  }
  namespace D {
    using namespace B;
    using namespace C;
    void f2() {
      i = 5; // ambiguous, B::C::i or A::i
    }
  }
  void f3() {
    i = 5; // uses A::i
  }
}
void f4() {
  i = 5; // ill-formed, neither i visible
}
"""

class UsingTest3 (Regression, Test):
  test = """// From C++WD'96 7.4.3.3 Example 2
namespace A {
  int i;
}
namespace B {
  int i;
  int j;
  namespace C {
    namespace D {
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
"""

class UsingTest4 (Regression, Test):
  test = """// From C++WD'96 7.4.3.6 Example
namespace D {
  int d1;
  void f(char);
}
using namespace D;
int d1; // ok, no conflict with D::d1
namespace E {
  int e;
  void f(int);
}
namespace D { // namespace extension
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
"""

class CastTest (Regression, Test):
  test = """
typedef int Foo;
void func() {
  (Foo)1;
  (Foo*)1;
  (const Foo&)1;
}
"""

class TryTest (Regression, Test):
  test = """
void func() {
  try {
    cout << bar;
  }
  catch (string foo) {
    cout << "Error: " << foo << endl;
  }
  catch (...) {
    cout << "Catchall";
  }
}
"""

class MacroTest (Regression, Test):
  test = """
int x;
#define LONGER 12345678
#define SHORTER 1234
#define LINKINSIDE x
#define ARGS(a, b, c) x
int A = LONGER, A2 = x;
int B = SHORTER, B2 = x;
int C = LINKINSIDE, C2 = x;
int D = ARGS(1, 2, 3), D2 = x;
"""

class FuncTest (Regression, Test):
  test = """
void func(char);
void func(int);
void func(double);
void func(const char*);
void test() {
  func('c');
  func(123);
  func(1.2);
  func("s");
}"""

class OperTest (Regression, Test):
  test = """
struct A {};
struct B {};
A operator +(const B&, const B&);
int operator +(const A&, const A&);
void func(A);
void func(B);
void func(int);
void main() {
  B x, y;
  func( (x + y) + (x + y) ); // should call func(int), test func = "func(int)"
}
"""

class KoenigTest (Regression, Test):
  test = """
namespace NS {
  struct A {};
  int operator +(A, A);
};
void func(int);
void func(NS::A) {
  NS::A x, y;
  func(x + y); // should call func(int)
}
"""

class TemplateTest (Regression, Test):
  test = """
struct Object {
  float f;
  double func();
  Object();
  Object(const Object&);
  Object& operator = (const Object&);
};
  
template <typename T>
class list {
  T* m_array;
  int m_size;
public:
  list();
  list(T*);

  T& operator [] (int index) { return m_array[index]; }
  int size() { return m_size; }

  void replace(int index, const T& with) { m_array[index] = with; }
};

void main() {
  list<Object> a_list;
  a_list.replace(1, Object());
  Object b(a_list[1]);
  a_list[2].func();
  b = a_list[3];
}
"""

class TemplateSpecTest (Regression, Test):
  test = """// Test template specializations
template <typename T>
class list {
public:
  list(T*, int size);
};

template <>
class list<void> {
public:
  list(void*, int size) {}
};

template <>
class list<int> {
public:
  list(int*, int size) {}
};
"""

class TemplateSpecTest2 (Regression, Test):
  test = """// Test template specializations #2
template <typename T, int I = 4>
class list {
public:
  list(T*, int size);
  int size() { return I; }
};

template <typename T>
class list<T,0> {
public:
  list(void*, int size) {}
  int size() { return 0; }
};

template <int I>
class list<int, I> {
public:
  list(int*, int size) {}
  int size() { return I; }
};
"""

class StdTest (Regression, Test):
  flags = gcc_include + python_include + "-Wp,-f "
  test = """
#include <vector>
namespace Foo {
  void func(std::vector<int> array);
}
"""

class StaticCastTest (Regression, Test):
  test = """
typedef unsigned int size_type;
int really_big_number = static_cast<size_type>(-1);
"""

# vim: set et sts=2 ts=8 sw=2:
