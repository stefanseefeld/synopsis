namespace A 
{
  namespace B 
  {
    void f1(int);
  }
  using namespace B;
}
void A::f1(int){}  // ill-formed, f1 is not a member of A

