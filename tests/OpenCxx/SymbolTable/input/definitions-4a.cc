
int x = 0;
struct T 
{
  T() { x++; }
  T(const T& t) { x++; }
  T& operator=(const T& t) { x++; return *this; }
};

struct C 
{
  T t;
};

int main()
{
  C a;
  if (x != 1)
    return 1;
  C b = a;
  if (x != 2)
    return 1;
  b = a;
  if (x != 3)
    return 1;
  
  return 0;
  
}

