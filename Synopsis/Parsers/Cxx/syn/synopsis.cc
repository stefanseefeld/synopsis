//
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "synopsis.hh"
#include <map>
#include <set>

#include <iostream>
#include <signal.h>

#include "filter.hh"

#ifdef DO_TRACE
int Trace::level = 0;
#endif

#define assertObject(pyo) if (!pyo) PyErr_Print(); assert(pyo)

// This func is called so you can breakpoint on it
void nullObj()
{
  std::cout << "Null ptr." << std::endl;
  if (PyErr_Occurred()) PyErr_Print();
  raise(SIGINT);
}

// The compiler firewalled private stuff
struct Synopsis::Private
{
  //. Constructor
  Private(Synopsis* s) : m_syn(s)
  {
    m_cxx = PyString_InternFromString("C++");
    Py_INCREF(Py_None);
    add((AST::Declaration*)NULL, Py_None);
    Py_INCREF(Py_None);
    add((Types::Type*)NULL, Py_None);
  }
  //. Reference to parent synopsis object
  Synopsis* m_syn;
  //. Interned string for "C++"
  PyObject* m_cxx;
  //. Returns the string for "C++" as a borrowed reference
  PyObject* cxx()
  {
    return m_cxx;
  }
  // Sugar
  typedef std::map<void*, PyObject*> ObjMap;
  // Maps from C++ objects to PyObjects
  ObjMap obj_map;
  // A set of builtin declarations to not to store in global namespace
  std::set<AST::Declaration*> builtin_decl_set;
  
  // Note that these methods always succeed
  
  //. Return the PyObject for the given AST::SourceFile
  PyObject* py(AST::SourceFile*);
  //. Return the PyObject for the given AST::Include
  PyObject* py(AST::Include*);
  //. Return the PyObject for the given AST::Declaration
  PyObject* py(AST::Declaration*);
  //. Return the PyObject for the given AST::Inheritance
  PyObject* py(AST::Inheritance*);
  //. Return the PyObject for the given AST::Parameter
  PyObject* py(AST::Parameter*);
  //. Return the PyObject for the given AST::Comment
  PyObject* py(AST::Comment*);
  //. Return the PyObject for the given Types::Type
  PyObject* py(Types::Type*);
  //. Return the PyObject for the given string
  PyObject* py(const std::string &);
  
  //. Add the given pair
  void add(void* cobj, PyObject* pyobj)
  {
    if (!pyobj) { nullObj();}
    obj_map.insert(ObjMap::value_type(cobj, pyobj));
  }

  // Convert a vector to a List
  template <class T>
  PyObject* List(const std::vector<T*>& vec)
  {
    PyObject* list = PyList_New(vec.size());
    int index = 0;
    typename std::vector<T*>::const_iterator iter = vec.begin();
    while (iter != vec.end())
      PyList_SET_ITEM(list, index++, py(*iter++));
    return list;
  }

  // Convert a vector to a Tuple
  template <class T>
  PyObject* Tuple(const std::vector<T*>& vec)
  {
    PyObject* tuple = PyTuple_New(vec.size());
    int index = 0;
    typename std::vector<T*>::const_iterator iter = vec.begin();
    while (iter != vec.end())
      PyTuple_SET_ITEM(tuple, index++, py(*iter++));
    return tuple;
  }

  // Convert a string vector to a List
  PyObject* List(const std::vector<std::string> &vec)
  {
    PyObject* list = PyList_New(vec.size());
    int index = 0;
    std::vector<std::string>::const_iterator iter = vec.begin();
    while (iter != vec.end())
      PyList_SET_ITEM(list, index++, py(*iter++));
    return list;
  }

  // Convert a string vector to a Tuple
  PyObject* Tuple(const std::vector<std::string> &vec)
  {
    PyObject* tuple = PyTuple_New(vec.size());
    int index = 0;
    std::vector<std::string>::const_iterator iter = vec.begin();
    while (iter != vec.end())
      PyTuple_SET_ITEM(tuple, index++, py(*iter++));
    return tuple;
  }
};

// Convert a Declaration vector to a List. Declarations can return NULL
// from py(), if they are not in the main file.
template <>
PyObject* Synopsis::Private::List(const std::vector<AST::Declaration*>& vec)
{
  std::vector<PyObject*> objects;
  std::vector<AST::Declaration*>::const_iterator iter = vec.begin();
  while (iter != vec.end())
  {
    PyObject* obj = py(*iter++);
    if (obj != 0) objects.push_back(obj);
  }

  PyObject* list = PyList_New(objects.size());
  int index = 0;
  std::vector<PyObject*>::const_iterator piter = objects.begin();
  while (piter != objects.end())
    PyList_SET_ITEM(list, index++, *piter++);
  return list;
}

PyObject* Synopsis::Private::py(AST::SourceFile* file)
{
  ObjMap::iterator iter = obj_map.find(file);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    add(file, m_syn->SourceFile(file));
    iter = obj_map.find(file);
    if (iter == obj_map.end())
    {
      std::cout << "Fatal: Still not PyObject after converting." << std::endl;
      throw "Synopsis::Private::py(AST::SourceFile*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}

PyObject* Synopsis::Private::py(AST::Include* incl)
{
  ObjMap::iterator iter = obj_map.find(incl);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    add(incl, m_syn->Include(incl));
    iter = obj_map.find(incl);
    if (iter == obj_map.end())
    {
      std::cout << "Fatal: Still not PyObject after converting." << std::endl;
      throw "Synopsis::Private::py(AST::Include*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}

PyObject* Synopsis::Private::py(AST::Declaration* decl)
{
  ObjMap::iterator iter = obj_map.find(decl);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    decl->accept(m_syn);
    iter = obj_map.find(decl);
    if (iter == obj_map.end())
    {
      // Probably means it's not in the main file
      return 0;
      /*
        std::cout << "Fatal: Still not PyObject after converting." << std::endl;
        throw "Synopsis::Private::py(AST::Declaration*)";
      */
    }
    // Force addition of type to dictionary
    PyObject* declared = py(decl->declared());
    Py_DECREF(declared);
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}

PyObject* Synopsis::Private::py(AST::Inheritance* decl)
{
  ObjMap::iterator iter = obj_map.find(decl);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    decl->accept(m_syn);
    iter = obj_map.find(decl);
    if (iter == obj_map.end())
    {
      std::cout << "Fatal: Still not PyObject after converting." << std::endl;
      throw "Synopsis::Private::py(AST::Inheritance*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}
PyObject* Synopsis::Private::py(AST::Parameter* decl)
{
  ObjMap::iterator iter = obj_map.find(decl);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    decl->accept(m_syn);
    iter = obj_map.find(decl);
    if (iter == obj_map.end())
    {
      std::cout << "Fatal: Still not PyObject after converting." << std::endl;
      throw "Synopsis::Private::py(AST::Parameter*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}

PyObject* Synopsis::Private::py(AST::Comment* decl)
{
  ObjMap::iterator iter = obj_map.find(decl);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    m_syn->visit_comment(decl);
    iter = obj_map.find(decl);
    if (iter == obj_map.end())
    {
      std::cout << "Fatal: Still not PyObject after converting." << std::endl;
      throw "Synopsis::Private::py(AST::Comment*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}


PyObject* Synopsis::Private::py(Types::Type* type)
{
  ObjMap::iterator iter = obj_map.find(type);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    type->accept(m_syn);
    iter = obj_map.find(type);
    if (iter == obj_map.end())
    {
      std::cout << "Fatal: Still not PyObject after converting." << std::endl;
      throw "Synopsis::Private::py(Types::Type*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}

PyObject* Synopsis::Private::py(const std::string &str)
{
  PyObject* pystr = PyString_FromStringAndSize(str.data(), str.size());
  //PyString_InternInPlace(&pystr);
  return pystr;
}

Synopsis::Synopsis(FileFilter* filter, PyObject *ast)
  : m_ast(ast), m_filter(filter)
{
  Trace trace("Synopsis::Synopsis");
  m_ast_module  = PyImport_ImportModule("Synopsis.AST");
  assertObject(m_ast_module);
  m_type_module = PyImport_ImportModule("Synopsis.Type");
  assertObject(m_type_module);
  
  m_declarations = PyObject_CallMethod(m_ast, "declarations", "");
  assertObject(m_declarations);
  m_dictionary = PyObject_CallMethod(m_ast, "types", "");
  assertObject(m_dictionary);
  
  m = new Private(this);
}

Synopsis::~Synopsis()
{
  Trace trace("Synopsis::~Synopsis");
  /*
    PyObject *file = PyObject_CallMethod(scopes.back(), "declarations", 0);
    size_t size = PyList_Size(file);
    for (size_t i = 0; i != size; i++)
    PyObject_CallMethod(declarations, "append", "O", PyList_GetItem(file, i));
  */
  
  Py_DECREF(m_declarations);
  Py_DECREF(m_dictionary);
  
  Py_DECREF(m_type_module);
  Py_DECREF(m_ast_module);
  
  // Deref the objects we created
  Private::ObjMap::iterator iter = m->obj_map.begin();
  Private::ObjMap::iterator end = m->obj_map.end();
  while (iter != end)
  {
    PyObject* obj = iter->second;
    PyObject* repr = PyObject_Repr(obj);
    //std::cout << (obj->ob_refcnt-1) << " " << PyString_AsString(repr) << std::endl;
    Py_DECREF(repr);
    Py_DECREF(obj);
    iter->second = NULL;
    ++iter;
  }
  //std::cout << "Total: " << m->obj_map.size()<<" objects." << std::endl;
  delete m;
}

void Synopsis::translate(AST::Scope* scope)//, PyObject* ast)
{
  AST::Declaration::vector globals, &decls = scope->declarations();
  AST::Declaration::vector::iterator it = decls.begin();
  
  // List all declarations not in the builtin_decl_set
  for (; it != decls.end(); ++it)
  if (m->builtin_decl_set.find(*it) == m->builtin_decl_set.end())
    globals.push_back(*it);
  
  // Translate those declarations
  PyObject* list;
  PyObject_CallMethod(m_declarations, "extend", "O",
                      list = m->List(globals));
  Py_DECREF(list);

  // Translate the sourcefiles, making sure the declarations list is done
  // for each
  PyObject* pyfiles = PyObject_CallMethod(m_ast, "files", NULL);
  assertObject(pyfiles);
  assert(PyDict_Check(pyfiles));
  
  AST::SourceFile::vector all_sourcefiles;
  m_filter->get_all_sourcefiles(all_sourcefiles);
  AST::SourceFile::vector::iterator file_iter = all_sourcefiles.begin();
  while (file_iter != all_sourcefiles.end())
  {
    AST::SourceFile* file = *file_iter++;
    
    PyObject* decls, *new_decls, *incls, *new_incls;
    PyObject* pyfile = m->py(file);
    
    // Add the declarations if it's a main file
    if (file->is_main())
    {
      decls = PyObject_CallMethod(pyfile, "declarations", NULL);
      assertObject(decls);
      PyObject_CallMethod(decls, "extend", "O", new_decls = m->List(file->declarations()));
      // TODO: add the includes
      Py_DECREF(new_decls);
      Py_DECREF(decls);
    }
    
    // Add the includes
    incls = PyObject_CallMethod(pyfile, "includes", NULL);
    assertObject(incls);
    PyObject_CallMethod(incls, "extend", "O", new_incls = m->List(file->includes()));
    Py_DECREF(new_incls);
    Py_DECREF(incls);
    
    // Add to the AST
    PyObject* pyfilename = PyObject_CallMethod(pyfile, "filename", NULL);
    PyDict_SetItem(pyfiles, pyfilename, pyfile);
    Py_DECREF(pyfilename);
    Py_DECREF(pyfile);
  }
  
  Py_DECREF(pyfiles);
}

void Synopsis::set_builtin_decls(const AST::Declaration::vector& builtin_decls)
{
  // Insert Base*'s into a set for faster lookup
  AST::Declaration::vector::const_iterator it = builtin_decls.begin();
  while (it != builtin_decls.end()) m->builtin_decl_set.insert(*it++);
}

PyObject *Synopsis::Base(Types::Base* type)
{
  Trace trace("Synopsis::Base");
  PyObject *name, *base;
  base = PyObject_CallMethod(m_type_module, "Base", "OO",
                             m->cxx(), name = m->Tuple(type->name()));
  PyObject_SetItem(m_dictionary, name, base);
  Py_DECREF(name);
  return base;
}

PyObject *Synopsis::Dependent(Types::Dependent* type)
{
  Trace trace("Synopsis::Dependent");
  PyObject *name, *base;
  base = PyObject_CallMethod(m_type_module, "Dependent", "OO",
                             m->cxx(), name = m->Tuple(type->name()));
  PyObject_SetItem(m_dictionary, name, base);
  Py_DECREF(name);
  return base;
}

PyObject *Synopsis::Unknown(Types::Named* type)
{
  Trace trace("Synopsis::Unknown");
  PyObject *name, *unknown;
  unknown = PyObject_CallMethod(m_type_module, "Unknown", "OO",
                                m->cxx(), name = m->Tuple(type->name()));
  PyObject_SetItem(m_dictionary, name, unknown);
  Py_DECREF(name);
  return unknown;
}

PyObject *Synopsis::Declared(Types::Declared* type)
{
  Trace trace("Synopsis::Declared");
  PyObject *name, *declared, *decl;
  declared = PyObject_CallMethod(m_type_module, "Declared", "OOO",
                                 m->cxx(), name = m->Tuple(type->name()), 
                                 decl = m->py(type->declaration()));
  // Skip zero-length names (eg: dummy declarators/enumerators)
  if (type->name().size())
    PyObject_SetItem(m_dictionary, name, declared);
  Py_DECREF(name);
  Py_DECREF(decl);
  return declared;
}

PyObject *Synopsis::Template(Types::Template* type)
{
  Trace trace("Synopsis::Template");
  PyObject *name, *templ, *decl, *params;
  templ = PyObject_CallMethod(m_type_module, "Template", "OOOO",
                              m->cxx(), name = m->Tuple(type->name()), 
                              decl = m->py(type->declaration()),
                              params = m->List(type->parameters()));
  PyObject_SetItem(m_dictionary, name, templ);
  Py_DECREF(name);
  Py_DECREF(decl);
  Py_DECREF(params);
  return templ;
}

PyObject *Synopsis::Modifier(Types::Modifier* type)
{
  Trace trace("Synopsis::Modifier");
  PyObject *modifier, *alias, *pre, *post;
  modifier = PyObject_CallMethod(m_type_module, "Modifier", "OOOO",
                                 m->cxx(), alias = m->py(type->alias()),
                                 pre = m->List(type->pre()), post = m->List(type->post()));
  Py_DECREF(alias);
  Py_DECREF(pre);
  Py_DECREF(post);
  return modifier;
}

PyObject *Synopsis::Array(Types::Array *type)
{
  Trace trace("Synopsis::Array");
  PyObject *array, *alias, *sizes;
  array = PyObject_CallMethod(m_type_module, "Array", "OOO",
                              m->cxx(), alias = m->py(type->alias()), 
                              sizes = m->List(type->sizes()));
  Py_DECREF(alias);
  Py_DECREF(sizes);
  return array;
}

PyObject *Synopsis::Parameterized(Types::Parameterized* type)
{
  Trace trace("Synopsis::Parametrized");
  PyObject *parametrized, *templ, *params;
  parametrized = PyObject_CallMethod(m_type_module, "Parametrized", "OOO",
                                     m->cxx(), templ = m->py(type->template_type()), 
                                     params = m->List(type->parameters()));
  Py_DECREF(templ);
  Py_DECREF(params);
  return parametrized;
}

PyObject *Synopsis::FuncPtr(Types::FuncPtr* type)
{
  Trace trace("Synopsis::FuncType");
  PyObject *func, *ret, *pre, *params;
  func = PyObject_CallMethod(m_type_module, "Function", "OOOO",
                             m->cxx(), ret = m->py(type->return_type()), 
                             pre = m->List(type->pre()),
                             params = m->List(type->parameters()));
  Py_DECREF(ret);
  Py_DECREF(pre);
  Py_DECREF(params);
  return func;
}

void Synopsis::addComments(PyObject* pydecl, AST::Declaration* cdecl)
{
  PyObject *comments, *new_comments;
  comments = PyObject_CallMethod(pydecl, "comments", NULL);
  PyObject_CallMethod(comments, "extend", "O", new_comments = m->List(cdecl->comments()));
  // Also set the accessability..
  PyObject_CallMethod(pydecl, "set_accessibility", "i", int(cdecl->access()));
  Py_DECREF(comments);
  Py_DECREF(new_comments);
}

//. merge the file with an existing (python) object if it exists
PyObject *Synopsis::SourceFile(AST::SourceFile* file)
{
  // don't construct, but instead find existing python object !
  Trace trace("Synopsis::SourceFile");
  PyObject *files = PyObject_CallMethod(m_ast, "files", "");
  assertObject(files);
  PyObject *pyfile = PyDict_GetItemString(files, file->filename().c_str());
  if (!pyfile) // the file wasn't found, create it now
  {
    PyObject *filename, *full_filename;
    pyfile = PyObject_CallMethod(m_ast_module, "SourceFile", "OOO",
				 filename = m->py(file->filename()),
				 full_filename = m->py(file->full_filename()),
				 m->cxx());
    assertObject(pyfile);
    PyObject_CallMethod(pyfile, "set_is_main", "i", (int)file->is_main());
    Py_DECREF(filename);
    Py_DECREF(full_filename);
  }
  else Py_INCREF(pyfile);

  Py_DECREF(files);
  return pyfile;
}

PyObject *Synopsis::Include(AST::Include* include)
{
  Trace trace("Synopsis::Include");
  PyObject *pyinclude, *target;
  pyinclude = PyObject_CallMethod(m_ast_module, "Include", "Oii",
                                  target = m->py(include->target()),
                                  include->is_macro() ? 1 : 0,
                                  include->is_next() ? 1 : 0);
  assertObject(pyinclude);
  Py_DECREF(target);
  return pyinclude;
}

PyObject *Synopsis::Declaration(AST::Declaration* decl)
{
  Trace trace("Synopsis::addDeclaration");
  PyObject *pydecl, *file, *type, *name;
  pydecl = PyObject_CallMethod(m_ast_module, "Declaration", "OiOOO",
                               file = m->py(decl->file()), decl->line(), m->cxx(),
                               type = m->py(decl->type()), name = m->Tuple(decl->name()));
  assertObject(pydecl);
  addComments(pydecl, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  return pydecl;
}

PyObject *Synopsis::Builtin(AST::Builtin* decl)
{
  Trace trace("Synopsis::Builtin");
  PyObject *pybuiltin, *file, *type, *name;
  pybuiltin = PyObject_CallMethod(m_ast_module, "Builtin", "OiOOO",
                                  file = m->py(decl->file()), decl->line(), m->cxx(),
                                  type = m->py(decl->type()), name = m->Tuple(decl->name()));
  assertObject(pybuiltin);
  addComments(pybuiltin, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  return pybuiltin;
}

PyObject *Synopsis::Macro(AST::Macro* decl)
{
  Trace trace("Synopsis::Macro");
  PyObject *pymacro, *file, *type, *name, *params, *text;
  if (decl->parameters()) params = m->List(*decl->parameters());
  else
  {
    params = Py_None;
    Py_INCREF(Py_None);
  }
  pymacro = PyObject_CallMethod(m_ast_module, "Macro", "OiOOOOO",
                                file = m->py(decl->file()), decl->line(), m->cxx(),
                                type = m->py(decl->type()), name = m->Tuple(decl->name()),
                                params, text = m->py(decl->text()));
  assertObject(pymacro);
  addComments(pymacro, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(params);
  Py_DECREF(text);
  return pymacro;
}

PyObject *Synopsis::Forward(AST::Forward* decl)
{
  Trace trace("Synopsis::addForward");
  PyObject *forward, *file, *type, *name;
  forward = PyObject_CallMethod(m_ast_module, "Forward", "OiOOO",
                                file = m->py(decl->file()), decl->line(), m->cxx(),
                                type = m->py(decl->type()), name = m->Tuple(decl->name()));
  addComments(forward, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  return forward;
}

PyObject *Synopsis::Comment(AST::Comment* decl)
{
  Trace trace("Synopsis::addComment");
  std::string text_str = decl->text()+"\n";
  PyObject *text = PyString_FromStringAndSize(text_str.data(), text_str.size());
  PyObject *comment, *file;
  comment = PyObject_CallMethod(m_ast_module, "Comment", "OOii",
                                text, file = m->py(decl->file()), 
                                decl->line(), decl->is_suspect() ? 1 : 0);
  Py_DECREF(text);
  Py_DECREF(file);
  return comment;
}

PyObject *Synopsis::Scope(AST::Scope* decl)
{
  Trace trace("Synopsis::addScope");
  PyObject *scope, *file, *type, *name;
  scope = PyObject_CallMethod(m_ast_module, "Scope", "OiOOO",
                              file = m->py(decl->file()), decl->line(), m->cxx(),
                              type = m->py(decl->type()), name = m->Tuple(decl->name()));
  PyObject *decls = PyObject_CallMethod(scope, "declarations", NULL);
  PyObject_CallMethod(decls, "extend", "O", m->List(decl->declarations()));
  addComments(scope, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(decls);
  return scope;
}

PyObject *Synopsis::Namespace(AST::Namespace* decl)
{
  Trace trace("Synopsis::addNamespace");
  PyObject *module, *file, *type, *name;
  module = PyObject_CallMethod(m_ast_module, "Module", "OiOOO",
                               file = m->py(decl->file()), decl->line(), m->cxx(),
                               type = m->py(decl->type()), name = m->Tuple(decl->name()));
  PyObject *decls = PyObject_CallMethod(module, "declarations", NULL);
  PyObject *new_decls = m->List(decl->declarations());
  PyObject_CallMethod(decls, "extend", "O", new_decls);
  addComments(module, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(decls);
  Py_DECREF(new_decls);
  return module;
}

PyObject *Synopsis::Inheritance(AST::Inheritance* decl)
{
  Trace trace("Synopsis::Inheritance");
  PyObject *inheritance, *parent, *attrs;
  inheritance = PyObject_CallMethod(m_ast_module, "Inheritance", "sOO",
                                    "inherits", parent = m->py(decl->parent()), 
                                    attrs = m->List(decl->attributes()));
  Py_DECREF(parent);
  Py_DECREF(attrs);
  return inheritance;
}

PyObject *Synopsis::Class(AST::Class* decl)
{
  Trace trace("Synopsis::addClass");
  PyObject *clas, *file, *type, *name;
  clas = PyObject_CallMethod(m_ast_module, "Class", "OiOOO",
                             file = m->py(decl->file()), decl->line(), m->cxx(),
                             type = m->py(decl->type()), name = m->Tuple(decl->name()));
  // This is necessary to prevent inf. loops in several places
  m->add(decl, clas);
  PyObject *new_decls, *new_parents;
  PyObject *decls = PyObject_CallMethod(clas, "declarations", NULL);
  PyObject_CallMethod(decls, "extend", "O", new_decls = m->List(decl->declarations()));
  PyObject *parents = PyObject_CallMethod(clas, "parents", NULL);
  PyObject_CallMethod(parents, "extend", "O", new_parents = m->List(decl->parents()));
  if (decl->template_type())
  {
    PyObject* ttype;
    PyObject_CallMethod(clas, "set_template", "O", ttype = m->py(decl->template_type()));
    Py_DECREF(ttype);
  }
  addComments(clas, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(decls);
  Py_DECREF(parents);
  Py_DECREF(new_decls);
  Py_DECREF(new_parents);
  return clas;
}

PyObject *Synopsis::Typedef(AST::Typedef* decl)
{
  Trace trace("Synopsis::addTypedef");
  // FIXME: what to do about the declarator?
  PyObject *tdef, *file, *type, *name, *alias;
  tdef = PyObject_CallMethod(m_ast_module, "Typedef", "OiOOOOi",
                             file = m->py(decl->file()), decl->line(), m->cxx(),
                             type = m->py(decl->type()), name = m->Tuple(decl->name()),
                             alias = m->py(decl->alias()), decl->constructed());
  addComments(tdef, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(alias);
  return tdef;
}

PyObject *Synopsis::Enumerator(AST::Enumerator* decl)
{
  Trace trace("Synopsis::addEnumerator");
  PyObject *enumor, *file, *name, *type;
  if (decl->type() == "dummy") // work around a hack with another hack ;-)
  {
     std::vector<std::string> eos;
     eos.push_back("EOS");
     enumor = PyObject_CallMethod(m_ast_module, "Builtin", "OiOOO",
                                  file = m->py(decl->file()), decl->line(), m->cxx(),
                                  type = m->py("EOS"), name = m->Tuple(eos));
  }
  else
     enumor = PyObject_CallMethod(m_ast_module, "Enumerator", "OiOOs",
                                  file = m->py(decl->file()), decl->line(), m->cxx(),
                                  name = m->Tuple(decl->name()), decl->value().c_str());
  addComments(enumor, decl);
  Py_DECREF(file);
  Py_DECREF(name);
  return enumor;
}

PyObject *Synopsis::Enum(AST::Enum* decl)
{
  Trace trace("Synopsis::addEnum");
  PyObject *enumor, *file, *enums, *name;
  enumor = PyObject_CallMethod(m_ast_module, "Enum", "OiOOO",
                               file = m->py(decl->file()), decl->line(), m->cxx(),
                               name = m->Tuple(decl->name()), enums = m->List(decl->enumerators()));
  addComments(enumor, decl);
  Py_DECREF(file);
  Py_DECREF(enums);
  Py_DECREF(name);
  return enumor;
}

PyObject *Synopsis::Variable(AST::Variable* decl)
{
  Trace trace("Synopsis::addVariable");
  PyObject *var, *file, *type, *name, *vtype;
  var = PyObject_CallMethod(m_ast_module, "Variable", "OiOOOOi",
                            file = m->py(decl->file()), decl->line(), m->cxx(),
                            type = m->py(decl->type()), name = m->Tuple(decl->name()),
                            vtype = m->py(decl->vtype()), decl->constructed());
  addComments(var, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(vtype);
  Py_DECREF(name);
  return var;
}

PyObject *Synopsis::Const(AST::Const* decl)
{
  Trace trace("Synopsis::addConst");
  PyObject *cons, *file, *type, *name, *ctype;
  cons = PyObject_CallMethod(m_ast_module, "Const", "OiOOOOOs",
                             file = m->py(decl->file()), decl->line(), m->cxx(),
                             type = m->py(decl->type()), ctype = m->py(decl->ctype()),
                             name = m->Tuple(decl->name()), decl->value().c_str());
  addComments(cons, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(ctype);
  Py_DECREF(name);
  return cons;
}

PyObject *Synopsis::Parameter(AST::Parameter* decl)
{
  Trace trace("Synopsis::Parameter");
  PyObject *param, *pre, *post, *type, *value, *name;
  param = PyObject_CallMethod(m_ast_module, "Parameter", "OOOOO",
                              pre = m->List(decl->premodifier()), type = m->py(decl->type()), 
                              post = m->List(decl->postmodifier()),
                              name = m->py(decl->name()), value = m->py(decl->value()));
  Py_DECREF(pre);
  Py_DECREF(post);
  Py_DECREF(type);
  Py_DECREF(value);
  Py_DECREF(name);
  return param;
}

PyObject *Synopsis::Function(AST::Function* decl)
{
  Trace trace("Synopsis::addFunction");
  PyObject *func, *file, *type, *name, *pre, *ret, *realname;
  func = PyObject_CallMethod(m_ast_module, "Function", "OiOOOOOO",
                             file = m->py(decl->file()), decl->line(), m->cxx(),
                             type = m->py(decl->type()), pre = m->List(decl->premodifier()),
                             ret = m->py(decl->return_type()),
                             name = m->Tuple(decl->name()), realname = m->py(decl->realname()));
  // This is necessary to prevent inf. loops in several places
  m->add(decl, func);
  PyObject* new_params;
  PyObject* params = PyObject_CallMethod(func, "parameters", NULL);
  PyObject_CallMethod(params, "extend", "O", new_params = m->List(decl->parameters()));
  if (decl->template_type())
  {
    PyObject* ttype;
    PyObject_CallMethod(func, "set_template", "O", ttype = m->py(decl->template_type()));
    Py_DECREF(ttype);
  }
  addComments(func, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(pre);
  Py_DECREF(ret);
  Py_DECREF(realname);
  Py_DECREF(params);
  Py_DECREF(new_params);
  return func;
}

PyObject *Synopsis::Operation(AST::Operation* decl)
{
  Trace trace("Synopsis::addOperation");
  PyObject *oper, *file, *type, *name, *pre, *ret, *realname;
  oper = PyObject_CallMethod(m_ast_module, "Operation", "OiOOOOOO",
                             file = m->py(decl->file()), decl->line(), m->cxx(),
                             type = m->py(decl->type()), pre = m->List(decl->premodifier()),
                             ret = m->py(decl->return_type()),
                             name = m->Tuple(decl->name()), realname = m->py(decl->realname()));
  // This is necessary to prevent inf. loops in several places
  m->add(decl, oper);
  PyObject* new_params;
  PyObject* params = PyObject_CallMethod(oper, "parameters", NULL);
  PyObject_CallMethod(params, "extend", "O", new_params = m->List(decl->parameters()));
  if (decl->template_type())
  {
    PyObject* ttype;
    PyObject_CallMethod(oper, "set_template", "O", ttype = m->py(decl->template_type()));
    Py_DECREF(ttype);
  }
  addComments(oper, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(pre);
  Py_DECREF(ret);
  Py_DECREF(realname);
  Py_DECREF(params);
  Py_DECREF(new_params);
  return oper;
}

void Synopsis::visit_declaration(AST::Declaration* decl)
{
  // Assume this is a dummy declaration
  if (m_filter->should_store(decl)) m->add(decl, Declaration(decl));
}
void Synopsis::visit_builtin(AST::Builtin* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Builtin(decl));
}
void Synopsis::visit_macro(AST::Macro* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Macro(decl));
}
void Synopsis::visit_scope(AST::Scope* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Scope(decl));
  //else
  //	m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_namespace(AST::Namespace* decl)
{
  // Namespaces are always included, because the Linker knows to combine
  // them always. The exception are "local namespaces", those created to
  // handle scopes created by braces in code.
  if (decl->type() != "local") m->add(decl, Namespace(decl));
}
void Synopsis::visit_class(AST::Class* decl)
{
  // Add if the class is in the main file, *or* if it has any members
  // declared in the main file (eg: forward declared nested classes which
  // are fully defined in this main file)
  if (m_filter->should_store(decl)) m->add(decl, Class(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_forward(AST::Forward* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Forward(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_typedef(AST::Typedef* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Typedef(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_variable(AST::Variable* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Variable(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_const(AST::Const* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Const(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_enum(AST::Enum* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Enum(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_enumerator(AST::Enumerator* decl)
{
  m->add(decl, Enumerator(decl));
}
void Synopsis::visit_function(AST::Function* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Function(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_operation(AST::Operation* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Operation(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}

void Synopsis::visit_inheritance(AST::Inheritance* decl)
{
  m->add(decl, Inheritance(decl));
}
void Synopsis::visit_parameter(AST::Parameter* decl)
{
  m->add(decl, Parameter(decl));
}
void Synopsis::visit_comment(AST::Comment* decl)
{
  m->add(decl, Comment(decl));
}

//
// Types::Visitor methods
//
/*void Synopsis::visitType(Types::Type* type) {
  m->add(type, this->Type(type));
  }*/
void Synopsis::visit_unknown(Types::Unknown* type)
{
  m->add(type, Unknown(type));
}
void Synopsis::visit_dependent(Types::Dependent* type)
{
  m->add(type, Dependent(type));
}
void Synopsis::visit_modifier(Types::Modifier* type)
{
  m->add(type, Modifier(type));
}
void Synopsis::visit_array(Types::Array *type)
{
  m->add(type, Array(type));
}
/*void Synopsis::visitNamed(Types::Named* type) {
  m->add(type, Named(type));
  }*/
void Synopsis::visit_base(Types::Base* type)
{
  m->add(type, Base(type));
}
void Synopsis::visit_declared(Types::Declared* type)
{
  if (!m_filter->should_store(type->declaration()))
    m->add(type, Unknown(type));
  else m->add(type, Declared(type));
}
void Synopsis::visit_template_type(Types::Template* type)
{
  if (!m_filter->should_store(type->declaration()))
    m->add(type, Unknown(type));
  else m->add(type, Template(type));
}
void Synopsis::visit_parameterized(Types::Parameterized* type)
{
  m->add(type, Parameterized(type));
}
void Synopsis::visit_func_ptr(Types::FuncPtr* type)
{
  m->add(type, FuncPtr(type));
}
