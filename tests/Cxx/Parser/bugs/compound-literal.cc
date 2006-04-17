struct Foo { float a;};

Foo func(float b)
{
  // GNU Extension:
  // 
  // postfix-expression:
  //   ( type-id ) { initializer-list , [opt] }
  return (Foo) {b};
}
