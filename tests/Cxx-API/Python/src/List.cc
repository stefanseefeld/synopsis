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
  std::cout << Object::narrow<const char *>(list.str()) << std::endl;
  list.sort();
  std::cout << Object::narrow<const char *>(list.str()) << std::endl;
  list.reverse();
  std::cout << Object::narrow<const char *>(list.str()) << std::endl;
  Object o = list.get(0);
  std::cout << Object::narrow<const char *>(o.str()) << std::endl;
}

int main(int, char **)
{  
  try
  {
    test1();
  }
  catch (const std::exception &e)
  {
    std::cout << "Error : " << e.what() << std::endl;
  }
}
