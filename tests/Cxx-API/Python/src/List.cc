#include <Synopsis/Python/Object.hh>
#include "Guard.hh"
#include <string>
#include <iostream>

using namespace Synopsis::Python;

void test1()
{
  List list;
  list.append('a');
  list.append(1);
  list.append("hello");
  list.append("world");
  std::cout << Object::narrow<std::string>(list.str()) << std::endl;
  list.sort();
  std::cout << Object::narrow<std::string>(list.str()) << std::endl;
  list.reverse();
  std::cout << Object::narrow<std::string>(list.str()) << std::endl;
  Object o = list.get(0);
  std::cout << Object::narrow<std::string>(o.str()) << std::endl;

  for (List::iterator i = list.begin(); i != list.end(); ++i)
    std::cout << Object::narrow<std::string>(i->str()) << std::endl;
  for (List::reverse_iterator i = list.rbegin(); i != list.rend(); ++i)
    std::cout << Object::narrow<std::string>(i->str()) << std::endl;
  std::cout << *(list.begin() + 1) << ' ' << *(list.end() - 1) << std::endl;
  std::cout << *(list.rbegin() + 1) << ' ' << *(list.rend() - 1) << std::endl;
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
