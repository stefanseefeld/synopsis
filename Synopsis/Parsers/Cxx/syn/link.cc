// Synopsis C++ Parser: link.cc source file
// Implements the link module/program that reads the stored syntax
// highlighting info and original file, and generates the output HTML file.

// $Id: link.cc,v 1.25 2002/12/09 13:53:50 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2001, 2002 Stephen Davies
// Copyright (C) 2002 Stefan Seefeld
//
// Synopsis is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.

// $Log: link.cc,v $
// Revision 1.25  2002/12/09 13:53:50  chalky
// Fixed to not use the PyString's internal data for the toc list
//
// Revision 1.24  2002/11/17 12:11:43  chalky
// Reformatted all files with astyle --style=ansi, renamed fakegc.hh
//
// Revision 1.23  2002/11/01 08:15:31  chalky
// Remove debug cout
//
// Revision 1.22  2002/11/01 07:15:56  chalky
// Reject duplicate links
//
// Revision 1.21  2002/11/01 04:04:29  chalky
// Clean up HTML: no empty spans, decode &2c; in URL, to-EOL comments.
//
// Revision 1.20  2002/10/28 18:03:54  chalky
// Properly decode scoped names, and avoid crash if not using links_scope
//
// Revision 1.19  2002/10/28 17:38:17  chalky
// Ooops, get rid of #
//
// Revision 1.18  2002/10/28 17:29:09  chalky
// Add anchor link with line number to each line
//
// Revision 1.17  2002/10/25 08:56:56  chalky
// Prevent bad output by clipping to end of line
//
// Revision 1.16  2002/10/11 11:09:12  chalky
// Remove debugging perror statement

/*
 * This is the 'link' program, that reads a .links file and the original file,
 * and outputs a file with the links interspersed as HTML. It is designed such
 * that this output be placed inside another HTML file, and as such has no
 * other HTML except the actual output and the links.
 */

#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <limits.h>

#include <stdio.h>

//. Static namespace for link module
namespace
{
//. Encapsulation of a link fragment. Links can be of several types, such
//. as start, end, or some other formatting type. They are split into
//. start and end so that spans can be nested properly
struct Link
{
    //. A scoped name type
    typedef std::vector<std::string> Name;
    //. The line and column of the link
    int line, col;
    //. Enumeration of the type of link. These should be in order of
    //. priority, so nested types should be nested in this list:
    //. <a><b>foo</b></a> means enum ordering: A_START,B_START,B_END,A_END
    enum Type
    {
        LINK_START, //.< Start of a label link
        REF_START, //.< Start of reference link
        SPAN_START,
        SPAN_END,
        REF_END,
        LINK_END //.< End of a link
    };
    //. The type of this link
    Type type;
    //. The scoped name of this link
    Name name;
    //. The description of this link
    std::string desc;

    //. Less-than functor that compares column and type
    struct lt_col
    {
        bool operator() (const Link* a, const Link* b) const
        {
            if (a->col != b->col)
                return a->col < b->col;
            return a->type < b->type;
        }
    };
    //. Set of links sorted by column
    typedef std::set
        <Link*, lt_col> Line;
    //. Map of Lines keyed by line number
    typedef std::map<int, Line> Map;
    //. Write link to the output stream for debugging
    std::ostream& write(std::ostream& o);
};

//. Debugging output operator for members of the Link Map
std::ostream& operator<<(std::ostream& o, const Link::Map::value_type& linepair)
{
    const Link::Line& line = linepair.second;
    o << "Line " << (*line.begin())->line << "\n";
    Link::Line::const_iterator iter = line.begin();
    while (iter != line.end())
        (*iter++)->write(o) << "\n";
    return o;
}

//. Debugging output method for Links
std::ostream& Link::write(std::ostream& o)
{
    o << " " << col << " (" << type << ") ";
    Link::Name::const_iterator iter = name.begin();
    if (iter == name.end())
    {
        o << "<no name>";
        return o;
    }
    o << *iter++;
    while (iter != name.end())
        o << "::" << *iter++;
    return o;
}
//. Filename of the input source file
const char* input_filename = 0;
//. Filename of the output HTML file
const char* output_filename = 0;
//. Filename of the links file
const char* links_filename = 0;
//. Scope to prepend to links before to find in TOC
const char* links_scope = 0;
//. A list of TOC's to load
std::vector<std::string> toc_filenames;
//. True if should append to output file. Note that this only works if the
//. process previously using the file has flushed things to disk!
bool links_append = false;
//. A map of links to insert into the output
Link::Map links;

//. Type of the TableOfContents
typedef std::map<std::string, std::string> TOC;
//. The TOC used for looking up hrefs
TOC toc;


//. Parses the command line arguments given to main()
void parse_args(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-i"))
        {
            if (++i >= argc)
            {
                std::cerr << "-i needs argument" << std::endl;
                exit(1);
            }
            input_filename = argv[i];
        }
        else if (!strcmp(argv[i], "-o"))
        {
            if (++i >= argc)
            {
                std::cerr << "-o needs argument" << std::endl;
                exit(1);
            }
            output_filename = argv[i];
        }
        else if (!strcmp(argv[i], "-l"))
        {
            if (++i >= argc)
            {
                std::cerr << "-l needs argument" << std::endl;
                exit(1);
            }
            links_filename = argv[i];
        }
        else if (!strcmp(argv[i], "-a"))
            links_append = true;
        else if (!strcmp(argv[i], "-t"))
        {
            if (++i >= argc)
            {
                std::cerr << "-t needs argument" << std::endl;
                exit(1);
            }
            toc_filenames.push_back(argv[i]);
        }
        else
        {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            exit(1);
        }
    }
    if (!input_filename || !output_filename || !links_filename)
    {
        std::cerr << "Usage:\n";
        std::cerr << " link -i input.cc -o output.html -l links.file [ -a ]\n";
        std::cerr << " -i   in\n -o   out\n -l   links\n -a   append to out\n";
        std::cerr << std::endl;
        exit(1);
    }
}

//. Writes some text to the output. It replaces chars that might be
//. confused by HTML, such as < and >. All spaces are replaced with
//. non-breaking spaces, and tabs are expanded to 8-col tabstops (hence the
//. col argument)
void write(std::ostream& out, int col, char* buf, int len, int buflen)
{
    char* ptr = buf, *end = buf+len;
    while (ptr != end && col < buflen)
    {
        char c = *ptr++;
        switch (c)
        {
        case '<':
            out << "&lt;";
            break;
        case '>':
            out << "&gt;";
            break;
        case ' ':
            out << "&nbsp;";
            break;
        case '"':
            out << "&quot;";
            break;
        case '&':
            out << "&amp;";
            break;
        case '\t':
            {
                int next = ((col/8)+1)*8;
                while (col++ < next)
                    out << "&nbsp;";
                continue;
            }
        default:
            out << c;
        }
        col++;
    }
}

//. Writes the line number to the output
void write_lineno(std::ostream& out, int line)
{
    // out << setw(4) << line << "| "; <-- but with& nbsp;'s
    out << "<a name=\"" << line << "\"></a>";
    out << "<span class=\"file-linenum\">";
    int mag = 10000;
    while (mag > 1)
    {
        int digit = line / mag;
        if (digit)
            break;
        out << "&nbsp;";
        mag /= 10;
    }
    out << line << "|&nbsp;";
    out << "</span>";
}

//. Undoes the %FF encoding
std::string decode(const std::string& str)
{
    std::string ret;
    std::string::const_iterator iter = str.begin(), end = str.end();
    while (iter != end)
    {
        char a, b, c = *iter++;
        if (c == '%')
        {
            a = *iter++;
            b = *iter++;
            if (a >= 'a')
                a -= 'a' - 10;
            else if (a >= 'A')
                a -= 'A' - 10;
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a' - 10;
            else if (b >= 'A')
                b -= 'A' - 10;
            else
                b -= '0';
            c = a * 16 + b;
        }
        ret.push_back(c);
    }
    return ret;
}

//. Writes whatever indent there is in the buf to the output using the
//. special span. 'col' is modified to the first non-indent character.
void write_indent(std::ostream& out, char* buf, int& col, int buflen)
{
    int len = 0;
    char* ptr = buf;
    while (*ptr && (*ptr == ' ' || *ptr == '\t'))
        ptr++, len++;
    if (!len)
        return;
    out << "<span class=\"file-indent\">";
    write(out, col, buf, len, buflen);
    out << "</span>";
    col += len;
}

//. Returns true if the link is a duplicate.
bool is_duplicate(Link* link, int len)
{
    Link::Line& line = links[link->line];
    Link::Line::iterator iter = line.find(link);
    if (iter == line.end())
        return false;

    // iter matches by col, but try to find match by name
    while ((*iter)->name != link->name)
    {
        ++iter;
        if (iter == line.end() || (*iter)->col != link->col)
            // Couldn't match by name for this column
            return false;
    }

    // Matched start by name.. try and find end too
    link->col += len;
    iter = line.find(link);
    if (iter == line.end())
        return false;

    // iter matches by col, but try to find match by name
    while ((*iter)->name != link->name)
    {
        ++iter;
        if (iter == line.end() || (*iter)->col != link->col)
            // Couldn't match by name for this column
            return false;
    }
    // name matched at both ends
    return true;
}

//. Reads the links file into the 'links' map.
void read_links() throw (std::string)
{
    std::ifstream in(links_filename);
    char buf[4096];
    if (!in)
    {
        return;
    } // this is okay -- just means the file wont be linked
    std::string word, type;
    int line, len;
    while (in)
    {
        if (!(in >> line))
            break;
        Link* link = new Link;
        link->line = line;
        in >> link->col >> len >> type;
        link->col--; // we count at zero, file counts at one
        if (len == -1)
            len = INT_MAX/2; // div 2 to prevent wrap-around
        if (type != "SPAN")
        {
            if (type == "DEF")
                link->type = Link::LINK_START;
            else if (type == "REF")
                link->type = Link::REF_START;
            else if (type == "CALL")
                link->type = Link::REF_START;
            else if (type == "IMPL")
                link->type = Link::REF_START;
            else if (type == "UDIR")
                link->type = Link::REF_START;
            else
                link->type = Link::REF_START;
            int c = -1;
            // Use up field sep ' '
            in.get();
            // Loop over scoped name till next ' '
            do
            {
                in >> word;
                // Replace '160's with spaces
                //for (std::string::size_type pos = word.find(160); pos != std::string::npos; pos = word.find(160, pos)) {
                //    word[pos] = ' ';
                //}
                word = decode(word);
                size_t start = 0, end;
                while ((end = word.find('\t', start)) != std::string::npos)
                {
                    link->name.push_back(word.substr(start, end-start));
                    start = end + 1;
                }
                link->name.push_back(word.substr(start, end-start));
            }
            while (in && (c = in.get()) != '\n' && c != ' ');
            // Read description
            if (!in.getline(buf, 4096))
                break;
            link->desc = decode(buf);
        }
        else
        {
            link->type = Link::SPAN_START;
            in >> type;
            link->name.push_back(decode(type));
        }
        // Skip to next line if duplicate link
        if (is_duplicate(link, len))
            continue;
        links[line].insert(link);
        // Add link end
        Link* end = new Link;
        end->line = line;
        end->col = link->col + len;
        switch (link->type)
        {
        case Link::LINK_START:
            end->type = Link::LINK_END;
            break;
        case Link::REF_START:
            end->type = Link::REF_END;
            break;
        case Link::SPAN_START:
            end->type = Link::SPAN_END;
            break;
        default:
            ; // throw some error
        }
        links[line].insert(end);
    }
}

//. Debugging method to dump all links to cout.
void dump_links()
{
    std::copy(links.begin(), links.end(), std::ostream_iterator<Link::Map::value_type>(std::cout, "\n"));
}

//. Reads in the TOC files. The filenames are taken from the toc_filenames
//. array and store in the 'toc' map.
void read_tocs() throw (std::string)
{
    char buf[3][4096];
    int url_len = 0;
    std::vector<std::string>::iterator iter = toc_filenames.begin();
    while (iter != toc_filenames.end())
    {
        std::string toc_filename = *iter++;
        size_t pipe = toc_filename.find('|');
        if (pipe != std::string::npos)
        {
            strcpy(buf[2], toc_filename.c_str() + pipe + 1);
            url_len = toc_filename.size() - pipe - 1;
            toc_filename = toc_filename.substr(0, pipe);
        }
        std::ifstream in(toc_filename.c_str());
        if (!in)
        {
            throw std::string("Error opening toc file: ")+toc_filename;
        }
        while (in)
        {
            // Get line
            if (!in.getline(buf[0], 4096, ','))
                break;
            if (!in.getline(buf[1], 4096, ','))
                break;
            if (!in.getline(buf[2]+url_len, 4096-url_len))
                break;
            // convert& 2c;'s to commas
            for (char*s = buf[0]; *s; s++)
            {
                if (!strncmp(s, "&amp;", 5))
                {
                    *s = '&';
                    memmove(s+1, s+5, strlen(s+5)+1);
                }
                if (!strncmp(s, "&2c;", 4))
                {
                    *s = ',';
                    memmove(s+1, s+4, strlen(s+4)+1);
                }
            }
            for (char*s = buf[2]+url_len; *s; s++)
            {
                if (!strncmp(s, "&amp;", 5))
                {
                    *s = '&';
                    memmove(s+1, s+5, strlen(s+5)+1);
                }
                if (!strncmp(s, "&2c;", 4))
                {
                    *s = ',';
                    memmove(s+1, s+4, strlen(s+4)+1);
                }
            }
            // We cheat here and exclude lines that dont have
            // 'links_scope' at the start, and then chop it from the ones
            // that do
            if (links_scope)
            {
                if (strncmp(buf[0], links_scope, strlen(links_scope)))
                    continue;
                memmove(buf[0], buf[0] + strlen(links_scope), strlen(buf[0]) - strlen(links_scope) + 1);
            }
            // Store in toc map
            toc[buf[0]] = buf[2];
        }
    }
}

//. Reads the input file, inserts links, and writes the result to the
//. output. It uses the 'links' line map to iterate through the file
//. sequentially.
void link_file() throw (std::string)
{
    std::ifstream in(input_filename);
    if (!in)
    {
        throw std::string("Error opening input file: ")+input_filename;
    }
    std::ofstream out(output_filename, links_append ? std::ios::app : std::ios::out);
    if (!out)
    {
        throw std::string("Error opening output file: ")+output_filename;
    }
    char buf[4096];
    int line = 1, buflen;
    Link::Map::iterator iter = links.begin(), end = links.end();
    while (in)
    {
        // Get line
        if (!in.getline(buf, 4096))
            break;
        buflen = strlen(buf);
        write_lineno(out, line);
        // Get Link::Line
        while (iter != end && iter->first < line)
            ++iter;
        if (iter != end && iter->first == line)
        {
            // Insert links and write at same time
            int col = 0;
            write_indent(out, buf, col, buflen);
            out << "<span class=\"file-default\">";
            Link::Line& line = iter->second;
            Link::Line::iterator link_i = line.begin();
            while (link_i != line.end())
            {
                Link* link = *link_i++;
                if (col < link->col)
                {
                    write(out, col, buf+col, link->col - col, buflen);
                    col = link->col;
                }
                switch (link->type)
                {
                case Link::LINK_START:
                case Link::REF_START:
                    {
                        std::string name;
                        Link::Name::iterator name_iter = link->name.begin();
                        if (name_iter != link->name.end())
                            name = *name_iter++;
                        while (name_iter != link->name.end())
                            name += "::" + *name_iter++;
                        TOC::iterator toc_iter = toc.find(name);
                        if (toc_iter == toc.end())
                        {
                            if (link->type == Link::LINK_START)
                                out << "<a name=\"" << name;
                            else
                                out << "<a href=\"#" << name;
                        }
                        else
                        {
                            std::string href = toc_iter->second;
                            if (link->type == Link::LINK_START)
                                out << "<a class=\"file-def\" name=\""<<name<<"\"";
                            else
                                out << "<a class=\"file-ref\"";
                            out << " href=\"" << href;
                        }
                        out << "\" title=\"" << link->desc << "\">";
                        break;
                    }
                case Link::REF_END:
                case Link::LINK_END:
                    out << "</a>";
                    break;
                case Link::SPAN_START:
                    out << "<span class=\"" << link->name[0] << "\">";
                    break;
                case Link::SPAN_END:
                    out << "</span>";
                    break;
                }
            }
            // Write any left-over buffer
            write(out, col, buf+col, -1, buflen);
            out << "</span>";
        }
        else
        {
            // Write buf
            int col = 0;
            write_indent(out, buf, col, buflen);
            if (col < buflen)
            {
                out << "<span class=\"file-default\">";
                write(out, col, buf+col, -1, buflen);
                out << "</span>";
            }
        }
        out << "<br>\n";
        line++;
    }
}
}
; // namespace {anon}

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

    try
    {
        read_links();

        read_tocs();

        link_file();
    }
    catch (std::string err)
    {
        std::cerr << "Error: " << err << std::endl;
        return 1;
    }

    return 0;
}

#else

#include PYTHON_INCLUDE

extern "C"
{
    static PyObject* linkError;

    //. The main python method, equivalent to the main() function
    static PyObject* py_link(PyObject* self, PyObject* args)
    {
        PyObject *py_tocs, *py_file;

        if (!PyArg_ParseTuple(
                    args, "Ossss",
                    &py_tocs,& input_filename,& output_filename,& links_filename,& links_scope
                ))
            return NULL;

        // Extract TOC array
        int toc_size = PyList_Size(py_tocs);
        for (int i = 0; i < toc_size; i++)
        {
            if (!(py_file = PyList_GetItem(py_tocs, i)))
                return NULL;
            char* filename = PyString_AsString(py_file);
            if (!filename)
                return NULL;
            toc_filenames.push_back(filename);
        }
        // Extract filenames
        if (!(input_filename))
            return NULL;
        if (!(output_filename))
            return NULL;
        if (!(links_filename))
            return NULL;
        if (!(links_scope))
            return NULL;
        // Do stuff
        try
        {
            read_links();
            read_tocs();
            link_file();
            reset();
        }
        catch (const std::string& err)
        {
            std::cerr << "Error: " << err << std::endl;
            PyErr_SetString(linkError, err.c_str());
            reset();
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
// vim: set ts=8 sts=4 sw=4 et:
