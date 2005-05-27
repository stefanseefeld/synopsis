// Test template specializations #2
template <typename T, int I= 4>
class list
{
public:
  list(T*, int size);
  int size() { return I; }
};

template <typename T>
class list<T,0>
{
public:
  list(void*, int size) {}
  int size() { return 0; }
};

template <int I>
class list<int, I>
{
public:
  list(int*, int size) {}
  int size() { return I; }
};
