#pragma once
#include <algorithm>
#include <cmath>

// Frame-rate-independent quaternion smoothing for the experimental camera-following beam.
namespace flashlight::beam {
struct Quaternion { double x, y, z, w; };
inline double dot(Quaternion a, Quaternion b) { return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }
inline bool normalize(Quaternion& q) {
    double n = dot(q,q);
    if (!std::isfinite(n) || n < 1e-12) return false;
    double inv=1.0/std::sqrt(n); q={q.x*inv,q.y*inv,q.z*inv,q.w*inv}; return true;
}
inline Quaternion interpolate(Quaternion a, Quaternion b, double t) {
    double d=dot(a,b);
    if (d<0) { b={-b.x,-b.y,-b.z,-b.w}; d=-d; }
    d=std::clamp(d,-1.0,1.0); t=std::clamp(t,0.0,1.0);
    double aa=1-t, bb=t;
    if (d<0.9995) { double theta=std::acos(d), denom=std::sin(theta); aa=std::sin((1-t)*theta)/denom; bb=std::sin(t*theta)/denom; }
    Quaternion q{aa*a.x+bb*b.x,aa*a.y+bb*b.y,aa*a.z+bb*b.z,aa*a.w+bb*b.w}; normalize(q); return q;
}
class Lag {
    Quaternion current_{0,0,0,1}; bool valid_=false;
public:
    void reset() { valid_=false; }
    Quaternion step(Quaternion camera, double dt, double halfLife) {
        if (!normalize(camera)) return current_;
        if (!valid_ || !std::isfinite(dt) || dt<0 || dt>0.5) { current_=camera; valid_=true; return current_; }
        double blend=(!std::isfinite(halfLife) || halfLife<=0) ? 1.0 : -std::expm1(-std::log(2.0)*dt/halfLife);
        current_=interpolate(current_,camera,blend); return current_;
    }
};
}
