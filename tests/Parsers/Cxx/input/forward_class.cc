class X
{
public:
  void f(int);
  int a;
};
class Y;

int X::* pmi = &X::a;
void (X::* pmf)(int) = &X::f;
double X::* pmd;
char Y::* pmc;
X obj;
X* pobj;
void foo()
{
  obj.*pmi = 7;   // assign 7 to an integer
                  // member of obj
  (obj.*pmf)(7);  // call a function member of obj
                  // with the argument 7
  pobj->*pmi = 7;   // assign 7 to an integer
  (pobj->*pmf)(7);  // call a function member of obj
}
