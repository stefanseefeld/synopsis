// vim: set ts=8 sw=2 sts=2 et:
/* This file contains test cases for templates, taken from the C++ std.
 */

// Comments out lines that are known to confuse the parser
#define FAIL(x)

// Template parameter may be a class template
namespace Ex_14_1_1
{
  template<class T>
    class myarray
    { };

  template<class K, class V, template<class T> class C = myarray>
    class Map
    {
      C<K> key;
      C<V> value;
    };
}

// Prefer type-parameter over non-type template parameter
namespace Ex_14_1_2
{
  // A class named T to confuse the programmer
  class T { /* ... */ };
  int i;

  // Here the template has a type-parameter 'T' rather than an unnamed
  // non-type template parameter of class T.
  // (A named non-type template parameter: "class T theT" or just "T theT")
  template<class T, T i>
    void f(T t)
    {
      T t1 = i;     // template parameters T and i
      ::T t2 = ::i; // global namespace members T and i
    }
}

// Non-type non-reference template-parameter is not an lvalue
namespace Ex_14_1_5
{
  template<const X& x, int i>
    void f()
    {
      i++; // error: change of template-parameter value

      &x; // ok
      &i; // error: address of non-reference template-parameter

      int& ri = i; // error: non-const reference bound to temporary
      const int& cri = i; // ok: const reference bound to temporary
    }
}

// Non-type template-parameter shall not be of void or floating type
namespace Ex_14_1_6
{
  template<double d> class X; // error
  template<double* pd> class Y; // ok
  template<double& rd> class Z; // ok
}

// Array type decay does not apply to template-parameters
namespace Ex_14_1_7
{
  template<int a[5]>
    struct S
    { };

  int v[5];
  int* p = v;
  S<v> x; // fine
  S<p> y; // error
}

// Template-name followed by '<' is always template-id
namespace Ex_14_2_3
{
  template<int i>
    class X
    { };

  FAIL( X<  1>2  > x1; )// syntax error
        X< (1>2) > x2;  // ok

  template<class T>
    class Y
    { };

  Y< X<1> >  x3; // ok
  Y< X< 6>>1 > > x4; // ok: Y< X< (6>>1) > >
}

// Must use 'template' when '<' would otherwise mean 'less-than'
// ie, after '.' or '->' or after :: in a qualified-id that explicitly depends
// on a template-argument
namespace Ex_14_2_4
{
  class X
  {
  public:
    template<size_t> X* alloc();
  };

  void f(X* p)
  {
    X* p1 = p->alloc<200>(); // ill-formed: < means less-than

    X* p2 = p->template alloc<200>(); // fine: < starts explicit qualification
  }
}

// Types of template-arguments in template-id shall match types in
// template-parameter-list
namespace Ex_14_3_1
{
  template<class T>
    class Array
    {
      T* v;
      int sz;
    public:
      explicit Array(int);
      T& operator[](int);
      T& elem(int i) { return v[i]; }
    };

  Array<int> v1(20);

  template<class T>
    class complex
    {
    public:
      complex(T, T) { }
    };
  typedef complex<double> dcomplex;
  
  Array<dcomplex> v2(30);
  Array<dcomplex> v3(40);

  void f()
  {
    v1[3] = 7;
    v2[3] = v3.elem(4) = dcomplex(7,8);
  }
}

// Prefer type-id over expression in template-argument
namespace Ex_14_3_2
{
  template<class T>
    void f();

  template<int i>
    void f();

  void g()
  {
    f<int()>(); // int() is a type-id: call the first f
  }
}

// Template argument for non-type non-reference template-parameter shall not
// be a string literal
namespace Ex_14_3_3
{
  template<class T, char* p>
    class X
    {
      X(const char* q) { }
    };

  X<int,"Studebaker"> x1; // error: string literal as template-argument

  char p[] = "Vivi";
  X<int,p> x2; // ok, p has external linkage
}

// Addresses of array elements and of non-static class members shall not be
// used as template-arguments
namespace Ex_14_3_4
{
  template<int* p>
    class X
    { };

  int a[10];
  struct S
  {
    int m;
    static int s;
  } s;

  X<&a[2]> x3; // error: address of array element
  X<&s.m> x4; // error: address of non-static member
  X<&s.s> x5; // error: &S::s must be used
  X<&S::s> x6; // ok: address of static member
}

// Non-type reference template-parameter has limitations on what it can be
// bound to
namespace Ex_14_3_5
{
  template<const int& CRI>
    struct B
    { };

  B<1> b2; // error: temporary required for template argument

  int c = 1;
  B<c> b1; // ok
}

// Standard conversions used to turn template-argument into type of non-type
// template-parameter
namespace Ex_14_3_6
{
  template<const int* pci>
    struct X
    { };
  int ai[10];
  X<ai> x; // Array to pointer and qualification conversions

  struct Base { };
  struct Derived : Base { };
  template<Base& b>
    struct Y
    { };
  Derived d;
  Y<d> yd; // derived to base conversion
}

// FuncPtr template argument must have exact type
namespace Ex_14_3_7
{
  void f(char);
  void f(int);

  template<void (*pf)(int)>
    struct A
    { };

  A<&f> a; // selects f(int)
}

// Declarations cannot become FuncPtr's because a FuncPtr is used as a
// template-argument
namespace Ex_14_3_8
{
  template<class T>
    struct A
    {
      static T t;
    };
  typedef int function();
  A<function> a; // ill-formed: would declare A<function>::t as a static member function
}

// Local, no-linkage and unnamed types cannot be used as template-arguments
namespace Ex_14_3_9
{
  template<class T>
    struct X {};
  void f()
  {
    struct S
    { };

    X<S> x3; // error: local type used as template-argument
  }
}

// Template follows normal access rules
namespace Ex_14_3_10
{
  template<class T>
    class X
    { };

  class Y
  {
  private:
    struct S
    { };

    X<S> x; // ok: S is accessible
  };

  X<Y::S> y; // error: S not accessible
}

// Template-argument list is "<>" if all default arguments used
namespace Ex_14_3_11
{
  template<class T = char>
    class String;
  String<>* p; // ok: String<char>
  String* q; // syntax error
}

// May specify argument for destructor call
namespace Ex_14_3_12
{
  template<class T>
    struct A
    {
      ~A();
    };

  void main()
  {
    A<int>* p;
    p->A<int>::~A(); // ok: destructor call
    FAIL( p->A<int>::~A<int>(); ) // ok: destructor call
  }
}

// Template-id's can refer to the same class or function
// if they are the same
namespace Ex_14_4_1
{
  template<class E, int size>
    class buffer
    { };

  buffer<char, 2*512> x;
  buffer<char, 1024> y; // same type as x

  template<class T, void(*err_fct)()>
    class list
    { };
  void error_handler1();
  void error_handler2();
  list<int, &error_handler1> x1;
  list<int, &error_handler2> x2;
  list<int, &error_handler2> x3; // same type as x2 but not x1 or x4
  list<char,&error_handler2> x4;
}

// Template-id not specified in primary template declaration
namespace Ex_14_5_1
{
  template<class T1, class T2, int I>
    class A<T1, T2, I> // error!
    { };

  template<class T1, int I>
    void sort<T1, I>(T1 data[I]); // error
};

// Can re-order template parameter names in method definition
namespace Ex_14_5_1_3
{
  template<class T1, class T2>
    struct A
    {
      void f1();
      void f2();
    };

  template<class T2, class T1>
    void A<T2,T1>::f1()
    { } // ok

  template<class T2, class T1>
    void A<T1,T2>::f2()
    { } // error
}

// Member function can be defined outside template
namespace Ex_14_5_1_1_2
{
  template<class T>
    class Array
    {
      T* v;
      int sz;
    public:
      explicit Array(int);
      T& operator[](int);
      T& elem(int i) { return v[i]; }
    };
  // Three function templates declared above
  
  template<class T>
    T& Array<T>::operator[](int i)
    {
      return v[i];
    }
}
  
// Template arguments for method taken from object type
namespace Ex_14_5_1_1_3
{
  using Ex_14_5_1_1_2::Array;
  using Ex_14_3_1::dcomplex;

  void f()
  {
    Array<int> v1(20);
    Array<dcomplex> v2(30);

    v1[3] = 7; // Array<int>::operator[]();
    v2[3] = dcomplex(7,8); // Array<dcomplex>::operator[]();
  }
}

// Can define member class outside class, but before first use
namespace Ex_14_5_1_2_2
{
  template<class T>
    struct A
    {
      class B;
    };

  A<int>::B* b1; // ok: requires A to be defined but not A::B
  template<class T>
    class A<T>::B
    { };
  A<int>::B b2; // ok: requires A::B to be defined
}

