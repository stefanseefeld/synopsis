/*
 * $Id: link.cc,v 1.6 2001/05/06 20:15:03 stefan Exp $
 *
 * This file is a part of Synopsis.
 * Copyright (C) 2000, 2001 Stephen Davies
 * Copyright (C) 2000, 2001 Stefan Seefeld
 *
 * Synopsis is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * $Log: link.cc,v $
 * Revision 1.6  2001/05/06 20:15:03  stefan
 * fixes to get std compliant; replaced some pass-by-value by pass-by-const-ref; bug fixes;
 *
 * Revision 1.5  2001/03/16 04:42:00  chalky
 * SXR parses expressions, handles differences from macro expansions. Some work
 * on function call resolution.
 *
 * Revision 1.4  2001/02/16 06:59:32  chalky
 * ScopePage summaries link to source
 *
 * Revision 1.3  2001/02/16 06:33:35  chalky
 * parameterized types, return types, variable types, modifiers, etc.
 *
 * Revision 1.2  2001/02/16 04:57:50  chalky
 * SXR: func parameters, namespaces, comments. Unlink temp file. a class=ref/def
 *
 * Revision 1.1  2001/02/16 02:29:55  chalky
 * Initial work on SXR and HTML integration
 *
 */

/*
 * This is the 'link' program, that reads a .links file and the original file,
 * and outputs a file with the links interspersed as HTML. It is designed such
 * that this output be placed inside another HTML file, and as such has no
 * other HTML except the actual output and the links.
 */

#include <iostream.h>
#include <fstream.h>
#include <vector>
#include <set>
#include <map>
#include <string>

//. Static namespace for link module
namespace {
    //. Encapsulation of a link fragment. Links can be of several types, such
    //. as start, end, or some other formatting type. They are split into
    //. start and end so that spans can be nested properly
    struct Link {
	//. A scoped name type
	typedef vector<string> Name;
	//. The line and column of the link
	int line, col;
	//. Enumeration of the type of link. These should be in order of
	//. priority, so nested types should be nested in this list:
	//. <a><b>foo</b></a> means enum ordering: A_START,B_START,B_END,A_END
	enum Type {
	    LINK_START, //< Start of a label link
	    REF_START, //< Start of reference link
	    SPAN_START,
	    SPAN_END,
	    REF_END, 
	    LINK_END //< End of a link
	};
	//. The type of this link
	Type type;
	//. The scoped name of this link
	Name name;

	//. Less-than functor that compares column and type
	struct lt_col {
	    bool operator() (const Link* a, const Link* b) {
		if (a->col != b->col) return a->col < b->col;
		return a->type < b->type;
	    }
	};
	//. Set of links sorted by column
	typedef set<Link*, lt_col> Line;
	//. Map of Lines keyed by line number
	typedef map<int, Line> Map;
	//. Write link to the output stream for debugging
	ostream& write(ostream& o);
    };
    //. Debugging output operator for members of the Link Map
    ostream& operator << (ostream& o, const Link::Map::value_type& linepair) {
	const Link::Line& line = linepair.second;
	o << "Line " << (*line.begin())->line << "\n";
	Link::Line::const_iterator iter = line.begin();
	while (iter != line.end())
	    (*iter++)->write(o) << "\n";
	return o;
    }
    //. Debugging output method for Links
    ostream& Link::write(ostream& o) {
	o << " " << col << " (" << type << ") ";
	Link::Name::const_iterator iter = name.begin();
	if (iter == name.end()) {
	    o << "<no name>"; return o;
	}
	o << *iter++;
	while (iter != name.end())
	    o << "::" << *iter++;
	return o;
    }
    //. Filename of the input source file
    const char* input_filename = NULL;
    //. Filename of the output HTML file
    const char* output_filename = NULL;
    //. Filename of the links file
    const char* links_filename = NULL;
    //. A list of TOC's to load
    vector<const char*> toc_filenames;
    //. True if should append to output file. Note that this only works if the
    //. process previously using the file has flushed things to disk!
    bool links_append = false;
    //. A map of links to insert into the output
    Link::Map links;
    
    //. Type of the TableOfContents
    typedef map<string, string> TOC;
    //. The TOC used for looking up hrefs 
    TOC toc;


    //. Parses the command line arguments given to main()
    void parse_args(int argc, char** argv)
    {
	for (int i = 1; i < argc; i++) {
	    if (!strcmp(argv[i], "-i")) {
		if (++i >= argc) { cerr << "-i needs argument" << endl; exit(1); }
		input_filename = argv[i];
	    } else if (!strcmp(argv[i], "-o")) {
		if (++i >= argc) { cerr << "-o needs argument" << endl; exit(1); }
		output_filename = argv[i];
	    } else if (!strcmp(argv[i], "-l")) {
		if (++i >= argc) { cerr << "-l needs argument" << endl; exit(1); }
		links_filename = argv[i];
	    } else if (!strcmp(argv[i], "-a")) {
		links_append = true;
	    } else if (!strcmp(argv[i], "-t")) {
		if (++i >= argc) { cerr << "-t needs argument" << endl; exit(1); }
		toc_filenames.push_back(argv[i]);
	    } else {
		cerr << "Unknown option: " << argv[i] << endl;
		exit(1);
	    }
	}
	if (!input_filename || !output_filename || !links_filename) {
	    cerr << "Usage:\n";
	    cerr << " link -i input.cc -o output.html -l links.file [ -a ]\n";
	    cerr << " -i   in\n -o   out\n -l   links\n -a   append to out\n";
	    cerr << endl;
	    exit(1);
	}
    }

    //. Writes some text to the output. It replaces chars that might be
    //. confused by HTML, such as < and >. All spaces are replaced with
    //. non-breaking spaces, and tabs are expanded to 8-col tabstops (hence the
    //. col argument)
    void write(ostream& out, int col, char* buf, int len)
    {
	char* ptr = buf, *end = buf+len;
	while (ptr != end && *ptr) {
	    char c = *ptr++;
	    switch (c) {
		case '<': out << "&lt;"; break;
		case '>': out << "&gt;"; break;
		case ' ': out << "&nbsp;"; break;
		case '"': out << "&quot;"; break;
		case '\t': {
		    int next = ((col/8)+1)*8;
		    while (col++ < next) out << "&nbsp;";
		    continue;
		}
		default: out << c;
	    }
	    col++;
	}
    }

    //. Writes the line number to the output
    void write_lineno(ostream& out, int line)
    {
	// out << setw(4) << line << "| "; <-- but with &nbsp;'s
	out << "<span class=\"file-linenum\">";
	int mag = 10000;
	while (mag > 1) {
	    int digit = line / mag;
	    if (digit) break;
	    out << "&nbsp;";
	    mag /= 10;
	}
	out << line << "| ";
	out << "</span>";
    }

    //. Writes whatever indent there is in the buf to the output using the
    //. special span. 'col' is modified to the first non-indent character.
    void write_indent(ostream&out, char* buf, int& col)
    {
	int len = 0;
	char* ptr = buf;
	while (*ptr && (*ptr == ' ' || *ptr == '\t')) ptr++, len++;
	if (!len) return;
	out << "<span class=\"file-indent\">";
	write(out, col, buf, len);
	out << "</span>";
	col += len;
    }

    //. Reads the links file into the 'links' map.
    void read_links() throw (string)
    {
	ifstream in(links_filename);
	if (!in) { return; } // this is okay -- just means the file wont be linked
	string word;
	int line, len;
	string type;
	while (in) {
	    if (!(in >> line)) break;
	    Link* link = new Link;
	    link->line = line;
	    in >> link->col >> len >> type;
	    link->col--; // we count at zero, file counts at one
	    if (type == "DEF" || type == "REF") {
		if (type == "DEF") link->type = Link::LINK_START;
		else link->type = Link::REF_START;
		while (in && in.get() != '\n') {
		    in >> word;
		    // Replace '160's with spaces
		    for (string::size_type pos = word.find(160); pos != string::npos; pos = word.find(160, pos)) {
			word[pos] = ' ';
		    }
		    link->name.push_back(word);
		}
	    } else  {
		link->type = Link::SPAN_START;
		link->name.push_back(type);
	    }
	    links[line].insert(link);
	    // Add link end
	    Link* end = new Link;
	    end->line = line;
	    end->col = link->col + len;
	    switch (link->type) {
		case Link::LINK_START: end->type = Link::LINK_END; break;
		case Link::REF_START: end->type = Link::REF_END; break;
		case Link::SPAN_START: end->type = Link::SPAN_END; break;
		default: ; // throw some error
	    }
	    links[line].insert(end);
	}
    }

    //. Debugging method to dump all links to cout.
    void dump_links()
    {
	copy(links.begin(), links.end(), ostream_iterator<Link::Map::value_type>(cout, "\n"));
    }

    //. Reads in the TOC files. The filenames are taken from the toc_filenames
    //. array and store in the 'toc' map.
    void read_tocs() throw (string)
    {
	char buf[3][4096];
	vector<const char*>::iterator iter = toc_filenames.begin();
	while (iter != toc_filenames.end()) {
	    const char* toc_filename = *iter++;
	    ifstream in(toc_filename);
	    if (!in) { throw string("Error opening toc file: ")+toc_filename; }
	    while (in) {
		// Get line
		if (!in.getline(buf[0], 4096, ',')) break;
		if (!in.getline(buf[1], 4096, ',')) break;
		if (!in.getline(buf[2], 4096)) break;
		// convert &2c;'s to commas
		for (char*s = buf[0]; *s; s++) {
		    if (!strncmp(s, "&2c;", 4)) {
			*s = ',';
			memmove(s+1, s+4, strlen(s+4)+1);
		    }
		}
		// Store in toc map
		toc[buf[0]] = buf[2];
	    }
	}
    }

    //. Reads the input file, inserts links, and writes the result to the
    //. output. It uses the 'links' line map to iterate through the file
    //. sequentially.
    void link_file() throw (string)
    {
	ifstream in(input_filename);
	if (!in) { throw string("Error opening input file: ")+input_filename; }
	ofstream out(output_filename);
	if (!out) { throw string("Error opening output file: ")+output_filename; }
	char buf[4096];
	int line = 1;
	Link::Map::iterator iter = links.begin(), end = links.end();
	while (in) {
	    // Get line
	    if (!in.getline(buf, 4096)) break;
	    write_lineno(out, line);
	    // Get Link::Line
	    while (iter != end && iter->first < line) ++iter;
	    if (iter != end && iter->first == line) {
		// Insert links and write at same time
		int col = 0;
		write_indent(out, buf, col);
		out << "<span class=\"file-default\">";
		Link::Line& line = iter->second;
		Link::Line::iterator link_i = line.begin();
		while (link_i != line.end()) {
		    Link* link = *link_i++;
		    if (col < link->col) {
			write(out, col, buf+col, link->col - col);
			col = link->col;
		    }
		    switch (link->type) {
			case Link::LINK_START: 
			case Link::REF_START:
			    {
				string name;
				Link::Name::iterator name_iter = link->name.begin();
				if (name_iter != link->name.end()) name = *name_iter++;
				while (name_iter != link->name.end())
				    name += "::" + *name_iter++;
				TOC::iterator toc_iter = toc.find(name);
				if (toc_iter == toc.end()) {
				    out << "<a href=\"#" << name;
				} else {
				    string href = toc_iter->second;
				    if (link->type == Link::LINK_START)
					out << "<a class=\"file-def\" name=\""<<name<<"\"";
				    else
					out << "<a class=\"file-ref\"";
				    out << " href=\"" << href;
				}
				out << "\" title=\"" << name << "\">";
				break;
			    }
			case Link::REF_END:
			case Link::LINK_END:
			    out << "</a>"; break;
			case Link::SPAN_START:
			    out << "<span class=\"" << link->name[0] << "\">";
			    break;
			case Link::SPAN_END:
			    out << "</span>"; break;
		    }
		}
		// Write any left-over buffer
		write(out, col, buf+col, -1);
		out << "</span>";
	    } else {
		// Write buf
		int col = 0;
		write_indent(out, buf, col);
		out << "<span class=\"file-default\">";
		write(out, col, buf+col, -1);
		out << "</span>";
	    }
	    out << "<br>\n";
	    line++;
	}
    }
}; // namespace {anon}

//. Clear all global vars
void reset()
{
    links.clear();
    toc.clear();
    toc_filenames.clear();
}

#ifdef STANDALONE

//. Method for a stand-alone 'link-synopsis' program
int main(int argc, char** argv)
{
    parse_args(argc, argv);

    try {
	read_links();

	read_tocs();

	link_file();
    } catch (string err) {
	cerr << "Error: " << err << endl;
	return 1;
    }

    return 0;
}

#else

// Python module
#if PYTHON_MAJOR == 2
#  if PYTHON_MINOR == 0
#    include <python2.0/Python.h>
#  else
#    error "this python version is not supported yet"
#  endif
#elif PYTHON_MAJOR == 1
#  if PYTHON_MINOR == 6
#    include <python1.6/Python.h>
#  elif PYTHON_MINOR == 5
#    include <python1.5/Python.h>
#  else
#    error "this python version is not supported yet"
#  endif
#endif

extern "C" {
    static PyObject* linkError;

    //. The main python method, equivalent to the main() function
    static PyObject* py_link(PyObject* self, PyObject* args)
    {
	PyObject *py_tocs, *py_file;

	if (!PyArg_ParseTuple(
		args, "Osss",
		&py_tocs, &input_filename, &output_filename, &links_filename
	    )) return NULL;

	// Extract TOC array
	int toc_size = PyList_Size(py_tocs);
	for (int i = 0; i < toc_size; i++) {
	    if (!(py_file = PyList_GetItem(py_tocs, i))) return NULL;
	    char* filename = PyString_AsString(py_file);
	    if (!filename) return NULL;
	    toc_filenames.push_back(filename);
	}
	// Extract filenames
	if (!(input_filename)) return NULL;
	if (!(output_filename)) return NULL;
	if (!(links_filename)) return NULL;
	// Do stuff
	try {
	    read_links();
	    read_tocs();
	    link_file();
	    reset();
	} catch (string err) {
	    cerr << "Error: " << err << endl;
	    PyErr_SetString(linkError, err.c_str());
	    return NULL;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
    }

    //. The list of methods in this python module
    static PyMethodDef link_methods[] =
    {
	{(char*)"link", py_link, METH_VARARGS},
	{NULL, NULL}
    };

    //. The initialisation method for this module
    void initlink()
    {
	PyObject* m = Py_InitModule((char*)"link", link_methods);
	PyObject_SetAttrString(m, (char*)"version", PyString_FromString("0.1"));
	
	// Add an exception called "error" to the module
	PyObject* d = PyModule_GetDict(m);
	linkError = PyErr_NewException("link.error", NULL, NULL);
	PyDict_SetItemString(d, "error", linkError);
    }

};


#endif
