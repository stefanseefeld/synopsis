#include <Synopsis/Trace.hh>
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>

using namespace Synopsis;

int main(int argc, char **argv)
{
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " [-d] <output> <input>" << std::endl;
    exit(-1);
  }
  try
  {
    std::string output;
    std::string input;
    if (argv[1] == std::string("-d"))
    {
      Trace::enable(Trace::ALL);
      output = argv[2];
      input = argv[3];
    }
    else
    {
      output = argv[1];
      input = argv[2];
    }
    std::ofstream ofs;
    {
      if (output != "-")
	ofs.open(output.c_str());
      else
      {
	ofs.copyfmt(std::cout);
	static_cast<std::basic_ios<char> &>(ofs).rdbuf(std::cout.rdbuf());
      }
    }
    std::ifstream ifs(input.c_str());
    Buffer buffer(ifs.rdbuf(), input);
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
  catch (std::exception const &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}

