class Class
{
public:
  enum Enum { NONE, ALL=0xff};
  struct Nested {};
  typedef unsigned int Size;

  void method() const;
  void method(int);
private:
  Size my_size;
};
