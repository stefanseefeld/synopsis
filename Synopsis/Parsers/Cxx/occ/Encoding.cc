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
  This encoding is also interpreted by TypeInfo.  If you modify this
  file, check typeinfo.{h,cc} out as well.

  'b' boolean
  'c' char
  'w' wchar_t
  'i' int (signed, unsigned)
  's' short (short int)
  'l' long (long int)
  'j' long long
  'f' float
  'd' double
  'r' long double
  'v' void

  'T' template class (e.g. Foo<int,char> ==> T[3]Foo[2]ic.  [2] means
      the length of "ic".  It doesn't mean the number of template
      arguments.
  'e' ...
  '?' no return type.  the return type of constructors
  '*' non-type template parameter

  'S' singned
  'U' unsigned
  'C' const
  'V' volatile

  'P' pointer
  'R' reference
  'A' []
  'F' function (e.g. char foo(int) ==> Fi_c)
  'M' pointer to member (e.g. Type::* ==> M[4]Type)

  'Q' qualified class (e.g. X::YY ==> Q[2][1]X[2]YY, ::YY ==> Q[2][0][2]YY)

  [x] means (0x80 + x)
  '0' means :: (global scope)

  Special function names:

  operator + ==> +
  operator new[] ==> new[]
  operator <type> ==> @<encoded type>		cast operator
*/

#include <cstring>
#include <iostream>
#include "Encoding.hh"
#include "Lexer.hh"
#include "PTree.hh"
#include "Environment.hh"
#include "Class.hh"
#include "TypeInfo.hh"

PTree::Node *Encoding::bool_t = 0;
PTree::Node *Encoding::char_t = 0;
PTree::Node *Encoding::wchar_t_t = 0;
PTree::Node *Encoding::int_t = 0;
PTree::Node *Encoding::short_t = 0;
PTree::Node *Encoding::long_t = 0;
PTree::Node *Encoding::float_t = 0;
PTree::Node *Encoding::double_t = 0;
PTree::Node *Encoding::void_t = 0;

PTree::Node *Encoding::signed_t = 0;
PTree::Node *Encoding::unsigned_t = 0;
PTree::Node *Encoding::const_t = 0;
PTree::Node *Encoding::volatile_t = 0;

PTree::Node *Encoding::operator_name = 0;
PTree::Node *Encoding::new_operator = 0;
PTree::Node *Encoding::anew_operator = 0;
PTree::Node *Encoding::delete_operator = 0;
PTree::Node *Encoding::adelete_operator = 0;

PTree::Node *Encoding::star = 0;
PTree::Node *Encoding::ampersand = 0;
PTree::Node *Encoding::comma = 0;
PTree::Node *Encoding::dots = 0;
PTree::Node *Encoding::scope = 0;
PTree::Node *Encoding::tilder = 0;
PTree::Node *Encoding::left_paren = 0;
PTree::Node *Encoding::right_paren = 0;
PTree::Node *Encoding::left_bracket = 0;
PTree::Node *Encoding::right_bracket = 0;
PTree::Node *Encoding::left_angle = 0;
PTree::Node *Encoding::right_angle = 0;

const int DigitOffset = 0x80;

void Encoding::do_init_static()
{
  Encoding::bool_t = new PTree::AtomBOOLEAN("bool", 4);
    Encoding::char_t = new PTree::AtomCHAR("char", 4);
    Encoding::wchar_t_t = new PTree::AtomCHAR("wchar_t", 7);
    Encoding::int_t = new PTree::AtomINT("int", 3);
    Encoding::short_t = new PTree::AtomSHORT("short", 5);
    Encoding::long_t = new PTree::AtomLONG("long", 4);
    Encoding::float_t = new PTree::AtomFLOAT("float", 5);
    Encoding::double_t = new PTree::AtomDOUBLE("double", 6);
    Encoding::void_t = new PTree::AtomVOID("void", 4);

    Encoding::signed_t = new PTree::AtomSIGNED("signed", 6);
    Encoding::unsigned_t = new PTree::AtomUNSIGNED("unsigned", 8);
    Encoding::const_t = new PTree::AtomCONST("const", 5);
    Encoding::volatile_t = new PTree::AtomVOLATILE("volatile", 8);

    Encoding::operator_name = new PTree::Reserved("operator", 8);
    Encoding::new_operator = new PTree::Reserved("new", 3);
    Encoding::anew_operator = new PTree::Reserved("new[]", 5);
    Encoding::delete_operator = new PTree::Reserved("delete", 6);
    Encoding::adelete_operator = new PTree::Reserved("delete[]", 8);

    Encoding::star = new PTree::Atom("*", 1);
    Encoding::ampersand = new PTree::Atom("&", 1);
    Encoding::comma = new PTree::Atom(",", 1);
    Encoding::dots = new PTree::Atom("...", 3);
    Encoding::scope = new PTree::Atom("::", 2);
    Encoding::tilder = new PTree::Atom("~", 1);
    Encoding::left_paren = new PTree::Atom("(", 1);
    Encoding::right_paren = new PTree::Atom(")", 1);
    Encoding::left_bracket = new PTree::Atom("[", 1);
    Encoding::right_bracket = new PTree::Atom("]", 1);
    Encoding::left_angle = new PTree::Atom("<", 1);
    Encoding::right_angle = new PTree::Atom(">", 1);

}

void Encoding::Reset(Encoding& e)
{
    len = e.len;
    if(len > 0)
	memmove(name, e.name, len);
}

char* Encoding::Get()
{
    if(len == 0)
	return 0;
    else{
	char* s = new (GC) char[len + 1];
	memmove(s, name, len);
	s[len] = '\0';
	return s;
    }
}

void Encoding::print(std::ostream &os, const char *p)
{
  for (const unsigned char *ptr = reinterpret_cast<const unsigned char*>(p);
       *ptr != '\0'; ++ptr)
    if(*ptr < 0x80) os << reinterpret_cast<const char *>(*ptr);
    else os << reinterpret_cast<const char *>(*ptr - 0x80 + '0');
}

// GetBaseName() returns "Foo" if ENCODE is "Q[2][1]X[3]Foo", for example.
// If an error occurs, the function returns 0.

const char *Encoding::GetBaseName(const char *encode, int& len, Environment*& env)
{
  if(encode == 0)
  {
    len = 0;
    return 0;
  }

  Environment* e = env;
  const unsigned char* p = (const unsigned char*)encode;
  if(*p == 'Q')
  {
    int n = p[1] - 0x80;
    p += 2;
    while(n-- > 1)
    {
      int m = *p++;
      if(m == 'T')
	m = GetBaseNameIfTemplate(p, e);
      else if(m < 0x80)
      {		// error?
	len = 0;
	return 0;
      }
      else
      {			// class name
	m -= 0x80;
	if(m <= 0)
	{		// if global scope (e.g. ::Foo)
	  if(e != 0) e = e->GetBottom();
	}
	else e = ResolveTypedefName(e, (const char*)p, m);
      }
      p += m;
    }
    env = e;
  }
  if(*p == 'T')
  {		// template class
    int m = p[1] - 0x80;
    int n = p[m + 2] - 0x80;
    len = m + n + 3;
    return (const char*)p;
  }
  else if(*p < 0x80)
  {		// error?
    len = 0;
    return 0;
  }
  else
  {
    len = *p - 0x80;
    return (const char*)p + 1;
  }
}

Environment* Encoding::ResolveTypedefName(Environment* env,
					  const char* name, int len)
{
    TypeInfo tinfo;
    Bind* bind;
    Class* c = 0;

    if(env != 0)
	if (env->LookupType(name, len, bind) && bind != 0)
	    switch(bind->What()){
	    case Bind::isClassName :
		c = bind->ClassMetaobject();
		break;
	    case Bind::isTypedefName :
		bind->GetType(tinfo, env);
		c = tinfo.ClassMetaobject();
		/* if (c == 0) */
		    env = 0;
		break;
	    default :
		break;
	    }
	else if (env->LookupNamespace(name, len)) {
	    /* For the time being, we simply ignore name spaces.
	     * For example, std::T is identical to ::T.
	     */
	    env = env->GetBottom();
	}
	else
	    env = 0;		// unknown typedef name

    if(c != 0)
	return c->GetEnvironment();
    else
	return env;
}

int Encoding::GetBaseNameIfTemplate(const unsigned char* name, Environment*& e)
{
    int m = name[0] - 0x80;
    if(m <= 0)
	return name[1] - 0x80 + 2;

    Bind* b;
    if(e != 0 && e->LookupType((char*)&name[1], m, b))
	if(b != 0 && b->What() == Bind::isTemplateClass){
	    Class* c = b->ClassMetaobject();
	    if(c != 0){
		e = c->GetEnvironment();
		return m + (name[m + 1] - 0x80) + 2;
	    }
	}

    // the template name was not found.
    e = 0;
    return m + (name[m + 1] - 0x80) + 2;
}

const unsigned char* Encoding::GetTemplateArguments(const unsigned char* name, int& len)
{
    int m = name[0] - 0x80;
    if(m <= 0){
	len = name[1] - 0x80;
	return &name[2];
    }
    else{
	len = name[m + 1] - 0x80;
	return &name[m + 2];
    }
}

void Encoding::CvQualify(PTree::Node *cv1, PTree::Node *cv2)
{
    bool c = false, v = false;
    if(cv1 != 0 && !cv1->is_atom())
	while(cv1 != 0){
	    int kind = cv1->car()->What();
	    cv1 = cv1->cdr();
	    if(kind == Token::CONST)
		c = true;
	    else if(kind == Token::VOLATILE)
		v = true;
	}

    if(cv2 != 0 && !cv2->is_atom())
	while(cv2 != 0){
	    int kind = cv2->car()->What();
	    cv2 = cv2->cdr();
	    if(kind == Token::CONST)
		c = true;
	    else if(kind == Token::VOLATILE)
		v = true;
	}

    if(v)
	Insert('V');

    if(c)
	Insert('C');
}

void Encoding::GlobalScope()
{
    Append(DigitOffset);
}

// SimpleName() is also used for operator names

void Encoding::SimpleName(PTree::Node *id)
{
  AppendWithLen(id->position(), id->length());
}

// NoName() generates a internal name for no-name enum and class
// declarations.

void Encoding::NoName()
{
    static int i = 0;
    static unsigned char name[] = "`0000";
    int n = i++;
    name[1] = n / 1000 + '0';
    name[2] = (n / 100) % 10 + '0';
    name[3] = (n / 10) % 10 + '0';
    name[4] = n % 10 + '0';
    AppendWithLen((const char*)name, 5);
}

void Encoding::Template(PTree::Node *name, Encoding& args)
{
    Append('T');
    SimpleName(name);
    AppendWithLen(args);
}

void Encoding::Qualified(int n)
{
    if(len + 1 >= MaxNameLen)
	MopErrorMessage("Encoding::Qualified()",
			"too long encoded name");

    memmove(name + 2, name, len);
    len += 2;
    name[0] = 'Q';
    name[1] = (unsigned char)(DigitOffset + n);
}

void Encoding::Destructor(PTree::Node *class_name)
{
  size_t len = class_name->length();
  Append((unsigned char)(DigitOffset + len + 1));
  Append('~');
  Append(class_name->position(), len);
}

void Encoding::PtrOperator(int t)
{
    if(t == '*')
	Insert('P');
    else
	Insert('R');
}

void Encoding::PtrToMember(Encoding& encode, int n)
{
    if(n < 2)
	Insert((const char *)encode.name, encode.len);
    else{
	Insert((const char *)encode.name, encode.len);
	Insert((unsigned char)(DigitOffset + n));
	Insert('Q');
    }

    Insert('M');
}

void Encoding::CastOperator(Encoding& type)
{
    Append((unsigned char)(DigitOffset + type.len + 1));
    Append('@');
    Append((char*)type.name, type.len);
}

void Encoding::Insert(unsigned char c)
{
    if(len >= MaxNameLen)
	MopErrorMessage("Encoding::Insert()",
			"too long encoded name");

    if(len > 0)
	memmove(name + 1, name, len);

    ++len;
    name[0] = c;
}

void Encoding::Insert(const char *str, size_t n)
{
    if(len + n >= MaxNameLen)
	MopErrorMessage("Encoding::Insert()",
			"too long encoded name");

    if(len > 0)
	memmove(&name[n], name, len);

    memmove(name, str, n);
    len += n;
}

void Encoding::Append(unsigned char c)
{
    if(len >= MaxNameLen)
	MopErrorMessage("Encoding::Append()",
			"too long encoded name");

    name[len++] = c;
}

void Encoding::Append(const char *str, size_t n)
{
    if(len + n >= MaxNameLen)
	MopErrorMessage("Encoding::Append(char*,int)",
			"too long encoded name");

    memmove(&name[len], str, n);
    len += n;
}

void Encoding::AppendWithLen(const char *str, size_t n)
{
    if(len + n + 1 >= MaxNameLen)
	MopErrorMessage("Encoding::AppendWithLen()",
			"too long encoded name");

    name[len++] = (unsigned char)(DigitOffset + n);
    memmove(&name[len], str, n);
    len += n;
}

PTree::Node *Encoding::MakePtree(const unsigned char*& encoded, PTree::Node *decl)
{
  PTree::Node *cv;
  PTree::Node *typespec = 0;
  if(decl) decl = PTree::list(decl);

  while(true)
  {
    cv = 0;
    switch(*encoded++)
    {
      case 'b' :
	typespec = PTree::snoc(typespec, bool_t);
	goto finish;
      case 'c' :
	typespec = PTree::snoc(typespec, char_t);
	goto finish;
      case 'w' :
	typespec = PTree::snoc(typespec, wchar_t_t);
	goto finish;
      case 'i' :
	typespec = PTree::snoc(typespec, int_t);
	goto finish;
      case 's' :
	typespec = PTree::snoc(typespec, short_t);
	goto finish;
      case 'l' :
	typespec = PTree::snoc(typespec, long_t);
	goto finish;
	break;
      case 'j' :
	typespec = PTree::nconc(typespec, PTree::list(long_t, long_t));
	goto finish;
	break;
      case 'f' :
	typespec = PTree::snoc(typespec, float_t);
	goto finish;
	break;
      case 'd' :
	typespec = PTree::snoc(typespec, double_t);
	goto finish;
	break;
      case 'r' :
	typespec = PTree::nconc(typespec, PTree::list(long_t, double_t));
	goto finish;
      case 'v' :
	typespec = PTree::snoc(typespec, void_t);
	goto finish;
      case 'e' :
	return dots;
      case '?' :
	goto finish;
      case 'Q' :
	typespec = PTree::snoc(typespec, MakeQname(encoded));
	goto finish;
      case 'S' :
	typespec = PTree::snoc(typespec, signed_t);
	break;
      case 'U' :
	typespec = PTree::snoc(typespec, unsigned_t);
	break;
      case 'C' :
	if(*encoded == 'V')
	{
	  ++encoded;
	  cv = PTree::list(const_t, volatile_t);
	}
	else cv = PTree::list(const_t);
	goto const_or_volatile;
      case 'V' :
	cv = PTree::list(volatile_t);
      const_or_volatile :
	switch(*encoded)
	{
	  case 'M' :
	  case 'P' :
	  case 'R' :
	    decl = PTree::nconc(cv, decl);
	    break;
	  case 'F' :
	    ++encoded;
	    goto cv_function;
	    default :
	      typespec = PTree::nconc(cv, typespec);
	      break;
	}
	break;
      case 'M' :
        {
	  PTree::Node *ptr;
	  if(*encoded == 'Q') ptr = MakeQname(++encoded);
	  else ptr = MakeLeaf(encoded);
	  
	  ptr = PTree::list(ptr, scope, star);
	  decl = PTree::cons(ptr, decl);
	}
	goto pointer_or_reference;
      case 'P' :
	decl = PTree::cons(star, decl);
	goto pointer_or_reference;
      case 'R' :
	decl = PTree::cons(ampersand, decl);
      pointer_or_reference :
	if(*encoded == 'A' || *encoded == 'F')
	  decl = PTree::list(PTree::list(left_paren, decl, right_paren));
	break;
      case 'A' :
	decl = PTree::nconc(decl, PTree::list(left_bracket, right_bracket));
	break;
      case 'F' :
      cv_function :
        {
	  PTree::Node *args = 0;
	  while(*encoded != '\0')
	  {
	    if(*encoded == '_')
	    {
	      ++encoded;
	      break;
	    }
	    else if(*encoded == 'v')
	    {
	      encoded += 2;
	      break;
	    }
	    if(args != 0) args = PTree::snoc(args, comma);
	    args = PTree::snoc(args, MakePtree(encoded, 0));
	  }
	  decl = PTree::nconc(decl, PTree::list(left_paren, args, right_paren));
	  if(cv) decl = PTree::nconc(decl, cv);
	}
	break;
      case '\0' :
	goto finish;
      case 'T' :
	{
	  PTree::Node *tlabel = MakeLeaf(encoded);      
	  PTree::Node *args = 0;
	  int n = *encoded++ - DigitOffset;
	  const unsigned char* stop = encoded + n;
	  while(encoded < stop)
	  {
	    if(args) args = PTree::snoc(args, comma);
	    args = PTree::snoc(args, MakePtree(encoded, 0));
	  }
	  tlabel = PTree::list(tlabel, PTree::list(left_angle, args, right_angle));
	  typespec = PTree::nconc(typespec, tlabel);
	  goto finish;
	}
      case '*' :
	goto error;
      default :
	if(*--encoded >= DigitOffset)
	{
	  if(typespec == 0) typespec = MakeLeaf(encoded);
	  else typespec = PTree::snoc(typespec, MakeLeaf(encoded));
	  goto finish;
	}
      error :
	throw std::runtime_error("Encoding::MakePtree(): sorry, cannot handle this type");
	break;
    }
  }
 finish :
  return PTree::list(typespec, decl);
}

PTree::Node *Encoding::MakeQname(const unsigned char*& encoded)
{
  int n = *encoded++ - DigitOffset;
  PTree::Node *qname = 0;
  while(n-- > 0)
  {
    PTree::Node *leaf = MakeLeaf(encoded);
    if(leaf) qname = PTree::snoc(qname, leaf);
    if(n > 0) qname = PTree::snoc(qname, scope);
  }
  return qname;
}

PTree::Node *Encoding::MakeLeaf(const unsigned char*& encoded)
{
  PTree::Node *leaf;
  int len = *encoded++ - DigitOffset;
  if(len > 0) leaf = new PTree::Atom((const char*)encoded, len);
  else leaf = 0;
  encoded += len;
  return leaf;
}

bool Encoding::IsSimpleName(const unsigned char* p)
{
  return *p >= DigitOffset;
}

PTree::Node *Encoding::NameToPtree(const char* name, int len)
{
  if(!name) return 0;
  if(name[0] == 'n')
  {
    if(len == 5 && strncmp(name, "new[]", 5) == 0)
      return PTree::list(operator_name, anew_operator);
    else if(len == 3 && strncmp(name, "new", 3) == 0)
      return PTree::list(operator_name, new_operator);
  }
  else if(name[0] == 'd')
  {
    if(len == 8 && strncmp(name, "delete[]", 8) == 0)
      return PTree::list(operator_name, adelete_operator);
    else if(len == 6 && strncmp(name, "delete", 6) == 0)
      return PTree::list(operator_name, delete_operator);
  }
  else if(name[0] == '~')
    return PTree::list(tilder, new PTree::Atom(&name[1], len - 1));
  else if(name[0] == '@')
  {		// cast operator
    const unsigned char* encoded = (const unsigned char*)&name[1];
    return PTree::list(operator_name, MakePtree(encoded, 0));
  }
  if(is_letter(name[0])) return new PTree::Atom(name, len);
  else return PTree::list(operator_name, new PTree::Atom(name, len));
}

