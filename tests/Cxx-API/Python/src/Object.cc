#include <Synopsis/Object.hh>
#include "Guard.hh"
#include <string>
#include <iostream>

using namespace Synopsis;

void test1()
{
  Object object;
  Object character('q');
  std::cout << Object::narrow<char>(character) << std::endl;
  std::cout << Object::narrow<const char *>(character.str()) << std::endl;
  Object string("hello world");
  std::cout << Object::narrow<const char *>(string) << std::endl;
  std::cout << Object::narrow<const char *>(string.str()) << std::endl;
  Object integer(42);
  std::cout << Object::narrow<long>(integer) << std::endl;
  std::cout << Object::narrow<const char *>(integer.str()) << std::endl;
  try { std::cout << Object::narrow<long>(string) << std::endl;}
  catch (Object::TypeError &e) { std::cout << e.what() << std::endl;}
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
