#include <Synopsis/AST/TypeKit.hh>
#include "Guard.hh"
#include <string>
#include <iostream>

using namespace Synopsis;

void test1()
{
  AST::TypeKit kit = AST::TypeKit();
  std::cout << kit.create_type("C--") << std::endl;
  std::cout << kit.create_named("C--", Tuple("Foo")) << std::endl;
  std::cout << kit.create_base("C--", Tuple("integer")) << std::endl;
  std::cout << kit.create_dependent("C--", Tuple("dep")) << std::endl;
  std::cout << kit.create_unknown("C--", Tuple("FooBar")) << std::endl;
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

