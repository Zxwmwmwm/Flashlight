#include "beam/pass_tracker.hpp"
#include <cassert>
#include <iostream>
using flashlight::beam::PassTracker;
int main(){
 PassTracker p;assert(!p.observe(false,true,0,2));assert(!p.observe(true,true,0,0));
 assert(!p.observe(false,true,1,2));assert(!p.observe(false,true,0,0));assert(p.observe(false,true,0,2));assert(!p.observe(false,true,0,2));assert(!p.observe(true,true,0,2));
 p.reset();assert(!p.observe(true,false,4,5));assert(!p.observe(false,true,4,5));assert(p.observe(false,true,4,6));
 p.reset();assert(!p.observe(true,false,1,2));assert(!p.observe(true,false,2,3));assert(!p.observe(false,true,1,4));assert(p.observe(false,true,2,4));
 std::cout<<"world pass heuristic tests passed\n";
}
