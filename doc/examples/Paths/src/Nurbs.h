#ifndef Nurbs_h_
#define Nurbs_h_

#include "Path.h"
#include <vector>

namespace Paths
{

//. The Nurbs class. It implements a nurbs curve
//. for the given order. It is a very powerful
//. and flexible curve representation. For simpler
//. cases you may prefer to use a `Bezier` curve.
//.
//. While non-rational curves are not sufficient to represent a circle,
//. this is one of many sets of NURBS control points for an almost uniformly 
//. parameterized circle:
//.
//. +--+----+-------------+
//. |x |  y | weight      |
//. +==+====+=============+
//. |1 |  0 | 1           |
//. +--+----+-------------+
//. |1 |  1 | `sqrt(2)/2` |
//. +--+----+-------------+
//. |0 |  1 | 1           |
//. +--+----+-------------+
//. |-1|  1 | `sqrt(2)/2` |
//. +--+----+-------------+
//. |-1|  0 | 1           |
//. +--+----+-------------+
//. |-1| -1 | `sqrt(2)/2` |
//. +--+----+-------------+
//. |0 | -1 | 1           |
//. +--+----+-------------+
//. |1 | -1 | `sqrt(2)/2` |
//. +--+----+-------------+
//. |1 |  0 | 1           |
//. +--+----+-------------+
//.
//. The order is three, the knot vector is {0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4}.
//. It should be noted that the circle is composed of four quarter circles,
//. tied together with double knots. Although double knots in a third order NURBS
//. curve would normally result in loss of continuity in the first derivative,
//. the control points are positioned in such a way that the first derivative is continuous.
//. (From Wikipedia_ )
//.
//. .. _Wikipedia: http://en.wikipedia.org/wiki/NURBS
//.
//. Example::
//.
//.   Nurbs<3> circle;
//.   circle.insert_control_point(0, Vertex(1., 0.), 1.);
//.   circle.insert_control_point(0, Vertex(1., 1.), sqrt(2.)/2.);
//.   ...
//.
template <std::size_t Order>
class Nurbs : public Path
{
public:
  //. Create a new Nurbs curve.
  Nurbs();
  //. Inserts a control point with the given weight.
  //. The knot value determines the position in the sequence.
  //.
  //. Parameters:
  //.   :knot: the parameter value at which to insert a new knot
  //.   :vertex: the control point
  //.   :weight: the weight of the control point
  void insert_control_point(double knot, const Vertex &vertex,
                            double weight);
  virtual void draw();
private:
  //. The data...
  std::vector<Vertex> controls_;
  std::vector<double> weights_;
  std::vector<double> knots_;
};

}

#endif
