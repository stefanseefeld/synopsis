#include <Synopsis/Path.hh>
#include <string>
#include <iostream>

using namespace Synopsis;

void test1()
{
  std::string cwd = Path::cwd();
  Path path(__FILE__);                // this file...
  std::string file = path.basename(); // ...without any directory components...
  std::cout << file << std::endl;
  Path abs = Path(file).abs();        // ...the absolute path...
//   std::cout << abs.str() << std::endl;// ...which isn't usable for testing...
  path = abs;
  path.strip(cwd + Path::SEPARATOR);  // ...without removing the prefix
  std::cout << path.str() << std::endl;
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
