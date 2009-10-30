//
// Copyright (C) 2009 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "comments.hh"
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <string>
#include <cstring>

extern "C"
{
#include <cpp.h>
};

void handle_newline(lexer_state &ls)
{
  add_newline();
  if (ls.flags & KEEP_OUTPUT) fputs("\n", ls.output);
}

void handle_pragma(lexer_state &ls)
{
  if (ls.flags & KEEP_OUTPUT)
  {
    // If this is a PRAGMA token, the
    // string content is in fact a compressed token list,
    // that we uncompress and print.
    fputs("#pragma ", ls.output);
    for (char *c = ls.ctok->name; *c; ++c)
    {
      if (STRING_TOKEN(*c))
        for (++c; *c != PRAGMA_TOKEN_END; ++c) fputc(*c, ls.output);
      else
        fputs(operators_name[*c], ls.output);
    }
  }
}

void handle_context(lexer_state &ls)
{
  if (ls.flags & KEEP_OUTPUT)
    fprintf(ls.output, "#line %ld \"%s\"\n", ls.line, ls.ctok->name);//, enter_leave);
  // Reset for new file.
  ls.oline = ls.line;
}

void handle_token(lexer_state &ls)
{
  if (ls.flags & KEEP_OUTPUT)
    fputs(STRING_TOKEN(ls.ctok->type) ? ls.ctok->name : operators_name[ls.ctok->type],
          ls.output);
  if (ls.ctok->type == COMMENT)
  {
    if (ls.ctok->name[0] == '/' && ls.ctok->name[1] == '*')
    {
      add_ccomment(ls.ctok->name);
      // If this is a multi-line comment, account for embedded newlines.
      char const *c = ls.ctok->name + 2;
      while (*(++c)) if (*c == '\n') ++ls.oline;
    }
    else
      add_cxxcomment(ls.ctok->name);
  }
  else // non-comment token
  {
    end_file_preamble();
    clear_comment_cache();
  }
}


void ucpp(char const *input, char const *output, std::vector<std::string> const &flags)
{
  lexer_state ls;

  // step 1
  init_cpp();

  // step 2
  no_special_macros = 0;
  emit_defines = emit_assertions = 0;

  // step 3 -- with assertions
  init_tables(1);

  // step 4 -- no default include path
  init_include_path(0);

  // step 5 -- no need to reset the two emit_* variables set in 2
  emit_dependencies = 0;

  // step 6 -- we work with stdin, this is not a real filename
  set_init_filename(const_cast<char*>(input), 0);

  // step 7 -- we make sure that assertions are on, and pragma are handled
  init_lexer_state(&ls);
  init_lexer_mode(&ls);
  ls.flags |= HANDLE_ASSERTIONS | HANDLE_PRAGMA | LINE_NUM | KEEP_OUTPUT;
  ls.flags |= MACRO_VAARG | CPLUSPLUS_COMMENTS | HANDLE_TRIGRAPHS; 
  ls.flags &= ~DISCARD_COMMENTS;

  // step 8 -- prepare I/O
  ls.input = fopen(input, "r");
  if (!ls.input) throw std::runtime_error("unable to open input for reading");
  if (output && output[0] == '-' && output[1] == 0) ls.output = stdout;
  else if (output)
  {
    ls.output = fopen(output, "w");
    if (!ls.output)
    {

      fclose(ls.input);
      throw std::runtime_error("unable to open output for writing");
    }
  }
  else
  {
    // no output
    ls.output = 0;
    ls.flags &= ~KEEP_OUTPUT;
  }

  // step 9 -- handle preprocessor flags: -D, -U, and -I
  for (std::vector<std::string>::const_iterator i = flags.begin(); i != flags.end(); ++i)
  {
    // -D
    if (*i == "-D") define_macro(&ls, const_cast<char*>((++i)->c_str()));
    else if (i->substr(0, 2) == "-D") define_macro(&ls, const_cast<char*>(i->substr(2).c_str()));
    // -U
    else if (*i == "-U") undef_macro(&ls, const_cast<char*>((++i)->c_str()));
    else if (i->substr(0, 2) == "-U") undef_macro(&ls, const_cast<char*>(i->substr(2).c_str()));
    // -I
    else if (*i == "-I") add_incpath(const_cast<char*>((++i)->c_str()));
    else if (i->substr(0, 2) == "-I") add_incpath(const_cast<char*>(i->substr(2).c_str()));
  }
  // step 10 -- we are a lexer and we want CONTEXT tokens
  enter_file(&ls, ls.flags, 0);

  // read tokens until end-of-input is reached -- errors (non-zero
  // retur nvalues different from CPPERR_EOF) are ignored
  int r;
  while ((r = lex(&ls)) < CPPERR_EOF)
  {
    if (r)
    {
      // error condition -- no token was retrieved
      continue;
    }
    if (ls.ctok->type == PRAGMA) handle_pragma(ls);
    else if (ls.ctok->type == CONTEXT) handle_context(ls);
    else if (ls.ctok->type == NEWLINE) handle_newline(ls);
    else handle_token(ls);
  }
  end_file_preamble();

  // give back memory and exit
  wipeout();
  if (ls.output && ls.output != stdout) fclose(ls.output);
  free_lexer_state(&ls);
}
