
/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o

    CTool Library
    Copyright (C) 1998-2001	Shaun Flisakowski

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
    o+
    o+     File:         parseenv.h
    o+
    o+     Programmer:   Shawn Flisakowski
    o+     Date:         Apr 7, 1995
    o+
    o+     The parsing envirornment.
    o+
    o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

#ifndef _ParseEnv_hh
#define _ParseEnv_hh

#include <config.hh>
#include <Context.hh>
#include <Lexer.hh>
#include <cstdio>
#include <string>

#define    YYBUFF_SIZE    4096

    /*  Redefinition - original provided by flex/lex */
#ifndef    YY_BUFFER_STATE_DEFD
typedef  struct yy_buffer_state  *YY_BUFFER_STATE;
#endif

class TransUnit;
class Base;
class SymEntry;

struct ParseEnv
{
  ParseEnv(std::istream *instream, std::ostream *errstream, const std::string &fname);
  ~ParseEnv();

  TransUnit         *transUnit;   // Pointer to the parsed translation unit

  std::string        realfile;    // The file _really_ being parsed.
  int                in_realfile;
  std::string        filename;

  std::istream       *yyinstream;  // A pointer to an open file
  std::ostream       *yyerrstream;

  int                yylineno;    // Line number
  int                yycolno;     // Column number
  int                yynxtcol;    // next Column number

  YY_BUFFER_STATE    yybuff;      // A buffer for the lexer
  
  ParseCtxt         *parseCtxt;

  std::string        line;
  std::string        last_line;
    
  // for the lexer
  int   err_top_level;
  int   err_cnt;
  int   err_tok;
  bool  possibleType;
  bool  possibleTag;
  bool  possibleLabel;
  
  bool file_ppln;
  int  in_preprocess;
  int  in_charconst;
  
  int  gcc2_paren_cnt;
  
  char  tokn_buff[MAX_TOKN_LEN];
  char *tokn_end;
  char *tokn_ptr;
  
  bool  isWide;
    
  bool cpp_comment_warn;
};

#endif
