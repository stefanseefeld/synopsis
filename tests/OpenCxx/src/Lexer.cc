#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace Synopsis;

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <output> <input>" << std::endl;
    exit(-1);
  }
  std::ofstream ofs(argv[1]);
  std::ifstream ifs(argv[2]);
  Buffer buffer(ifs.rdbuf());
  Lexer lexer(&buffer);
  int i = 0;
  while(true)
  {
    Token token;

    int t = lexer.look_ahead(i++, token);
    if(!t) break;
    else
    {
      ofs << std::setw(3);
      if(t < 128)
	ofs << t << " '" << static_cast<char>(t) << "': ";
      else
	ofs << t << "    : ";
      ofs.put('"');
      while(token.length-- > 0) ofs.put(*token.ptr++);
      ofs << "\"\n";
    }
  }
}

