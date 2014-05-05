
#include "Spline.hpp"
#include <iostream>

using namespace Eigen;
using namespace spline_interpolation;

Spline::Spline(const std::vector<Eigen::VectorXd> &path,
               const Eigen::VectorXd &maxVelocity,
               const Eigen::VectorXd &maxAcceleration,
               double frequency) :
    _error(false),
    _maxVelocity(maxVelocity),
    _maxAccel(maxAcceleration),
    _frequency(frequency)
{
    if(path.size()==0)
    {
        std::cout << "Warning! Attempting to perform cubic spline on a path with zero waypoints!"
                  << std::endl;
        return;
    }
    
    if(_maxVelocity.size() != _maxAccel.size())
    {
        std::cout << "ERROR! Mismatch between max velocity vector size ("
                  << _maxVelocity.size() << ") and max acceleration vector size ("
                  << _maxAccel.size() << ")!" << std::endl;
        _error = true;
        return;
    }
    
    _output_trajectory.push_back(path[0]);
    _path_segment_map.push_back(0);
    for(size_t i=1; i<path.size(); ++i)
    {
        if(!_checkConfigSize(path[i], i))
            return;
        _interpolateNextStep(path[i],path[i-1]);
        _path_segment_map.push_back(_output_trajectory.size());
    }
}

bool Spline::_checkConfigSize(const Eigen::VectorXd &config, size_t index)
{
    if(config.size() != _maxVelocity.size())
    {
        _error = true;
        std::cout << "ERROR! Config at index " << index
                  << " has size " << config.size() << " when it should have size "
                  << _maxVelocity.size() << std::endl;
    }
    
    return !_error;
}

void Spline::_interpolateNextStep(const Eigen::VectorXd &next_config,
                      const Eigen::VectorXd &last_config)
{
    double minTime = _getMinimumTime(next_config - last_config);
    size_t ticks = ceil(minTime*_frequency);
    
    Eigen::VectorXd step_config;
    for(size_t i=1; i<ticks; ++i)
    {
        double t = double(i)/ticks;
        double u = t*t*(3-2*t);
        
        step_config = last_config + u*(next_config - last_config);
        
        _output_trajectory.push_back(step_config);
    }
    _output_trajectory.push_back(next_config);
}

double Spline::_getMinimumTime(const Eigen::VectorXd &diff)
{
    double minTime = 0;
    double time = 0;
    for(int i=0; i<diff.size(); ++i)
    {
        time = _getCubicSplineTime(diff[i], _maxVelocity[i], _maxAccel[i]);
        if( time > minTime )
        {
            minTime = time;
        }
    }
    
    return minTime;
}

double Spline::_getCubicSplineTime(double dx,
                                   double max_vel,
                                   double max_accel)
{
    max_vel = fabs(max_vel);
    max_accel = fabs(max_accel);
    
    double T_max_vel = (3*dx)/(2*max_vel);
    double T_max_accel = sqrt(6*dx/max_accel);
    
    return std::max(T_max_vel, T_max_accel);
}

size_t Spline::getPathSegmentIndex(size_t timestep)
{
    for(size_t i=0; i<_path_segment_map.size()-1; ++i)
    {
        if(timestep < _path_segment_map[i+1])
            return i;
    }
    
    return _path_segment_map.size()-1;
}
