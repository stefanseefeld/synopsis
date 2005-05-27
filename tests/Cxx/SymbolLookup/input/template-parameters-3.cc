class T { /* ... */ };
int i;

template <class T, T i> void f(T t)
{
  T t1 = i;        // template-parameters T and i
  ::T t2 = ::i;    // global namespace members T and i
}
