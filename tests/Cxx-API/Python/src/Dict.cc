#include <Synopsis/Dict.hh>
#include "Guard.hh"
#include <string>
#include <iostream>

using namespace Synopsis;

void test1()
{
  Dict dict;
  dict.set('a', 'A');
  dict.set(1, 2);
  dict.set("hello", "world");
  std::cout << Object::narrow<const char *>(dict.str()) << std::endl;
  std::cout << Object::narrow<const char *>(dict.keys().str()) << std::endl;
  std::cout << Object::narrow<const char *>(dict.values().str()) << std::endl;
  std::cout << Object::narrow<const char *>(dict.items().str()) << std::endl;
}

void test2()
{
  Dict dict;
  dict.set('a', 'A');
  std::cout << Object::narrow<const char *>(dict.get('a').str()) << std::endl;
  std::cout << Object::narrow<const char *>(dict.get('b').str()) << std::endl;
  dict.set(1, 2);
  dict.set("hello", "world");
  for (Dict::iterator i = dict.begin(); i != dict.end(); ++i)
    std::cout << Object::narrow<const char *>((*i).str()) << std::endl;
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
