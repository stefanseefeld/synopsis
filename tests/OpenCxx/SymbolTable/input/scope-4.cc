struct X 
{
  enum E { z = 16 };
  int b[X::z]; //ok
};

