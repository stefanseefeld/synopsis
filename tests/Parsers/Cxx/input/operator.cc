struct A {};
struct B {};
A operator +(const B&, const B&);
int operator +(const A&, const A&);
void func(A);
void func(B);
void func(int);
void main()
{
  B x, y;
  func( (x + y) + (x + y) ); // should call func(int), test func = "func(int)"
}
