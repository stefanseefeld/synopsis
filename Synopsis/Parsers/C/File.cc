//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <File.hh>
#include <Expression.hh>
#include <Statement.hh>
#include <Token.hh>

#include <cstdio>
#include <cstring>
#include <cassert>
#include <fstream>

ParseEnv *global_parse_env;
Type     *global_type_list;

extern int yyparse();

File::~File()
{
  Statement *ste, *nxt = 0;

  for (ste = my_head; ste; ste = ste->next)
  {
    delete nxt;
    nxt = ste;
  }
  delete nxt;
}

void File::find_statement(fnStemntCallback cb)
{
  for (Statement *ste = my_head; ste; ste = ste->next)
    ste->find_statement(cb);
}

void File::find_function_def(fnFunctionCallback cb)
{
  for (Statement *ste = my_head; ste; ste = ste->next)
    // Function definition can only occur at the top level.
    if (ste->isFuncDef()) (cb)((FunctionDef*) ste);
}

void File::add(Statement *stemnt)
{
  if (stemnt)
  { 
    stemnt->next = 0;
    
    if (my_tail) my_tail->next = stemnt;
    else my_head = stemnt;
    my_tail = stemnt;
  }
}

void File::insert(Statement *stemnt, Statement *after)
{
  if (stemnt)
  {
    stemnt->next = 0;

    if (my_tail)
    {
       if (after)
       {
         stemnt->next = after->next;
         after->next = stemnt;
       }
       else
       {
         stemnt->next = my_head;
         my_head = stemnt;
       }
       if (stemnt->next == 0) my_tail = stemnt;
    }
    else my_head = my_tail = stemnt;
  }
}

bool File::insert_before(Statement *stemnt, Statement *before)
{
  if (!stemnt) return false;

  Statement *sp;

  for(Statement **spp = &my_head; (sp = *spp) != 0; spp = &sp->next)
  {
    if (sp == before)
    {
      stemnt->next = before;
      *spp = stemnt;
      return true;
    }
  }
  return false;
}

std::ostream &operator << (std::ostream &os, const File &f)
{
  int inInclude = 0;

  for (Statement *stemnt = f.my_head; stemnt; stemnt = stemnt->next)
  {
    if (inInclude > 0)
    {
      if (stemnt->isEndInclude()) inInclude--;
      else if (stemnt->isInclude()) inInclude++;
    }
    else
    {
      if (stemnt->isInclude()) inInclude++;
      os << *stemnt << std::endl;
    }
  }
  return os;
}

File *File::parse(const std::string &path, const std::string &name)
{
  std::ifstream ifs(path.c_str());
  if (!ifs) return 0;
    
  global_parse_env = new ParseEnv(&ifs, &std::cerr, name);
  File *unit = global_parse_env->transUnit;
    
  unit->my_contxt.EnterScope();

  while(yyparse());

  ifs.close();

  int err_cnt = global_parse_env->err_cnt;

  delete global_parse_env;
  global_parse_env = 0;

  if (!unit) return 0;
 
  unit->my_contxt.ExitScope();

  return unit;
}
