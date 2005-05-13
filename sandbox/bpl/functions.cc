class Foo
{
public:
  Foo() { init();}
private:
  void init();
};

void Foo::init()
{

}

Foo *bar()
{
  return new Foo();
}
