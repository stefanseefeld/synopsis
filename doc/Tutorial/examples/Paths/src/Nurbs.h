#ifndef Nurbs_h_
#define Nurbs_h_

#include "Path.h"
#include <vector>

namespace Paths
{

/**
 * The Nurbs class. It implements a nurbs curve
 * for the given order. It is a very powerful
 * and flexible curve representation. For simpler
 * cases you may prefer to use a {@link Bezier} curve.
 */
template <size_t Order>
class Nurbs : public Path
{
public:
  /**
   * Create a new Nurbs curve.
   */
  Nurbs();
  /**
   * Inserts a control point with the given weight.
   * The knot value determines the position in the sequence.
   * @param knot the parameter value at which to insert a new knot
   * @param vertex the control point
   * @param weight the weight of the control point
   */
  void insert_control_point(double knot, const Vertex &vertex,
                            double weight);
  virtual void draw();
private:
  /**
   * The data...
   */
  std::vector<Vertex> controls_;
  std::vector<double> weights_;
  std::vector<double> knots_;
};

}

#endif
