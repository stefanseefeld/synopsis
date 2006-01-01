//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/process_pragma.hh>
#include <Synopsis/Trace.hh>
#include <iostream>
#include <sstream>
#include <iterator>
#include <algorithm>

namespace Synopsis
{

//. Process a pragma directive.
//. Warning: as the lexer is backtracking, it may rewind
//. across pragma directives, so there is no guarantee
//. each directive is seen only once.
void process_pragma(std::string const &line)
{
  std::string token;
  std::istringstream iss(line);
  iss >> token;
  if (token != "synopsis") return;
  iss >> token;
  if (token == "trace")
  {
    iss >> token;
    if (token == "on") Trace::enable(Trace::ALL);
    else if (token == "off") Trace::enable(Trace::NONE);
  }
  else if (token == "message")
  {
    std::cout << "message:";
    // Copy the rest of the line to std::cout.
    std::copy(std::istreambuf_iterator<char>(iss),
	      std::istreambuf_iterator<char>(),
	      std::ostreambuf_iterator<char>(std::cout));
    std::cout << std::endl;
  }
}

}
