struct C 
{
  int i;
  void f();
  const C& operator=( const C& );
  void * operator new(unsigned int, C*);
};
const C& C::operator=( const C& other)
{
  if ( this != &other )
  {
    this->~C();          // lifetime of '*this' ends
    new (this) C(other);
    // new object of type C created
    f();                 // well-defined
  }
  return *this;
}

void f() 
{
  C c1;
  C c2;
  c1 = c2; // well-defined
  c1.f();  // well-defined; c1 refers to a new object of type C
}

