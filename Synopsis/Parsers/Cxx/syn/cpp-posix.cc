// $Id: cpp-posix.cc,v 1.1 2003/12/17 15:07:24 stefan Exp $
//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#  include <unistd.h>
#  include <signal.h>
#  include <sys/wait.h>

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
  static char cppfile[1024];
  strcpy(cppfile, "/tmp/synopsis-XXXXXX");
  int temp_fd = mkstemp(cppfile);
  if (temp_fd == -1)
  {
    perror("RunPreprocessor");
    exit(1);
  }
  // Not interested in the open file, just the unique filename
  close(temp_fd);
   
  if (preprocessor)
  {
    switch(fork())
    {
      case 0:
      {
        std::vector<const char *> args;
        char *cc = getenv("CC");
        if (cc)
        {
          // separate command and arguments
          do
          {
            args.push_back(cc);
            cc = strchr(cc, ' ');                  // find next whitespace...
            while (cc && *cc == ' ') *cc++ = '\0'; // ...and skip to next non-ws
          }
          while (cc && *cc != '\0');
        }
        else
        {
          args.push_back("cpp");
        }
        args.insert(args.end(), flags.begin(), flags.end());
        args.push_back("-C"); // keep comments
        args.push_back("-E"); // stop after preprocessing
        args.push_back("-o"); // output to...
        args.push_back(cppfile);
        args.push_back("-x"); // language c++
        args.push_back("c++");
        args.push_back(src);
        if (verbose)
        {
          std::cout << "calling external preprocessor\n" << args[0];
          for (std::vector<const char *>::iterator i = args.begin(); i != args.end(); ++i)
            std::cout << ' ' << *i;
          std::cout << std::endl;
        }
        args.push_back(0);
        execvp(args[0], (char **)&*args.begin());
        perror("cannot invoke compiler");
        exit(-1);
        break;
      }
      case -1:
        perror("RunPreprocessor");
        exit(-1);
        break;
      default:
      {
        int status;
        wait(&status);
        if (status != 0)
        {
          if (WIFEXITED(status))
            std::cout << "exited with status " << WEXITSTATUS(status) << std::endl;
          else if (WIFSIGNALED(status))
            std::cout << "stopped with status " << WTERMSIG(status) << std::endl;
          exit(1);
        }
      }
    } // switch

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

