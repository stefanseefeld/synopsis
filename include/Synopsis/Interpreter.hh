// $Id: Interpreter.hh,v 1.2 2004/01/11 19:46:29 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_Interpreter_hh
#define _Synopsis_Interpreter_hh

#include <Synopsis/Object.hh>
#include <string>

namespace Synopsis
{

class Interpreter
{
public:
  struct Exception
  {
    Exception() {}
  };

  enum Mode { EVAL = Py_eval_input,
	      FILE = Py_file_input,
	      SINGLE = Py_single_input};
   
  Interpreter() {}
  ~Interpreter() {}

  Object run_string(const std::string &, Mode, Object, Object);
  Object run_file(const std::string &, Mode, Object, Object);
private:
};

inline Object Interpreter::run_string(const std::string &code,
                                      Mode m, Object globals, Object locals)
{
  PyObject *retn = PyRun_String(const_cast<char *>(code.c_str()), m,
				globals.my_impl, locals.my_impl);
  if (!retn) throw Exception();
  return retn;
}

inline Object Interpreter::run_file(const std::string &script,
                                    Mode m, Object globals, Object locals)
{
  ::FILE *file = fopen(script.c_str(), "r");
  PyObject *retn = PyRun_File(file, const_cast<char *>(script.c_str()),
			      m, globals.my_impl, locals.my_impl);
  fclose(file);
  if (!retn) throw Exception();
  return retn;
}

}

#endif
