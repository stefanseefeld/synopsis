namespace NS
{
  struct A {};
  int operator +(A, A);
};

void func(int);
void func(NS::A)
{
  NS::A x, y;
  func(x + y); // should call func(int)
}
