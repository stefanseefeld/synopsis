
#if defined(BORLAND55)
#include <mem.h>
#else
#include <memory.h>
#endif

struct T { int a; };

// start of test case:
#define N sizeof(T)

int main ()
{
  char buf[N];
  T obj;  // obj initialized to its original value
  obj.a = 1138;
  memcpy(buf, &obj, N);
  // between these two calls to memcpy,
  // obj might be modified
  memcpy(&obj, buf, N);
  // at this point, each subobject of obj of scalar type
  // holds its original value
  
  if (1138 != obj.a) return 1;
  return 0;
}
