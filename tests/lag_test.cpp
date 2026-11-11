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
 auto safe=l.step({NAN,0,0,1},.016,.1);assert(same(safe,turn)); l.reset();assert(same(l.step(identity,.016,.1),identity));
 std::cout << "beam lag math tests passed\n";
}
