struct C 
{
  typedef int I;
};
typedef int I1, I2;
int* p;
int* q;

struct A 
{
  ~A() {}
};
typedef A AB;
int main() 
{
  AB *ab;
  ab->AB::~AB(); // explicitly calls the destructor for A
  p->C::I::~I(); // I is looked up in the scope of A
  q->I1::~I2();  // I2 is looked up in the scope of
                 // the postfix-expression
  return 0;
}

