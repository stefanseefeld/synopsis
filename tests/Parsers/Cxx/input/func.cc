void func(char);
void func(int);
void func(double);
void func(const char*);

void test()
{
  func('c');
  func(123);
  func(1.2);
  func("s");
}
