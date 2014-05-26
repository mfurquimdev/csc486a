#ifndef NG_CATMULLROMSPLINE_HPP
#define NG_CATMULLROMSPLINE_HPP

#include "ng/engine/linearalgebra.hpp"

#include <vector>

namespace ng
{

template<class T>
class CatmullRomSpline
{
public:
    std::vector<genType_base<T,3>> ControlPoints;

    vec3 CalculatePoint(std::size_t segmentIndex, T t) const
    {
        if (ControlPoints.size() < 4)
        {
            throw std::logic_error("Not enough points for Catmull-Rom (need at least 4)");
        }

        if (segmentIndex > ControlPoints.size() - 4)
        {
            throw std::logic_error("Not enough points for Catmull-Rom (need at least 2 control points before and after each segment)");
        }

        return T(0.5) * ( ( T(2) * ControlPoints[segmentIndex + 1] ) +
                        ( - ControlPoints[segmentIndex] + ControlPoints[segmentIndex + 2]) * t +
                        ( T(2) * ControlPoints[segmentIndex] - T(5) * ControlPoints[segmentIndex + 1] + T(4) * ControlPoints[segmentIndex + 2] - ControlPoints[segmentIndex + 3]) * t * t +
                        ( - ControlPoints[segmentIndex] + T(3) * ControlPoints[segmentIndex + 1] - T(3) * ControlPoints[segmentIndex + 2] + ControlPoints[segmentIndex + 3]) * t * t * t);
    }
};

} // end namespace ng

#endif // NG_CATMULLROMSPLINE_HPP
