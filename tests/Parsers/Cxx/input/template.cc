struct Object
{
  float f;
  double func();
  Object();
  Object(const Object&);
  Object& operator = (const Object&);
};
  
template <typename T>
class list
{
  T* m_array;
  int m_size;
public:
  list();
  list(T*);

  T& operator [] (int index) { return m_array[index]; }
  int size() { return m_size; }

  void replace(int index, const T& with) { m_array[index] = with; }
};

void main()
{
  list<Object> a_list;
  a_list.replace(1, Object());
  Object b(a_list[1]);
  a_list[2].func();
  b = a_list[3];
}
