/*
  Copyright (C) 1997-2000 Shigeru Chiba, University of Tsukuba.

  Permission to use, copy, distribute and modify this software and   
  its documentation for any purpose is hereby granted without fee,        
  provided that the above copyright notice appear in all copies and that 
  both that copyright notice and this permission notice appear in 
  supporting documentation.

  Shigeru Chiba makes no representations about the suitability of this 
  software for any purpose.  It is provided "as is" without express or
  implied warranty.
*/
/*
  Copyright (c) 1995, 1996 Xerox Corporation.
  All Rights Reserved.

  Use and copying of this software and preparation of derivative works
  based upon this software are permitted. Any copy of this software or
  of any derivative work must include the above copyright notice of
  Xerox Corporation, this paragraph and the one after it.  Any
  distribution of this software or derivative works must comply with all
  applicable United States export control laws.

  This software is made available AS IS, and XEROX CORPORATION DISCLAIMS
  ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE, AND NOTWITHSTANDING ANY OTHER PROVISION CONTAINED HEREIN, ANY
  LIABILITY FOR DAMAGES RESULTING FROM THE SOFTWARE OR ITS USE IS
  EXPRESSLY DISCLAIMED, WHETHER ARISING IN CONTRACT, TORT (INCLUDING
  NEGLIGENCE) OR STRICT LIABILITY, EVEN IF XEROX CORPORATION IS ADVISED
  OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#include "PTree.hh" // for MopWarningMessage only !
#include <PTree/generation.hh>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <stdexcept>
#if !defined(_MSC_VER)
#include <sys/time.h>
#endif
// #include "Lexer.hh"
// #include "Class.hh"

#if (defined(sun) && defined(SUNOS4)) || defined(SUNOS5)
extern "C" 
{
  int gettimeofday(struct timeval*, void*);
};
#endif

namespace
{
const int MAX = 32;
PTree::Node **resultsArgs[MAX];
int resultsIndex;

const char *skip_spaces(const char *pat)
{
  while(*pat == ' ' || *pat == '\t') ++pat;
  return pat;
}

const char *integer_to_string(sint num, int& length)
{
  const int N = 16;
  static char buf[N];
  bool minus;

  int i = N - 1;
  if(num >= 0) minus = false;
  else
  {
    minus = true;
    num = -num;
  }

  buf[i--] = '\0';
  if(num == 0)
  {
    buf[i] = '0';
    length = 1;
    return &buf[i];
  }
  else
  {
    while(num > 0)
    {
      buf[i--] = '0' + char(num % 10);
      num /= 10;
    }

    if(minus) buf[i--] = '-';
    length = N - 2 - i;
    return &buf[i + 1];
  }
}

const char *match_word(PTree::Node *list, const char *pat)
{
  const char* str = list->position();
  size_t str_len = list->length();

  for(int j = 0; ; ++pat)
  {
    char c = *pat;
    switch(c)
    {
      case '\0' :
      case ' ' :
      case '\t' :
      case '[' :
      case ']' :
	if(j == str_len) return pat;
	else return 0;
      case '%' :
	c = *++pat;
	switch(c)
	{
	  case '[' :
	  case ']' :
	  case '%' :
	    if(j >= str_len || c != str[j++]) return 0;
	    break;
	  default :
	    if(j == str_len) return pat;
	    else return 0;
	}
	break;
      default :
	if(j >= str_len || c != str[j++]) return 0;
    }
  }
}

const char *match_pat(PTree::Node *list, const char *pat);

const char *match_list(PTree::Node *list, const char *pat)
{
  char c, d;
  pat = skip_spaces(pat);
  while((c = *pat) != '\0')
  {
    if(c == ']')
      if(list == 0)
	return(pat + 1);
      else
	return 0;
    else if(c == '%' && (d = pat[1], (d == 'r' || d == '_'))){
      /* %r or %_ */
      if(d == 'r') 
	*resultsArgs[resultsIndex++] = list;
      list = 0;
      pat = pat + 2;
    }
    else if(list == 0) return 0;
    else
    {
      pat = match_pat(list->car(), pat);
      if(pat == 0) return 0;
      list = list->cdr();
    }
    pat = skip_spaces(pat);
  }
  throw std::runtime_error("PTree::match(): unmatched bracket");
}

const char *match_pat(PTree::Node *list, const char *pat)
{
  switch(*pat)
  {
    case '[' :		/* [] means 0 */
      if(list != 0 && list->is_atom())
	return 0;
      else
	return match_list(list, pat + 1);
    case '%' :
      switch(pat[1])
      {
        case '?' :
	  *resultsArgs[resultsIndex++] = list;
	  return(pat + 2);
        case '*' :
	  return(pat + 2);
	case '_' :
	case 'r' :	/* %_ and %r must be appear in a list */
	  return 0;
	default :
	  break;
      }
  }
  if(list != 0 && list->is_atom())
    return match_word(list, pat);
  else return 0;
}

size_t count_args(const char *pat)
{
  size_t n = 0;
  for(char c = *pat; c != '\0'; c = *++pat)
    if(c == '%')
    {
      c = *++pat;
      if(c == '?' || c == 'r') ++n;
    }
  return n;
}
}

namespace PTree
{

bool match(PTree::Node *list, char *pattern, ...)
{
  va_list args;
  size_t n = count_args(pattern);
  if(n >= MAX)
    throw std::runtime_error("PTree::match(): too many arguments");

  va_start(args, pattern);
  for(size_t i = 0; i < n; ++i)
    resultsArgs[i] = va_arg(args, PTree::Node **);

  va_end(args);

  resultsIndex = 0;
  const char *pat = skip_spaces(pattern);
  pat = match_pat(list, pat);
  if(pat == 0) return false;
  else
  {
    pat = skip_spaces(pat);
    if(*pat == '\0') return true;
    else
    {
      MopWarningMessage("PTree::Node::Match()", "[ ] are forgot?");
      MopMoreWarningMessage(pattern);
      return false;
    }
  }
}

Node *gen_sym()
{
  static char head[] = "_sym";
  static int seed = 1;
  int len1, len2;

  integer_to_string(seed, len1);

#if !defined(_MSC_VER) && !defined(__WIN32__)
  struct timeval time;
  gettimeofday(&time, 0);
  uint rnum = (time.tv_sec * 10 + time.tv_usec / 100) & 0xffff;
#else
  static uint time = 0;
  time++;
  uint rnum = time & 0xffff;
#endif
  const char *num = integer_to_string(rnum, len2);

  int size = len1 + len2 + sizeof(head) - 1 + 1;
  char *name = new (GC) char[size];
  memmove(name, head, sizeof(head) - 1);
  memmove(&name[sizeof(head) - 1], num, len2);
  name[sizeof(head) - 1 + len2] = '_';
  num = integer_to_string(seed++, len1);
  memmove(&name[sizeof(head) - 1 + len2 + 1], num, len1);
  return new PTree::Atom(name, size);
}

Node *make(const char *pat, ...)
{
  va_list args;
  const int N = 4096;
  static char buf[N];
  char c;
  int len;
  const char* ptr;
  PTree::Node *p;
  PTree::Node *q;
  int i = 0, j = 0;
  PTree::Node *result = 0;

  va_start(args, pat);
  while((c = pat[i++]) != '\0')
    if(c == '%')
    {
      c = pat[i++];
      if(c == '%') buf[j++] = c;
      else if(c == 'd')
      {
	ptr = integer_to_string(va_arg(args, int), len);
	memmove(&buf[j], ptr, len);
	j += len;
      }
      else if(c == 's')
      {
	ptr = va_arg(args, char*);
	len = strlen(ptr);
	memmove(&buf[j], ptr, len);
	j += len;
      }
      else if(c == 'c')
	buf[j++] = va_arg(args, int);
      else if(c == 'p')
      {
	p = va_arg(args, PTree::Node *);
	if(p == 0)
	  /* ignore */;
	else if(p->is_atom())
	{
	  memmove(&buf[j], p->position(), p->length());
	  j += p->length();
	}
	else
	{   
	  if(j > 0)
	    q = list(new PTree::DupAtom(buf, j), p);
	  else q = list(p);
	  j = 0;
	  result = nconc(result, q);
	}
      }
      else throw std::runtime_error("PTree::make(): invalid format");
    }
    else buf[j++] = c;
  va_end(args);

  if(j > 0)
    if(result == 0)
      result = new PTree::DupAtom(buf, j);
    else
      result = snoc(result, new PTree::DupAtom(buf, j));

  return result;
}

bool reify(Node *n, unsigned int& value)
{
  if(!n->is_atom()) return false;

  const char *p = n->position();
  int len = n->length();
  value = 0;
  if(len > 2 && *p == '0' && is_xletter(p[1]))
  {
    for(int i = 2; i < len; ++i)
    {
      char c = p[i];
      if(is_digit(c)) value = value * 0x10 + (c - '0');
      else if('A' <= c && c <= 'F') value = value * 0x10 + (c - 'A' + 10);
      else if('a' <= c && c <= 'f') value = value * 0x10 + (c - 'a' + 10);
      else if(is_int_suffix(c)) break;
      else return false;
    }
    return true;
  }
  else if(len > 0 && is_digit(*p))
  {
    for(int i = 0; i < len; ++i)
    {
      char c = p[i];
      if(is_digit(c)) value = value * 10 + c - '0';
      else if(is_int_suffix(c)) break;
      else return false;
    }
    return true;
  }
  else return false;
}

bool reify(Node *n, const char*& str)
{
  if(!n->is_atom()) return false;
  const char *p = n->position();
  size_t l = n->length();
  if(*p != '"') return false;
  else
  {
    char *tmp = new(GC) char[l];
    char *sp = tmp;
    for(size_t i = 1; i < l; ++i)
      if(p[i] != '"')
      {
	*sp++ = p[i];
	if(p[i] == '\\' && i + 1 < l) *sp++ = p[++i];
      }
      else while(++i < l && p[i] != '"');
    *sp = '\0';
    str = tmp;
    return true;
  }
}

Node *qmake(const char*)
{
  throw std::runtime_error("PTree::qmake(): the metaclass must be compiled by OpenC++.");
}

// class PtreeHead	--- this is used to implement PTree::Node::qMake().

PTree::Head &PTree::Head::operator + (PTree::Node *p)
{
    ptree = append(ptree, p);
    return *this;
}

PTree::Head &PTree::Head::operator + (const char* str)
{
  if(*str != '\0') ptree =  append(ptree, str, strlen(str));
  return *this;
}

PTree::Head &PTree::Head::operator + (char c)
{
    ptree = append(ptree, &c, 1);
    return *this;
}

PTree::Head &PTree::Head::operator + (int n)
{
    int len;
    const char *str = integer_to_string(n, len);
    ptree = append(ptree, str, len);
    return *this;
}

PTree::Node *PTree::Head::append(PTree::Node *lst, PTree::Node *tail)
{
    PTree::Node *last;
    PTree::Node *p;
    PTree::Node *q;

    if(tail == 0)
	return lst;

    if(!tail->is_atom() && PTree::length(tail) == 1){
	tail = tail->car();
	if(tail == 0)
	    return lst;
    }

    if(tail->is_atom() && lst != 0){
	last = PTree::last(lst);
	if(last != 0){
	    p = last->car();
	    if(p != 0 && p->is_atom()){
		q = new PTree::DupAtom(p->position(), p->length(),
				 tail->position(), tail->length());
		last->set_car(q);
		return lst;
	    }
	}
    }

    return PTree::snoc(lst, tail);
}

PTree::Node *PTree::Head::append(PTree::Node *lst, const char *str, size_t len)
{
  if(lst)
  {
    PTree::Node *last = PTree::last(lst);
    if(last)
    {
      PTree::Node *p = last->car();
      if(p && p->is_atom())
      {
	PTree::Node *q = new PTree::DupAtom(p->position(), p->length(), str, len);
	last->set_car(q);
	return lst;
      }
    }
  }
  return PTree::snoc(lst, new DupAtom(str, len));
}

}
