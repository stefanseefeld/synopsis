class T { };
struct B 
{
  ~B();
};
void h() 
{
  B b;
  new (&b) T;
} // undefined behavior at block exit
