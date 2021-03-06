#ifndef SPLINE_HPP
#define SPLINE_HPP

#include <Eigen/Geometry>
#include <vector>

namespace spline_interpolation {

class Spline
{
public:
    Spline(const std::vector<Eigen::VectorXd>& path,
           const Eigen::VectorXd& maxVelocity,
           const Eigen::VectorXd& maxAcceleration,
           double frequency = 200);
    
    inline const std::vector<Eigen::VectorXd>& getTrajectory() { return _output_trajectory; }
    size_t getPathSegmentIndex(size_t timestep);
    inline bool valid() const { return !_error; }
    
protected:
    
    size_t _current_step_num;
    bool _error;
    Eigen::VectorXd _maxVelocity;
    Eigen::VectorXd _maxAccel;
    double _frequency;
    
    bool _checkConfigSize(const Eigen::VectorXd& config, size_t index);
    void _interpolateNextStep(const Eigen::VectorXd& next_config,
                       const Eigen::VectorXd& last_config);
    
    double _getMinimumTime(const Eigen::VectorXd& diff);
    double _getCubicSplineTime(double dx, double max_vel, double max_accel,
                                      std::string& reason, size_t joint_num);
    
    std::vector<Eigen::VectorXd> _output_trajectory;
    std::vector<size_t> _path_segment_map;
};


} // namespace spline_interpolation

#endif // SPLINE_HPP
