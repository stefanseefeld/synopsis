#ifndef _Polyline_h
#define _Polyline_h

#include "Path.h"
#include <vector>

namespace Paths
{

// The Polyline class. It is an ordered set of
// connected line segments.
class Polyline : public Path
{
public:
  // Create a new Polyline.
  //
  Polyline();
  // @group Manipulators {

  // Add a new vertex.
  void add_vertex(const Vertex &);
  // Remove the vertex at index i.
  void remove_vertex(size_t i);
  // }
  virtual void draw();
private:
  // The data...
  std::vector<Vertex> _vertices;
};

}

#endif
