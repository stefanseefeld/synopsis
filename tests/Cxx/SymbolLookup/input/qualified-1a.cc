class A 
{
public:
  static int n;
};
int A::n;
int main()
{
  int A;
  A::n = 42;          // OK
  //  A b;            // ill-formed: A does not name a type
  return 0;
}

