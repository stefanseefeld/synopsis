struct A 
{
  typedef int AT;
  void f1(AT) {}
  void f2(float) {}
};

struct B 
{
  typedef float BT;
  friend void A::f1(AT);
  friend void A::f2(BT);
};
