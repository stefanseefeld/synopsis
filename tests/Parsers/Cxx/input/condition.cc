// Tests the ability to use a declaration in the condition of an if or switch
struct X
{
  operator bool() const {return true;}
  struct Y
  {
    operator bool() const {return true;}
  };
};
void foo()
{
  int _i;
  X _x;
  X::Y _y;

  if (int i = 3) {}
  if (const int i = 3) {}
  if (const int* i = &_i) {}
  if (int* const i = &_i) {}
  
  if (X foo = _x) {}
  if (const X foo = _x) {}
  if (const X* foo = &_x) {}
  if (X* const foo = &_x) {}

  if (X::Y foo = _y) {}
  if (const X::Y foo = _y) {}
  if (const X::Y* foo = &_y) {}
  if (X::Y* const foo = &_y) {}

  switch (int i = 3) {}
  switch (const int i = 3) {}
  
}
