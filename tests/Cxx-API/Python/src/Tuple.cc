#include <Synopsis/Tuple.hh>
#include <Synopsis/List.hh>
#include "Guard.hh"
#include <string>
#include <iostream>

using namespace Synopsis;

void test1()
{
  List list;
  list.append('a');
  list.append(1);
  list.append("hello");
  list.append("world");
  Tuple tuple = list.tuple();
  std::cout << Object::narrow<const char *>(tuple.str()) << std::endl;
  Object object(tuple);
  tuple = Object::narrow<Tuple>(object);
  std::cout << Object::narrow<const char *>(tuple.str()) << std::endl;
}

void test2()
{
  Tuple t1('1', '2', '3');
  std::cout << Object::narrow<const char *>(t1.str()) << std::endl;
  Tuple t2("hello", "world", 42);
  std::cout << Object::narrow<const char *>(t2.str()) << std::endl;
}

int main(int, char **)
{  
  try
  {
    test1();
    test2();
  }
  catch (const std::exception &e)
  {
    std::cout << "Error : " << e.what() << std::endl;
  }
}
