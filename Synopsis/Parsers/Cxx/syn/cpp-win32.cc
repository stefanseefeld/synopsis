// $Id: cpp-win32.cc,v 1.1 2003/12/17 15:07:24 stefan Exp $
//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#  include <windows.h>
#  include <stdlib.h>	// For exit
#  include <process.h>	// For _spawn
#  include <fcntl.h>	   // For _O_RDONLY
#  include <io.h>		   // For open, lseek, read

#include <vector>
#include <iostream>
#include <cstring>

// ucpp_main is the renamed main() func of ucpp, since it is included in this
// module
extern "C" int ucpp_main(int argc, char** argv);

const char *
RunPreprocessor(const char *preprocessor,
                const char *src, const std::vector<const char *> &flags,
                bool verbose)
{
  static char cppfile[_MAX_FNAME];
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char fname[_MAX_FNAME];
  char ext[_MAX_EXT];

  _splitpath(src, drive, dir, fname, ext);
  _makepath(cppfile, drive, dir, fname, ".i");
  
  if (preprocessor)
  {

    std::vector<const char *> args;
    args.push_back("-nologo");
    args.insert(args.end(), flags.begin(), flags.end());
    args.push_back("-E");  // Use -EP if you don't want the directives #line
    args.push_back("-C");  // Don't strip comments
    args.push_back("-P");  // Preprocessor output to <file>.i
    args.push_back("-Tp"); // Consider source file as C++ file,
                           // whatever extension.
//     args.push_back("-Fp"); // Preprocessor output 
//     args.push_back(dest);
    args.push_back(src);
    args.push_back((char*)0);

    if (verbose)
    {
       std::cout << "calling external preprocessor 'cl'\n";
       for (std::vector<const char *>::iterator i = args.begin(); i != args.end(); ++i)
          std::cout << ' ' << *i;
       std::cout << std::endl;
    }


    int status = _spawnvp(_P_WAIT, "cl", (char **)&*args.begin());
    if(status != 0)
    {
       if(status == -1) perror("cannot invoke the preprocessor");
       // VIC Feb 20 1998
       // MSVC 5.0 returns non-zero exit code on normal input
       if (status != 2) exit(status);
    }

  }
  else
  { // else use ucpp
    // Create argv vector
    std::vector<const char *> args = flags;
    char *cc = getenv("CC");
    args.insert(args.begin(), cc ? cc : "ucpp");
    args.push_back("-C"); // keep comments
    args.push_back("-lg"); // gcc-like line numbers
    args.push_back("-o"); // output to...
    args.push_back(cppfile);
    args.push_back(src);
    if (verbose)
    {
      std::cout << "calling ucpp\n";
      for (std::vector<const char *>::iterator i = args.begin(); i != args.end(); ++i)
        std::cout << ' ' << *i;
      std::cout << std::endl;
    }
    
    // Call ucpp
    int status = ucpp_main(args.size(), (char **)&*args.begin());
    if (status != 0)
      std::cerr << "ucpp returned error flag. ignoring error." << std::endl;
  }
  return cppfile;
}
