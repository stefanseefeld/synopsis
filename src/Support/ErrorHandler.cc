//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "ErrorHandler.hh"
#include <iostream>
#include <cstdlib>

#ifndef _WIN32
# include <signal.h>
# include <sys/wait.h>
# ifdef __GLIBC__
#  include <execinfo.h>
# endif
#endif

namespace
{
Synopsis::ErrorHandler::Callback callback = 0;

#ifndef _WIN32
  struct sigaction olda;
  struct sigaction newa;
#endif
  
void print_stack()
{
#ifdef __GLIBC__
  void *bt[128];
  int bt_size = backtrace(bt, sizeof(bt) / sizeof(void *));
  char **symbols = backtrace_symbols (bt, bt_size);
  for (int i = 0; i < bt_size; i++) std::cout << symbols[i] << std::endl;
#endif
}

void sighandler(int signo)
{
  std::string signame = "Signal";

#ifndef _WIN32
  switch (signo)
  {
    case SIGABRT:
        signame = "Abort";
        break;
    case SIGBUS:
        signame = "Bus error";
        break;
    case SIGSEGV:
        signame = "Segmentation Violation";
        break;
    default:
        signame = "unknown";
        break;
  };
#endif

  std::cerr << signame << " caught" << std::endl;
  if (callback) callback();
  print_stack();
  exit(-1);
}

}

using namespace Synopsis;

ErrorHandler::ErrorHandler(Callback c)
{
  callback = c;
#ifndef _WIN32
  newa.sa_handler = &sighandler;
  sigaction(SIGSEGV, &newa, &olda);
  sigaction(SIGBUS, &newa, &olda);
  sigaction(SIGABRT, &newa, &olda);
#endif  
}

ErrorHandler::~ErrorHandler()
{
#ifndef _WIN32
  sigaction(SIGABRT, &olda, 0);
  sigaction(SIGBUS, &olda, 0);
  sigaction(SIGSEGV, &olda, 0);
#endif
}

