//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef ASGTranslator_hh_
#define ASGTranslator_hh_

#include <boost/python.hpp>
#include <clang-c/Index.h>
#include <stack>
#include <map>

namespace bpl = boost::python;

class SymbolTable
{
  struct less
  {
    bool operator()(CXCursor c1, CXCursor c2) const
    { return c1.data[0] < c2.data[0] || 
	(c1.data[0] == c2.data[0] && c1.data[1] < c2.data[1]);
    }
  };
  typedef std::map<CXCursor, bpl::object, less> symbol_map;

public:
  void declare(CXCursor c, bpl::object d) { symbols_[c] = d;}
  bpl::object lookup(CXCursor c) const { return symbols_[c];}

private:
  mutable symbol_map symbols_;
};

class TypeRepository
{
  struct less
  {
    bool operator()(CXType t1, CXType t2) const
    { return t1.data[0] < t2.data[0] || 
	(t1.data[0] == t2.data[0] && t1.data[1] < t2.data[1]);
    }
  };
  typedef std::map<CXType, bpl::object, less> type_map;
public:
  TypeRepository(bpl::dict types, bool verbose);
  void declare(CXType t, bpl::object declaration, bool visible);
  bpl::object lookup(CXType t) const;

private:
  bpl::object qname(std::string const &name) const { return qname_(bpl::make_tuple(name));}

  bpl::object asg_module_;
  bpl::object qname_;
  mutable bpl::dict types_;
  mutable type_map cx_types_;
  bool verbose_;
};

class ASGTranslator
{
public:
  ASGTranslator(std::string const &filename,
		std::string const &base_path, bool primary_file_only,
		bpl::object asg, bpl::dict files, bool v, bool d);

  void translate(CXTranslationUnit);

  //. look up an ASG.Declaration from a cursor.
  bpl::object lookup(CXCursor d) const { return symbols_.lookup(d);}

private:
  bool is_visible(CXCursor);

  bpl::object qname(std::string const &name);
  void declare(CXCursor, bpl::object);

  CXChildVisitResult visit_pp_directive(CXCursor c, CXCursor p);
  CXChildVisitResult visit_declaration(CXCursor c, CXCursor p);

  bpl::object get_source_file(std::string const &);
  bpl::object create(CXCursor c);

  bpl::list get_comments(CXCursor);

  static CXChildVisitResult visit(CXCursor c, CXCursor p, CXClientData d);

  CXTranslationUnit tu_;
  bpl::object       qname_;
  bpl::object       asg_module_;
  bpl::object       sf_module_;
  bpl::dict         files_;
  TypeRepository    types_;
  SymbolTable       symbols_;
  bpl::list         declarations_;
  bpl::list         enumerators_;
  std::stack<bpl::object> scope_;
  // When translating declarations, we want to store comments that may
  // represent inline documentation. To do that, we keep track of the
  // location of the last visited cursor, which allows us to tokenize
  // everything between that and the current cursor's start, fishing for
  // comments.
  CXSourceLocation  comment_horizon_;
  bpl::object       file_;
  std::string       primary_filename_;
  bool              primary_file_only_;
  std::string       base_path_;
  bool              verbose_;
  bool              debug_;
};

#endif
