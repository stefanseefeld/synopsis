namespace Foo
{
  int x;
}

void func()
{
  using namespace Foo;
  x;
}

void func2()
{
  using namespace Foo = Bar;
  Bar::x;
}

void func3()
{
  Foo::x;
}

void func4()
{
  using Foo::x;
  x;
}
