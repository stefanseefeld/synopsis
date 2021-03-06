void* malloc(int) { return 0; }




/////////////////////////////////////////////////////////////////////////////
// Code above is both sufficient and necessary for proper compilation.     //
/////////////////////////////////////////////////////////////////////////////     
struct B 
{
  virtual void f() {}
  void mutate();
  virtual ~B() {}
};



struct D1 : B { void f() {} };
struct D2 : B { void f() {} };
void B::mutate() 
{
  new (this) D2; // reuses storage - ends the lifetime of '*this'
  // f();        // undefined behavior
  // ... = this; // ok, 'this' points to valid memory
}
void g() 
{
  void* p = malloc(sizeof(D1) + sizeof(D2));
  B* pb = new (p) D1;
  pb->mutate();
  &pb;           // ok: pb points to valid memory
  void* q = pb;  // ok: pb points to valid memory
  pb->f();       // undefined behavior, lifetime of *pb has ended
}
