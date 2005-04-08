struct B 
{
  B(){}
  ~B(){}
};
const B b;
void h() 
{
  b.~B();
  new (&b) const B; // undefined behavior
}

