typedef int  c;
enum { i = 1 };
class X 
{
  //            char  v[i];  // error: 'i' refers to ::i
  // but when reevaluated is X::i
  int  f() { return sizeof(c); }  // okay: X::c
  char  c;
  enum { i = 2 };
};
typedef char*  T;
struct Y 
{
  //            T  a;    // error: 'T' refers to ::T
  // but when reevaluated is Y::T
  typedef long  T;
  T  b;
};

typedef int I;
class D 
{
  //typedef I I; // error: even tho no reordering involved
};
