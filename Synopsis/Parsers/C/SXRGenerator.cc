//
// Copyright (C) 2011 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "SXRGenerator.hh"
#include <Support/utils.hh>
#include <cstring>
#include <boost/filesystem/operations.hpp>
#include <sstream>
#include <iostream>

namespace fs = boost::filesystem;

SXRGenerator::SXRGenerator(ASGTranslator const &t, bool v, bool d)
  : translator_(t),
    verbose_(v),
    debug_(d)
{
}

void SXRGenerator::generate(CXTranslationUnit tu,
			    std::string const &sxr, std::string const &abs_filename, std::string const &filename)
{
  tu_ = tu;
  obuf_.open(sxr.c_str(), std::ios_base::out);
  // FIXME: This function is broken. Construct the range manually instead...
  // CXSourceRange range = clang_getCursorExtent(clang_getTranslationUnitCursor(tu));
  fs::path path(abs_filename);
  size_t size = fs::file_size(path);
  CXFile f = clang_getFile(tu, abs_filename.c_str());
  CXSourceLocation begin = clang_getLocationForOffset(tu, f, 0);
  CXSourceLocation end = clang_getLocationForOffset(tu, f, size);
  CXSourceRange range = clang_getRange(begin, end);

  CXToken *tokens;
  unsigned num_tokens;
  clang_tokenize(tu, range, &tokens, &num_tokens);
  write("<sxr filename=\"");
  write(filename.c_str());
  write("\">\n");
  write("<line>");

  // Why does clang start counting at '1' ?
  unsigned line = 1, column = 1;
  for (unsigned i = 0; i != num_tokens; ++i) 
  {
    // CXSourceRange extent = clang_getTokenExtent(tu, tokens[i]);
    // CXSourceLocation start = clang_getRangeStart(extent);
    CXSourceLocation start = clang_getTokenLocation(tu, tokens[i]);
    unsigned l, c;
    clang_getSpellingLocation(start, 0, &l, &c, 0);
    if (l > line) column = 1;
    while (l > line) { write("</line>\n<line>"); ++line;}
    while (c > column) { obuf_.sputc(' '); ++column;}
    //  Now fill the output buffer till the given location (l,c)
    CXString token_string = clang_getTokenSpelling(tu, tokens[i]);
    char const *s = clang_getCString(token_string);
    size_t len = strlen(s);
    CXTokenKind kind = clang_getTokenKind(tokens[i]);
    switch (kind)
    {
      case CXToken_Comment:
	write_comment(s, line);
	break;
      case CXToken_Keyword:
      case CXToken_Literal:
	write("<span class=\"");
	write(token_kind_to_class(kind));
	write("\">");
	write_esc(s);
	write("</span>");
	break;
      case CXToken_Identifier:
	write_xref(tokens[i]);
	break;
      default:
	write_esc(s);
	break;
    }
    clang_disposeString(token_string);
    column += len;
  }
  write("</line>\n</sxr>\n");
  clang_disposeTokens(tu, tokens, num_tokens);
  obuf_.close();
}

char const *SXRGenerator::token_kind_to_class(CXTokenKind k)
{
  switch (k)
  {
    case CXToken_Comment: return "comment";
    case CXToken_Keyword: return "keyword";
    case CXToken_Literal: return "literal";
    default: "unknown";
  }
}

std::string SXRGenerator::xref(CXCursor c)
{
  bpl::object decl = translator_.lookup(c);
  if (decl)
    return bpl::extract<std::string>(bpl::str(decl.attr("name")));
  else
  // throw std::runtime_error("Error: can't find definition for symbol " + cursor_info(c));
    return "";
}

std::string SXRGenerator::from(CXCursor c)
{
  if (debug_)
    std::cout << "from " << cursor_info(c) << std::endl;
  if (clang_isDeclaration(c.kind))
  {
    bpl::object decl = translator_.lookup(c);
    if (decl)
      return bpl::extract<std::string>(bpl::str(decl.attr("name")));
  }
  else
  {
    CXCursor p = clang_getCursorSemanticParent(c);
    if (clang_equalCursors(p, clang_getTranslationUnitCursor(tu_)))
      return "global scope";
    if (!clang_equalCursors(p, clang_getNullCursor()))
      return from(p);
  }
  return "";
}

void SXRGenerator::write_comment(std::string const &comment, unsigned &line)
{
  write("<span class=\"comment\">");
  // If the comment contains newlines, we need to write it in chunks
  std::string::size_type begin = 0, end = comment.find('\n');
  while (end != std::string::npos)
  {
    write_esc(comment.substr(begin, end - begin));
    write("</span></line>\n");
    write("<line><span class=\"comment\" continuation=\"true\">");
    ++line;
    begin = end + 1;
    end = comment.find('\n', begin);
  }
  write_esc(comment.substr(begin));
  write("</span>");
}

void SXRGenerator::write_xref(CXToken const &t)
{
  CXSourceLocation l = clang_getTokenLocation(tu_, t);
  CXString s = clang_getTokenSpelling(tu_, t);
  std::string text = clang_getCString(s);
  clang_disposeString(s);
  CXCursor c = clang_getCursor(tu_, l);
  CXCursor r = clang_getCursorReferenced(c);
  if (clang_isCursorDefinition(c) ||
      clang_isDeclaration(c.kind) ||
      c.kind == CXCursor_MacroDefinition)
  {
    std::string xref = this->xref(c);
    if (!xref.empty())
    {
      write("<a href=\"");
      write_esc(xref);
      std::string from = this->from(clang_getCursorSemanticParent(c));
      if (!from.empty())
      {
	write("\" from=\"");
	write_esc(from);
      }
      write("\" type=\"definition\">");
      write_esc(text);
      write("</a>");
    }
    else
      write_esc(text);
  }
  else if (!clang_equalCursors(r, c))
  {
    std::string xref = this->xref(r);
    if (!xref.empty())
    {
      write("<a href=\"");
      write_esc(xref);
      std::string from = this->from(c);
      if (!from.empty())
      {
	write("\" from=\"");
	write_esc(from);
      }
      write("\" type=\"reference\">");
      write_esc(text);
      write("</a>");
    }
    else
      write_esc(text);
  }
  else 
    throw std::runtime_error("unimplemented: " + cursor_info(c));
}

void SXRGenerator::write(char const *text, size_t len)
{
  if (len) obuf_.sputn(text, len);
  else obuf_.sputn(text, strlen(text));
}

void SXRGenerator::write_esc(std::string const &text)
{
  // output each character, escaping it as necessary
  for (std::string::const_iterator i = text.begin(); i != text.end(); ++i)
    switch (*i)
    {
      case '&': write("&amp;"); break;
      case '<': write("&lt;"); break;
      case '>': write("&gt;"); break;
      case '"': write("&quot;"); break;
      default: obuf_.sputc(*i); break;
    }
};

void SXRGenerator::write_esc(char const *text)
{
  // output each character, escaping it as necessary
  while (*text)
  {
    switch (*text)
    {
      case '&': write("&amp;"); break;
      case '<': write("&lt;"); break;
      case '>': write("&gt;"); break;
      case '"': write("&quot;"); break;
      default: obuf_.sputc(*text); break;
    }
    ++text;
  }
};
