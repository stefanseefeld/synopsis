class Root
{
public:
  void dummy();
};
class Interface : virtual private Root
{
public:
  virtual void call();
};
class Decorator : virtual public Interface
{
public:
  virtual void call();
private:
  Interface body;
};
class Composite : virtual public Interface
{
public:
  virtual void call();
private:
  Interface one;
  Interface two;
};
class Mixin : virtual public Root
{
public:
  void dummy();
};
class Multi: virtual public Composite, virtual public Mixin
{
public:
  void dummy();
};

class Extern { void dummy();};

class Implementation : virtual public Multi, private Extern
{
public:
  virtual void call();
private:
  Interface *a;
};
