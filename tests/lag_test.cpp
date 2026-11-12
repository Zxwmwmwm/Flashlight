#include "beam/lag.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace flashlight::beam;
bool same(Quaternion a,Quaternion b,double eps=1e-8) { return 1-std::abs(dot(a,b))<eps; }
int main() {
 const Quaternion identity{0,0,0,1}, turn{0,0,1,0}, opposite{0,0,0,-1};
 Lag l; assert(same(l.step(identity, .016, .1),identity));
 assert(same(l.step(opposite, .016, .1), identity));
 auto half=l.step(turn, .1, .1); assert(std::abs(half.z-std::sqrt(.5))<1e-8); assert(std::abs(dot(half,half)-1)<1e-9);
 Lag a,b; a.step(identity,0,.1);b.step(identity,0,.1); for(int i=0;i<10;i++)a.step(turn,.01,.1);for(int i=0;i<5;i++)b.step(turn,.02,.1);assert(same(a.step(turn,0,.1),b.step(turn,0,.1)));
 assert(same(l.step(identity,.016,0),identity));assert(same(l.step(turn,2,.1),turn));
 // Even for a very large camera turn, the lagged beam direction stays
 // behind the camera instead of snapping back to its center.
 Lag fast;fast.step(identity,.016,.2);Quaternion yaw180{0,1,0,0};
 Quaternion lagged=fast.step(yaw180,.016,.2);Quaternion direction=relativeForward(yaw180,lagged);
 assert(direction.z>0.0);assert(std::abs(direction.x*direction.x+direction.y*direction.y+direction.z*direction.z-1.0)<1e-8);
 Quaternion almost=relativeForward({0,std::sin(.7),0,std::cos(.7)},identity);
 Quaternion over=relativeForward({0,std::sin(.9),0,std::cos(.9)},identity);
 assert(almost.z<0&&over.z>0);assert(almost.x>0&&over.x>0);
 auto safe=l.step({NAN,0,0,1},.016,.1);assert(same(safe,turn)); l.reset();assert(same(l.step(identity,.016,.1),identity));
 std::cout << "beam lag math tests passed\n";
}
