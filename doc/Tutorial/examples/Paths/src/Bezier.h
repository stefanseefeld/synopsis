#ifndef _Bezier_h
#define _Bezier_h

#include "Path.h"
#include <vector>

namespace Paths
{

//.
//. The Bezier class. It implements a bezier curve
//. for the given order.
//.
template <size_t Order>
class Bezier : public Path
{
public:
  //. Create a new Bezier.
  Bezier();

  //. @group Manipulators {

  //. Add a new control point.
  void add_control_point(const Vertex &);

  //. Remove the control point at index i.
  void remove_control_point(size_t i);
  //. }
  virtual void draw();
private:
  //. The data...
  std::vector<Vertex> _controls;
};

}

#endif
