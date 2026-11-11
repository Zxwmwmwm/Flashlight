#include "probe/pattern.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace flashlight::probe;
int main() {
    Pattern p;
    assert(parsePattern("? 10 20 ? 30", p));
    const std::uint8_t bytes[] = {1,0x10,0x20,2,0x30, 4,0x10,0x20,9,0x30};
    assert(countMatches(bytes, sizeof(bytes), p, 3) == 2);
    assert(countMatches(bytes, sizeof(bytes), p, 1) == 1);
    assert(countMatches(bytes, 4, p) == 0);
    assert(parsePattern("10 20", p));
    assert(countMatches(bytes, sizeof(bytes), p) == 2);
    assert(!parsePattern("? ??", p));
    assert(!parsePattern("GG", p));
    assert(!parsePattern("1", p));
    assert(!parsePattern("", p));
    assert(parsePattern("30", p));
    assert(countMatches(bytes, sizeof(bytes), p, 3) == 2);
    assert(parsePattern("10 20 ? 30", p));
    assert(countMatches(bytes + 1, 4, p) == 1);
    std::cout << "signature matcher tests passed\n";
}
