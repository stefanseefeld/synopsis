// Test template specializations
template <typename T>
class list
{
public:
  list(T*, int size);
};

template <>
class list<void>
{
public:
  list(void*, int size) {}
};

template <>
class list<int>
{
public:
  list(int*, int size) {}
};
