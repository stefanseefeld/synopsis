//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _File_hh
#define _File_hh

#include <ParseEnv.hh>
#include <Symbol.hh>
#include <Callback.hh>

#include <cstdlib>
#include <iostream>
#include <cassert>

class Statement;

class File
{
public:
  File(const std::string &name) : my_name(name), my_head(0), my_tail(0) {}
  ~File();

  //. parse an input file and return a File object
  //. @param input the input file name (possibly preprocessed temporary file)
  //. @param name the name of the file to be reported (the original file)
  static File *parse(const std::string &input, const std::string &name);

  void find_statement(fnStemntCallback);
  void find_function_def(fnFunctionCallback);

  void add(Statement *stemnt);
  void insert(Statement *stemnt, Statement *after = 0);
  bool insert_before(Statement *stemnt, Statement *before);

// private:
  const std::string  my_name;   // The file we parsed this from.

  Statement         *my_head;   // The first statement in the list.
  Statement         *my_tail;   // The last statement in the list.
   
  Context            my_contxt; // Hold onto this context.

  friend std::ostream &operator << (std::ostream &os, const File &f);
};

extern ParseEnv *global_parse_env; // quick hack
extern Type     *global_type_list;

#endif
