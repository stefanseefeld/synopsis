// Tests function pointers

typedef void PyObject;

// A function which takes a func ptr as a parameter
void insert(void* (*convert)(PyObject*), bool yesno);

// A function which takes a func ptr as a parameter
void insert(void* (*convert)(PyObject*, int), bool yesno);

// A function which returns a func ptr
void* (*insert2(int))(PyObject*);

int main()
{
  (void)insert2(1);
}
