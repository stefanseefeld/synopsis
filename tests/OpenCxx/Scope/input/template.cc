template <typename T>
void function(T t) {}

template <typename T>
struct Struct
{
  static T type;
};

template <> struct Struct<int>
{
  static int type;
};

template struct Struct<double>;
