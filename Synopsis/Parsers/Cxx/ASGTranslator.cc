//
// Copyright (C) 2011 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "ASGTranslator.hh"
#include <Support/utils.hh>
#include <Support/path.hh>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace Synopsis;

namespace
{
inline bpl::dict annotations(bpl::object o) 
{ return bpl::extract<bpl::dict>(o.attr("annotations"));}
inline bpl::list comments(bpl::object o) 
{ return bpl::extract<bpl::list>(annotations(o).get("comments"));}

void cv_qual(CXType t, bpl::list mods)
{
  if (clang_isConstQualifiedType(t))
    mods.append("const");
  if (clang_isVolatileQualifiedType(t))
    mods.append("volatile");
}

void print_cursor_extent(CXCursor c)
{
  CXSourceRange range = clang_getCursorExtent(c);
  CXSourceLocation start = clang_getRangeStart(range);
  unsigned line, column;
  clang_getSpellingLocation(start, 0/*file*/, &line, &column, /*offset*/ 0);
  // std::cout << "start: " << line << ':' << column;
  CXSourceLocation end = clang_getRangeEnd(range);
  clang_getSpellingLocation(end, 0/*file*/, &line, &column, /*offset*/ 0);
  // std::cout << " end: " << line << ':' << column;
  // std::cout << std::endl;
}

class Stringifier
{
public:
  Stringifier(CXTranslationUnit tu) : tu_(tu) {}
  std::string stringify(CXCursor parent)
  {
    value_ = "";
    clang_visitChildren(parent, &Stringifier::visit, this);
    // std::cout << "stringify : " << value_ << std::endl;
    return value_;
  }
private:

  static CXChildVisitResult visit(CXCursor c, CXCursor p, CXClientData d)
  {
    Stringifier *stringifier = static_cast<Stringifier *>(d);

    CXString k = clang_getCursorKindSpelling(c.kind);
    std::string kind = clang_getCString(k);
    clang_disposeString(k);
    // std::cout << "stringify::visit " << kind << std::endl;
    // print_cursor_extent(c);
    stringifier->value_ += stringify_range(stringifier->tu_, clang_getCursorExtent(c));
    return CXChildVisit_Continue;
  }

  CXTranslationUnit tu_;
  std::string value_;
};


class ParamVisitor
{
public:
  ParamVisitor(SymbolTable &symbols, TypeRepository &types)
    : asg_module_(bpl::import("Synopsis.ASG")),
      symbols_(symbols),
      types_(types) 
  {
    bpl::object qname_module = bpl::import("Synopsis.QualifiedName");
    qname_ = qname_module.attr("QualifiedCxxName");  
  }

  bpl::list translate(CXCursor parent)
  {
    clang_visitChildren(parent, &ParamVisitor::visit, this);
    return params_;
  }
private:

  bpl::object qname(std::string const &name) { return qname_(bpl::make_tuple(name));}
  bpl::object create(CXCursor c)
  {
    CXString n = clang_getCursorSpelling(c);
    std::string name = clang_getCString(n);
    clang_disposeString(n);
    switch (c.kind)
    {
      case CXCursor_ParmDecl:
      {
	bpl::object type = types_.lookup(clang_getCursorType(c));
	return asg_module_.attr("Parameter")(bpl::list(), // premod
					     type,
					     bpl::list(), // postmod
					     name,
					     bpl::object()); // value
      }
      case CXCursor_TemplateTypeParameter:
      {
	bpl::object type = asg_module_.attr("DependentTypeId")("C++", qname("typename"));
	return asg_module_.attr("Parameter")(bpl::list(), // premod
					     type,
					     bpl::list(), // postmod
					     name,
					     bpl::object()); // value
      }
      case CXCursor_NonTypeTemplateParameter:
      {
	return asg_module_.attr("Parameter")(bpl::list(), // premod
					     bpl::object(), // type
					     bpl::list(), // postmod
					     name,
					     bpl::object()); // value
      }
      default:
	throw std::runtime_error("unimplemented: " + cursor_info(c));
    }
  }
  static CXChildVisitResult visit(CXCursor c, CXCursor p, CXClientData d)
  {
    ParamVisitor *visitor = static_cast<ParamVisitor*>(d);
    switch (c.kind)
    {
      case CXCursor_ParmDecl:
      case CXCursor_TemplateTypeParameter:
      case CXCursor_NonTypeTemplateParameter:
      {
	bpl::object o = visitor->create(c);
	// parameters aren't declared. We need them in the symbol table anyhow.
	visitor->symbols_.declare(c, o);
        visitor->params_.append(o);
        return CXChildVisit_Continue;
      }
    }
    return CXChildVisit_Break;
  }

  bpl::object asg_module_;
  bpl::object qname_;
  SymbolTable &symbols_;
  TypeRepository &types_;
  bpl::list params_;

};

}

TypeRepository::TypeRepository(bpl::dict types, bool verbose)
  : asg_module_(bpl::import("Synopsis.ASG")),
    types_(types),
    verbose_(verbose)
{
  bpl::object qname_module = bpl::import("Synopsis.QualifiedName");
  qname_ = qname_module.attr("QualifiedCxxName");  

#define DECLARE_BUILTIN_TYPE(T) types_[qname(T)] = asg_module_.attr("BuiltinTypeId")("C++", qname(T))

  DECLARE_BUILTIN_TYPE("bool");
  DECLARE_BUILTIN_TYPE("char");
  DECLARE_BUILTIN_TYPE("unsigned char");
  DECLARE_BUILTIN_TYPE("signed char");
  DECLARE_BUILTIN_TYPE("short");
  DECLARE_BUILTIN_TYPE("unsigned short");
  DECLARE_BUILTIN_TYPE("int");
  DECLARE_BUILTIN_TYPE("unsigned int");
  DECLARE_BUILTIN_TYPE("long");
  DECLARE_BUILTIN_TYPE("unsigned long");
  DECLARE_BUILTIN_TYPE("long long");
  DECLARE_BUILTIN_TYPE("unsigned long long");
  DECLARE_BUILTIN_TYPE("int");
  DECLARE_BUILTIN_TYPE("unsigned int");
  DECLARE_BUILTIN_TYPE("float");
  DECLARE_BUILTIN_TYPE("double");
  DECLARE_BUILTIN_TYPE("void");
  DECLARE_BUILTIN_TYPE("...");
  DECLARE_BUILTIN_TYPE("long double");
  DECLARE_BUILTIN_TYPE("wchar_t");
  // some GCC extensions...
  DECLARE_BUILTIN_TYPE("__builtin_va_list");

#undef DECLARE_BUILTIN_TYPE
}

void TypeRepository::declare(CXType t, bpl::object declaration, bool visible)
{
  if (verbose_)
    std::cout << "declare " << type_info(t) << std::endl;
  bpl::object name = declaration.attr("name");
  bpl::object type;
  if (visible) type = asg_module_.attr("DeclaredTypeId")("C++", name, declaration);
  else asg_module_.attr("UnknownTypeId")("C++", name, declaration);
  types_[name] = type;
  cx_types_[t] = type;
}

bpl::object TypeRepository::lookup(CXType t) const
{
  if (verbose_)
    std::cout << "lookup " << type_info(t) << std::endl;
  switch (t.kind)
  {
    case CXType_Void: return types_[qname("void")];
    case CXType_Bool: return types_[qname("bool")];
    case CXType_UChar: return types_[qname("unsigned char")];
    case CXType_UShort: return types_[qname("unsigned short")];
    case CXType_UInt: return types_[qname("unsigned int")];
    case CXType_ULong: return types_[qname("unsigned long")];
    case CXType_ULongLong: return types_[qname("unsigned long long")];
    case CXType_SChar: return types_[qname("signed char")];
    case CXType_Char_S: return types_[qname("char")];
    case CXType_WChar: return types_[qname("wchar_t")];
    case CXType_Short: return types_[qname("short")];
    case CXType_Int: return types_[qname("int")];
    case CXType_Long: return types_[qname("long")];
    case CXType_LongLong: return types_[qname("long long")];
    case CXType_Float: return types_[qname("float")];
    case CXType_Double: return types_[qname("double")];
    case CXType_LongDouble: return types_[qname("long double")];
    case CXType_Typedef: return lookup(clang_getCanonicalType(t));
    case CXType_LValueReference:
    {
      CXType i = clang_getPointeeType(t);
      bpl::list postmod;
      cv_qual(i, postmod);
      postmod.append("&");
      bpl::object inner = lookup(i);
      return asg_module_.attr("ModifierTypeId")("C++", inner, bpl::list(), postmod);
    }
    case CXType_Pointer:
    {
      CXType i = clang_getPointeeType(t);
      bpl::list postmod;
      cv_qual(i, postmod);
      postmod.append("*");
      bpl::object inner = lookup(i);
      return asg_module_.attr("ModifierTypeId")("C++", inner, bpl::list(), postmod);
    }
    default:
    {
      bpl::object type = cx_types_[t];
      if (!type)
	throw std::runtime_error("undefined type " + type_info(t));
      return type;
    }
  };
}

ASGTranslator::ASGTranslator(std::string const &filename,
			     std::string const &base_path, bool primary_file_only,
			     bpl::object ir,
			     bool v, bool d)
  : asg_module_(bpl::import("Synopsis.ASG")),
    sf_module_(bpl::import("Synopsis.SourceFile")),
    files_(bpl::extract<bpl::dict>(ir.attr("files"))()),
    types_(bpl::extract<bpl::dict>(ir.attr("asg").attr("types"))(), v),
    raw_filename_(filename),
    base_path_(base_path),
    primary_file_only_(primary_file_only),
    verbose_(v),
    debug_(d)
{
  bpl::object qname_module = bpl::import("Synopsis.QualifiedName");
  qname_ = qname_module.attr("QualifiedCxxName");
  bpl::object asg = ir.attr("asg");
  declarations_ = bpl::extract<bpl::list>(asg.attr("declarations"))();
  // determine canonical filenames
  std::string long_filename = Synopsis::make_full_path(raw_filename_);
  std::string short_filename = Synopsis::make_short_path(raw_filename_, base_path_);

  bpl::object file = files_.get(short_filename);
  if (file)
    file_ = file;
  else
  {
    file_ = sf_module_.attr("SourceFile")(short_filename, long_filename, "C++");
    files_[short_filename] = file_;
  }
  annotations(file_)["primary"] = true;
}

void ASGTranslator::translate(CXTranslationUnit tu)
{
  tu_ = tu; // save to make tu accessible elsewhere (tokenization)
  clang_visitChildren(clang_getTranslationUnitCursor(tu), &ASGTranslator::visit, this);
}

bpl::object ASGTranslator::get_source_file(std::string const &filename)
{
  std::string long_filename = make_full_path(filename);
  std::string short_filename = make_short_path(filename, base_path_);

  bpl::object file = files_.get(short_filename);
  if (!file)
  {
    file = sf_module_.attr("SourceFile")(short_filename, long_filename, "C++");
    files_[short_filename] = file;
  }
  return file;
}

bpl::object ASGTranslator::qname(std::string const &name)
{
  if (scope_.size())
    return scope_.top().attr("name") + bpl::make_tuple(name);
  else
    return qname_(bpl::make_tuple(name));
}

void ASGTranslator::declare(CXCursor c, bpl::object d)
{
  if (scope_.size())
    bpl::extract<bpl::list>(scope_.top().attr("declarations"))().append(d);
  else
    declarations_.append(d);
  bpl::extract<bpl::list>(file_.attr("declarations"))().append(d);
  symbols_.declare(c, d);
}

bpl::object ASGTranslator::create(CXCursor c)
{
  if (verbose_)
    std::cout << "create " << cursor_info(c) << std::endl;
  CXString k = clang_getCursorKindSpelling(c.kind);
  CXString n = clang_getCursorSpelling(c);
  std::string kind = clang_getCString(k);
  std::string name = clang_getCString(n);
  clang_disposeString(n);
  clang_disposeString(k);
  CXFile sf;
  unsigned line;
  CXSourceLocation l = clang_getCursorLocation(c);
  clang_getSpellingLocation(l, &sf, &line, /*column*/ 0, /*offset*/ 0);
  CXString f = clang_getFileName(sf);
  std::string file = clang_getCString(f);
  clang_disposeString(f);
  bpl::object source_file = get_source_file(file);
  switch (c.kind)
  {
    case CXCursor_StructDecl:
      return asg_module_.attr("Class")(source_file, line, "struct", qname(name));
    case CXCursor_UnionDecl:
      return asg_module_.attr("Class")(source_file, line, "union", qname(name));
    case CXCursor_ClassDecl:
      return asg_module_.attr("Class")(source_file, line, "class", qname(name));
    case CXCursor_ClassTemplate:
    case CXCursor_ClassTemplatePartialSpecialization:
    {
      // For template specializations we alter the name to include the template signature.
      // Specializations can't be found via symbol lookup, but we do want to be able to refer
      // to them explicitly.
      if (c.kind == CXCursor_ClassTemplatePartialSpecialization)
      {
	CXString sn = clang_getCursorDisplayName(c);
	name = clang_getCString(sn);
	clang_disposeString(sn);
      }
      ParamVisitor param_visitor(symbols_, types_);
      bpl::list params = param_visitor.translate(c);
      clang_visitChildren(c, &ASGTranslator::visit, this);
      CXCursorKind k = clang_getTemplateCursorKind(c);
      char const *kind = "class";
      if (k == CXCursor_StructDecl) kind == "struct";
      else if (k == CXCursor_UnionDecl) kind == "union";
      bpl::object templ = asg_module_.attr("ClassTemplate")(source_file, line, "class", qname(name));
      bpl::object t_id = asg_module_.attr("TemplateId")("C++", qname(name), templ, params);
      templ.attr("template") = t_id;
      return templ;
    }
    case CXCursor_Namespace:
    {
      if (name.empty()) // anonymous
      {
	name = bpl::extract<std::string>(file_.attr("name"));
	std::string::size_type p = name.rfind('/');
	if (p != std::string::npos) name.erase(0, p + 1);
	name = "{" + name + "}";
      }
      return asg_module_.attr("Module")(source_file, line, "namespace", qname(name));
    }
    case CXCursor_FieldDecl:
    {
      bpl::object type = types_.lookup(clang_getCursorType(c));
      return asg_module_.attr("Variable")(source_file, line, "data member", qname(name), type, false);
    }
    case CXCursor_EnumConstantDecl:
    {
      Stringifier stringifier(tu_);
      bpl::object value(stringifier.stringify(c));
      return asg_module_.attr("Enumerator")(source_file, line, qname(name), value);
    }
    case CXCursor_VarDecl:
    {
      bpl::object type = types_.lookup(clang_getCursorType(c));
      std::string vtype = scope_.size() ?
	std::string(bpl::extract<std::string>(scope_.top().attr("type"))) :
	"global variable";
      if (vtype == "function") vtype = "local variable";
      else if (vtype == "namespace") vtype = "variable";

      return asg_module_.attr("Variable")(source_file, line, vtype, qname(name), type, false);
    }
    case CXCursor_TypedefDecl:
    {
      bpl::object type = types_.lookup(clang_getCursorType(c));
      return asg_module_.attr("Typedef")(source_file, line, "typedef", qname(name), type, false);
    }
    case CXCursor_EnumDecl:
    {
      enumerators_ = bpl::list();
      clang_visitChildren(c, &ASGTranslator::visit, this);
      return asg_module_.attr("Enum")(source_file, line, qname(name), enumerators_);
    }
    case CXCursor_FunctionDecl:
    {
      CXString dn = clang_getCursorDisplayName(c);
      std::string full_name = clang_getCString(dn);
      clang_disposeString(dn);
      bpl::object return_type = types_.lookup(clang_getCursorResultType(c));
      clang_visitChildren(c, &ASGTranslator::visit, this);
      bpl::object f = asg_module_.attr("Function")(source_file, line,
						   "function",
						   bpl::list(), // premod
						   return_type,
						   bpl::list(), // postmod
						   qname(full_name), // fname (mangled)
						   name); // fname
      ParamVisitor param_visitor(symbols_, types_);
      f.attr("parameters") = param_visitor.translate(c);
      return f;
    }
    default:
      throw std::runtime_error("unimplemented: " + cursor_info(c));
  }
}

CXChildVisitResult ASGTranslator::visit_declaration(CXCursor c, CXCursor p)
{
  // Skip any builtin declarations
  CXSourceLocation l = clang_getCursorLocation(c);
  CXFile sf;
  clang_getSpellingLocation(l, &sf, 0 /*line*/, /*column*/ 0, /*offset*/ 0);
  CXString f = clang_getFileName(sf);
  bool visible = clang_getCString(f);
  clang_disposeString(f);
  if (!visible) return CXChildVisit_Continue;

  CXType type = clang_getCursorType(c);
  switch (c.kind)
  {
    case CXCursor_StructDecl:
    case CXCursor_UnionDecl:
    case CXCursor_ClassDecl:
    case CXCursor_ClassTemplate:
    case CXCursor_ClassTemplatePartialSpecialization:
    case CXCursor_Namespace:
    {
      bpl::object o = create(c);
      declare(c, o);
      if (c.kind == CXCursor_ClassTemplatePartialSpecialization)
      {
	CXCursor p = clang_getSpecializedCursorTemplate(c);
	bpl::object primary = lookup(p);
	bpl::extract<bpl::list>(primary.attr("specializations"))().append(o);
	o.attr("primary") = primary;
      }
      if (type.kind != CXType_Invalid) // e.g. for namespaces
	types_.declare(type, o, true);
      scope_.push(o);
      clang_visitChildren(c, &ASGTranslator::visit, this);
      scope_.pop();
      return CXChildVisit_Continue;
    }
    case CXCursor_EnumConstantDecl:
    {
      bpl::object o = create(c);
      // enumerators aren't declared. We need them in the symbol table anyhow.
      symbols_.declare(c, o);
      enumerators_.append(o);
      return CXChildVisit_Continue;
    }
    case CXCursor_FieldDecl:
    case CXCursor_VarDecl:
    case CXCursor_TypedefDecl:
      if (type.kind != CXType_Invalid) // e.g. builtin typendefs
      {
	bpl::object o = create(c);
	declare(c, o);
	types_.declare(type, o, true);
      }
      return CXChildVisit_Continue;
    case CXCursor_EnumDecl:
    {
      bpl::object o = create(c);
      declare(c, o);
      types_.declare(type, o, true);
      return CXChildVisit_Continue;
    }
    case CXCursor_FunctionDecl:
    {
      bpl::object o = create(c);
      declare(c, o);
      return CXChildVisit_Continue;
    }
    // These are dealt with elsewhere. Ignore them in this context.
    case CXCursor_ParmDecl:
    case CXCursor_TemplateTypeParameter:
    case CXCursor_NonTypeTemplateParameter:
      return CXChildVisit_Continue;
    default:
      throw std::runtime_error("unimplemented: " + cursor_info(c));
  }
}

CXChildVisitResult ASGTranslator::visit(CXCursor c, CXCursor p, CXClientData d)
{
  ASGTranslator *translator = static_cast<ASGTranslator*>(d);
  if (translator->verbose_)
    std::cout << "visit " << cursor_info(c) << std::endl;

  CXCursor tu = clang_getTranslationUnitCursor(translator->tu_);
  CXCursor lp = clang_getCursorLexicalParent(c);
  // std::cout << "parent cursor equals TU :" << clang_equalCursors(tu, p) << std::endl;
  // std::cout << "parent cursor equals lexical parent :" << clang_equalCursors(lp, p) << std::endl;
  // std::cout << "lexical parent equals TU :" << clang_equalCursors(lp, tu) << std::endl;

  if (clang_isDeclaration(c.kind)) return translator->visit_declaration(c, p);
  return CXChildVisit_Continue;
}
