
#if defined(BORLAND55)
#include <mem.h>
#else
#include <memory.h>
#endif 

struct T 
{
  T(int x):a(x){ }
  int a; 
};

int main()
{
  T* t1p;
  T* t2p;

  T a(1138);
  T b(327);

  t1p = &a;
  t2p = &b;
  // provided that t2p points to an initialized object ...
  memcpy(t1p, t2p, sizeof(T));
  // at this point, every subobject of POD type in *t1p
  // contains the same value as the corresponding subobject in
  // *t2p
          
  if (a.a != b.a) return 1;
  return 0;
}
