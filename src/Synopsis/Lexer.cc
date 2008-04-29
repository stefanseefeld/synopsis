//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "Synopsis/Lexer.hh"
#include "Synopsis/Buffer.hh"
#include "Synopsis/process_pragma.hh"
#include <iostream>
#include <string>
#include <cassert>

using namespace Synopsis;

Lexer::Lexer(Buffer *buffer, int tokenset)
  : buffer_(buffer),
    token_(buffer_->ptr(), 0, '\n')
{
  keywords_["asm"] = Token::ATTRIBUTE;
  keywords_["auto"] = Token::AUTO;
  keywords_["break"] = Token::BREAK;
  keywords_["case"] = Token::CASE;
  keywords_["char"] = Token::CHAR;
  // FIXME: Add support for _Complex to Parser.
  keywords_["_Complex"] = Token::Ignore;
  keywords_["const"] = Token::CONST;
  keywords_["continue"] = Token::CONTINUE;
  keywords_["default"] = Token::DEFAULT;
  keywords_["do"] = Token::DO;
  keywords_["double"] = Token::DOUBLE;
  keywords_["else"] = Token::ELSE;
  keywords_["enum"] = Token::ENUM;
  keywords_["extern"] = Token::EXTERN;
  keywords_["float"] = Token::FLOAT;
  keywords_["for"] = Token::FOR;
  keywords_["goto"] = Token::GOTO;
  keywords_["if"] = Token::IF;
  keywords_["inline"] = Token::INLINE;
  keywords_["int"] = Token::INT;
  keywords_["long"] = Token::LONG;
  keywords_["register"] = Token::REGISTER;
  keywords_["return"] = Token::RETURN;
  keywords_["short"] = Token::SHORT;
  keywords_["signed"] = Token::SIGNED;
  keywords_["sizeof"] = Token::SIZEOF;
  keywords_["static"] = Token::STATIC;
  keywords_["struct"] = Token::STRUCT;
  keywords_["switch"] = Token::SWITCH;
  keywords_["typedef"] = Token::TYPEDEF;
  keywords_["union"] = Token::UNION;
  keywords_["unsigned"] = Token::UNSIGNED;
  keywords_["void"] = Token::VOID;
  keywords_["volatile"] = Token::VOLATILE;
  keywords_["while"] = Token::WHILE;
  if (tokenset & CXX)
  {
    keywords_["bool"] = Token::BOOLEAN;
    keywords_["catch"] = Token::CATCH;
    keywords_["const_cast"] = Token::CONST_CAST;
    keywords_["class"] = Token::CLASS;
    keywords_["delete"] = Token::DELETE;
    keywords_["dynamic_cast"] = Token::DYNAMIC_CAST;
    keywords_["explicit"] = Token::EXPLICIT;
    keywords_["export"] = Token::EXPORT;
    keywords_["false"] = Token::Constant;
    keywords_["friend"] = Token::FRIEND;
    keywords_["mutable"] = Token::MUTABLE;
    keywords_["namespace"] = Token::NAMESPACE;
    keywords_["new"] = Token::NEW;
    keywords_["reinterpret_cast"] = Token::REINTERPRET_CAST;
    keywords_["operator"] = Token::OPERATOR;
    keywords_["private"] = Token::PRIVATE;
    keywords_["protected"] = Token::PROTECTED;
    keywords_["public"] = Token::PUBLIC;
    keywords_["static_cast"] = Token::STATIC_CAST;
    keywords_["template"] = Token::TEMPLATE;
    keywords_["this"] = Token::THIS;
    keywords_["throw"] = Token::THROW;
    keywords_["true"] = Token::Constant;
    keywords_["try"] = Token::TRY;
    keywords_["typeid"] = Token::TYPEID;
    keywords_["typename"] = Token::TYPENAME;
    keywords_["using"] = Token::USING;
    keywords_["virtual"] = Token::VIRTUAL;
    keywords_["wchar_t"] = Token::WCHAR;
  }
  if (tokenset & GCC)
  {
    keywords_["__alignof__"] = Token::SIZEOF;
    keywords_["__asm"] = Token::ATTRIBUTE;
    keywords_["__asm__"] = Token::ATTRIBUTE;
    keywords_["__attribute__"] = Token::ATTRIBUTE;
    keywords_["__builtin_offsetof"] = Token::OFFSETOF;
    keywords_["__builtin_va_arg"] = Token::EXTENSION; // Is this correct ?
    keywords_["__complex__"] = Token::Ignore;
    keywords_["__const"] = Token::CONST;
    keywords_["__extension__"] = Token::EXTENSION;
    keywords_["__imag__"] = Token::Ignore;
    keywords_["__inline"] = Token::INLINE;
    keywords_["__inline__"] = Token::INLINE;
    keywords_["__real__"] = Token::Ignore;
    keywords_["__restrict"] = Token::Ignore;
    keywords_["__restrict__"] = Token::Ignore;
    keywords_["__signed"] = Token::SIGNED;
    keywords_["__signed__"] = Token::SIGNED;
    keywords_["typeof"] = Token::TYPEOF;
    keywords_["__typeof"] = Token::TYPEOF;
    keywords_["__typeof__"] = Token::TYPEOF;
  }
  if (tokenset & MSVC)
  {
    keywords_["cdecl"] = Token::Ignore;
    keywords_["_cdecl"] = Token::Ignore;
    keywords_["__cdecl"] = Token::Ignore;
    keywords_["_fastcall"] = Token::Ignore;
    keywords_["__fastcall"] = Token::Ignore;
    keywords_["_stdcall"] = Token::Ignore;
    keywords_["__stdcall"] = Token::Ignore;
    keywords_["__thiscall"] = Token::Ignore;
    keywords_["_based"] = Token::Ignore;
    keywords_["__based"] = Token::Ignore;
    keywords_["_asm"] = Token::ASM;
    keywords_["__asm"] = Token::ASM;
    keywords_["_inline"] = Token::INLINE;
    keywords_["__inline"] = Token::INLINE;
    keywords_["__declspec"] = Token::DECLSPEC;
    keywords_["__pragma"] = Token::PRAGMA;
    keywords_["__int8"] = Token::CHAR;
    keywords_["__int16"] = Token::SHORT;
    keywords_["__int32"] = Token::INT;
    keywords_["__int64"] = Token::INT64;
    keywords_["__w64"] = Token::Ignore;
  }
}

Token Lexer::get_token()
{
  if (!fill(1)) return Token();
  Token token = tokens_.front();
  tokens_.pop();
  return token;
}

Token::Type Lexer::look_ahead(size_t offset, Token &t)
{
  if (!fill(offset + 1)) return Token::BadToken;
  t = tokens_.at(offset);
  return t.type;
}

const char *Lexer::save()
{
  if (!fill(1)) throw std::runtime_error("unexpected EOF");
  Token current = tokens_.front();
  return current.ptr;
}

void Lexer::restore(const char *pos)
{
  token_.type = '\n';
  token_.ptr = buffer_->ptr();
  token_.length = 0;
  tokens_.clear();
  rewind(pos);
}

unsigned long Lexer::origin(const char *ptr, std::string &filename) const
{
  unsigned long line, column;
  buffer_->origin(ptr, filename, line, column);
  return line;
}

void Lexer::rewind(const char *p)
{
  buffer_->reset(p - buffer_->ptr());
}

Token::Type Lexer::read_token(const char *&ptr, size_t &length)
{
  Token::Type t = Token::BadToken;
  while(true)
  {
    t = read_line();
    if(t == Token::Ignore) continue;
    token_.type = t;

    if(t == Token::ATTRIBUTE)
    {
      skip_attribute();
      continue;
    }
    else if(t == Token::EXTENSION)
    {
      t = skip_extension(ptr, length);
      if(t == Token::Ignore) continue;
      else return t;
    }
    else if(t == Token::ASM)
    {
      skip_asm();
      continue;
    }
    else if(t == Token::DECLSPEC)
    {
      skip_declspec();
      continue;
    }
    else if(t == Token::PRAGMA)
    {
      skip_pragma();
      continue;
    }
    if(t != '\n') break;
  }

  ptr = token_.ptr;
  length = token_.length;
  return t;
}

bool Lexer::fill(size_t o)
{
  while (tokens_.size() < o)
  {
    Token t;
    t.type = read_token(t.ptr, t.length);
    if (t.type == Token::BadToken) return false;
    tokens_.push(t);
  }
  return true;
}

void Lexer::skip_attribute()
{
  char c;
  do { c = buffer_->get();}
  while(c != '(' && c != '\0');
  if (c == '\0') return;
  skip_paren();
}

Token::Type Lexer::skip_extension(const char *&ptr, size_t &length)
{
  ptr = token_.ptr;
  length = token_.length;

  char c;
  do { c = buffer_->get();}
  while(is_blank(c) || c == '\n');

  if(c != '(')
  {
    buffer_->unget();
    return Token::Ignore; // if no (..) follows, ignore __extension__
  }
  skip_paren();
  return Token::Identifier; // regards it as the identifier __extension__
}

inline bool check_end_of_instruction(Buffer *buffer, char c, const char *delimiter)
{
  if (c == '\0') return true;
  if (strchr(delimiter, c))
  {
    buffer->unget();
    return true;
  }
  return false;
}

void Lexer::skip_paren()
{
  size_t i = 1;
  do
  {
    char c = buffer_->get();
    if (c == '\0') return;
    if(c == '(') ++i;
    else if(c == ')') --i;
  } while(i > 0);
}

void Lexer::process_directive()
{
  char c = ' ';
  while(is_blank(c)) c = buffer_->get();
  if (is_letter(c))
  {
    unsigned long top = buffer_->position();
    char const *ptr = buffer_->ptr(top);
    do { c = buffer_->get();}
    while(is_letter(c) || is_digit(c));
    size_t length = static_cast<size_t>(buffer_->position() - top);
    if (length == 6 && strncmp(ptr, "pragma", 6) == 0)
    {
      while(c != '\n' && c != '\0') c = buffer_->get();
      length = static_cast<size_t>(buffer_->position() - top);
      process_pragma(std::string(ptr + 6, length - 6));
      return;
    }
  }
  else ; // FIXME: issue a parse error: no directive found.
  while(c != '\n' && c != '\0') c = buffer_->get();
}

/* You can have the following :

   Just count the '{' and '}' and it should be ok
   __asm { mov ax,1
           mov bx,1 }

   Stop when EOL found. Note that the first ';' after
   an __asm instruction is an ASM comment !
   int v; __asm mov ax,1 __asm mov bx,1; v=1;

   Stop when '}' found
   if (cond) {__asm mov ax,1 __asm mov bx,1}

   and certainly more...
*/
void Lexer::skip_asm()
{
  char c;

  do
  {
    c = buffer_->get();
    if (check_end_of_instruction(buffer_, c, "")) return;
  }
  while(is_blank(c) || c == '\n');

  if(c == '{')
  {
    size_t i = 1;
    do
    {
      c = buffer_->get();
      if (check_end_of_instruction(buffer_, c, "")) return;
      if(c == '{') ++i;
      else if(c == '}') --i;
    } while(i > 0);
  }
  else
  {
    while(true)
    {
      if (check_end_of_instruction(buffer_, c, "}\n")) return;
      c = buffer_->get();
    }
  }
}

void Lexer::skip_declspec()
{
  char c;
  do
  {
    c = buffer_->get();
    if (check_end_of_instruction(buffer_, c, "")) return;
  } while(is_blank(c));

  if (c == '(')
  {
    size_t i = 1;
    do
    {
      c = buffer_->get();
      if (check_end_of_instruction(buffer_, c, "};")) return;
      if(c == '(') ++i;
      else if(c == ')') --i;
    } while(i > 0);
  }
}

void Lexer::skip_pragma()
{
  char c = get_next_non_white_char();

  if (c == '(')
  {
    size_t i = 1;
    do
    {
      c = buffer_->get();
      if (check_end_of_instruction(buffer_, c, "};")) return;
      if(c == '(') ++i;
      else if(c == ')') --i;
    } while(i > 0);

    c = get_next_non_white_char(); // assume ';'
  }
}

char Lexer::get_next_non_white_char()
{
  char c;
  while(true)
  {
    do { c = buffer_->get();}
    while(is_blank(c));

    if(c != '\\') break;

    c = buffer_->get();
    if(c != '\n' && c!= '\r') 
    {
      buffer_->unget();
      break;
    }
  }
  return c;
}

Token::Type Lexer::read_line()
{
  char c = get_next_non_white_char();
  unsigned long top = buffer_->position();
  token_.ptr = buffer_->ptr(top);
  if(c == '\0')
  {
    buffer_->unget();
    return '\0';
  }
  else if(c == '\n') return '\n';
  else if(c == '#' && token_.type == '\n')
  {
    process_directive();
    return '\n';
  }
  else if(c == '\'' || c == '"')
  {
    if(c == '\'')
    {
      if(read_char_const(top)) return Token::CharConst;
    }
    else
    {
      if(read_str_const(top)) return Token::StringL;
    }
    buffer_->reset(top + 1);
    token_.length = 1;
    return single_char_op(c);
  }
  else if(is_digit(c)) return read_number(c, top);
  else if(c == '.')
  {
    c = buffer_->get();
    if(is_digit(c)) return read_float(top);
    else
    {
      buffer_->unget();
      return read_separator('.', top);
    }
  }
  else if(is_letter(c))
  {
    if (c == 'L')
    {
      c = buffer_->get();
      if (c == '\'' || c == '"')
      {
	if (c == '\'')
	{
	  if (read_char_const(top+1))
	  {
	    ++token_.length;
	    return Token::WideCharConst;
	  }
	} 
	else
	{
	  if(read_str_const(top+1))
	  {
	    ++token_.length;
	    return Token::WideStringL;
	  }
	}
      }
      buffer_->reset(top);
    }
    return read_identifier(top);
  }
  else return read_separator(c, top);
}

bool Lexer::read_char_const(unsigned long top)
{
  while(true)
  {
    char c = buffer_->get();
    if(c == '\\')
    {
      c = buffer_->get();
      if(c == '\0') return false;
    }
    else if(c == '\'')
    {
      token_.length = static_cast<size_t>(buffer_->position() - top + 1);
      return true;
    }
    else if(c == '\n' || c == '\0') return false;
  }
}

/*
  If text is a sequence of string constants like:
	"string1" "string2"  L"string3"
  then the string constants are delt with as a single constant.
*/
bool Lexer::read_str_const(unsigned long top)
{
  // Skip the L if there is one
  if (buffer_->at(top) == 'L') buffer_->get();
  while(true)
  {
    char c = buffer_->get();
    if(c == '\\')
    {
      c = buffer_->get();
      if(c == '\0') return false;
    }
    else if(c == '"')
    {
      // We are past one string literal token now.
      // Any following whitespace needs to be skipped
      // before looking for anything else.
      unsigned long pos = buffer_->position() + 1;
      while (true)
      {
	int nline = 0;
	// Consume whitespace.
	do
	{
	  c = buffer_->get();
	  if(c == '\n') ++nline;
	} while(is_blank(c) || c == '\n');
	// Consume comment.
	if (c == '/')
	{
	  char d = buffer_->get();
	  if (d == '/' || d == '*')
	    read_comment(d, buffer_->position() - 2);
	  else
	  {
	    buffer_->unget();
	    break;
	  }
	}
	else break;
      }
      if(c == '"')
	/* line_number += nline; */ ;
      else
      {
	token_.length = static_cast<size_t>(pos - top);
	buffer_->reset(pos);
	return true;
      }
    }
    else if(c == '\n' || c == '\0') return false;
  }
}

Token::Type Lexer::read_number(char c, unsigned long top)
{
  char c2 = buffer_->get();

  if(c == '0' && is_xletter(c2))
  {
    do { c = buffer_->get();}
    while(is_hexdigit(c));
    while(is_int_suffix(c)) c = buffer_->get();

    buffer_->unget();
    token_.length = static_cast<size_t>(buffer_->position() - top + 1);
    return Token::Constant;
  }

  while(is_digit(c2)) c2 = buffer_->get();

  if(is_int_suffix(c2))
    do { c2 = buffer_->get();}
    while(is_int_suffix(c2));
  else if(c2 == '.') return read_float(top);
  else if(is_eletter(c2))
  {
    buffer_->unget();
    return read_float(top);
  }

  buffer_->unget();
  token_.length = static_cast<size_t>(buffer_->position() - top + 1);
  return Token::Constant;
}

Token::Type Lexer::read_float(unsigned long top)
{
  char c;
    
  do { c = buffer_->get();}
  while(is_digit(c));
  if(is_float_suffix(c))
    do { c = buffer_->get();}
    while(is_float_suffix(c));
  else if(is_eletter(c))
  {
    unsigned long p = buffer_->position();
    c = buffer_->get();
    if(c == '+' || c == '-')
    {
      c = buffer_->get();
      if(!is_digit(c))
      {
	buffer_->reset(p);
	token_.length = static_cast<size_t>(p - top);
	return Token::Constant;
      }
    }
    else if(!is_digit(c))
    {
      buffer_->reset(p);
      token_.length = static_cast<size_t>(p - top);
      return Token::Constant;
    }

    do { c = buffer_->get();}
    while(is_digit(c));

    while(is_float_suffix(c)) c = buffer_->get();
  }
  buffer_->unget();
  token_.length = static_cast<size_t>(buffer_->position() - top + 1);
  return Token::Constant;
}

Token::Type Lexer::read_identifier(unsigned long top)
{
  char c;

  do { c = buffer_->get();}
  while(is_letter(c) || is_digit(c));
  token_.length = static_cast<size_t>(buffer_->position() - top);
  buffer_->unget();
  return screen(buffer_->ptr(top), token_.length);
}

Token::Type Lexer::screen(const char *identifier, size_t length)
{
  Dictionary::iterator i = keywords_.find(std::string(identifier, length));
  if (i != keywords_.end()) return i->second;
  return Token::Identifier;
}

Token::Type Lexer::read_separator(char c, unsigned long top)
{
  char c1 = buffer_->get();
  if (c1 == '\0') return Token::BadToken;
  token_.length = 2;
  if(c1 == '=')
  {
    switch(c)
    {
      case '*' :
      case '/' :
      case '%' :
      case '+' :
      case '-' :
      case '&' :
      case '^' :
      case '|' : return Token::AssignOp;
      case '=' :
      case '!' : return Token::EqualOp;
      case '<' :
      case '>' : return Token::RelOp;
      default : 
	buffer_->unget();
	token_.length = 1;
	return single_char_op(c);
    }
  }
  else if(c == c1)
  {
    switch(c)
    {
      case '<' :
      case '>' :
	if(buffer_->get() != '=')
	{
	  buffer_->unget();
	  return Token::ShiftOp;
	}
	else
	{
	  token_.length = 3;
	  return Token::AssignOp;
	}
      case '|' : return Token::LogOrOp;
      case '&' : return Token::LogAndOp;
      case '+' :
      case '-' : return Token::IncOp;
      case ':' : return Token::Scope;
      case '.' :
	if(buffer_->get() == '.')
	{
	  token_.length = 3;
	  return Token::Ellipsis;
	}
	else buffer_->unget();
      case '/' : return read_comment(c1, top);
      default :
	buffer_->unget();
	token_.length = 1;
	return single_char_op(c);
    }
  }
  else if(c == '.' && c1 == '*') return Token::PmOp;
  else if(c == '-' && c1 == '>')
    if(buffer_->get() == '*')
    {
      token_.length = 3;
      return Token::PmOp;
    }
    else
    {
      buffer_->unget();
      return Token::ArrowOp;
    }
  else if(c == '/' && c1 == '*') return read_comment(c1, top);
  else
  {
    buffer_->unget();
    token_.length = 1;
    return single_char_op(c);
  }

  std::cerr << "*** An invalid character has been found! ("
	    << (int)c << ',' << (int)c1 << ")\n";
  return Token::BadToken;
}

Token::Type Lexer::single_char_op(unsigned char c)
{
  /* !"#$%&'()*+,-./0123456789:;<=>? */
  static char valid[] = "x   xx xxxxxxxx          xxxxxx";

  if('!' <= c && c <= '?' && valid[c - '!'] == 'x') return c;
  else if(c == '[' || c == ']' || c == '^') return c;
  else if('{' <= c && c <= '~') return c;
  else if(c == '#') 
  {
    // Skip to end of line
    do{ c = buffer_->get();}
    while(c != '\n' && c != '\0');
    return Token::Ignore;
  }
  else
  {
    std::cerr << "*** An invalid character has been found! ("<<(char)c<<")"<< std::endl;
    return Token::BadToken;
  }
}

Token::Type Lexer::read_comment(char c, unsigned long top)
{
  unsigned long len = 0;
  if (c == '*')	// a nested C-style comment is prohibited.
    do 
    {
      c = buffer_->get();
      if (c == '*')
      {
	c = buffer_->get();
	if (c == '/')
	{
	  len = 1;
	  break;
	}
	else buffer_->unget();
      }
    } while(c != '\0');
  else /* if (c == '/') */
    do { c = buffer_->get();}
    while(c != '\n' && c != '\0');

  len += buffer_->position() - top;
  token_.length = static_cast<size_t>(len);
  comments_.push_back(Token(buffer_->ptr(top), token_.length, Token::Comment));
  return Token::Ignore;
}

Lexer::Comments Lexer::get_comments()
{
  Comments c = comments_;
  comments_.clear();
  return c;
}

