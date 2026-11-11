#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace flashlight::probe {
struct Pattern {
    std::vector<std::uint8_t> bytes;
    std::vector<bool> known;
    std::size_t anchorStart = 0;
    std::size_t anchorLength = 0;
};
inline bool parsePattern(const std::string& value, Pattern& out) {
    out = Pattern{};
    std::size_t pos = 0;
    while (pos < value.size()) {
        while (pos < value.size() && value[pos] == ' ') ++pos;
        if (pos == value.size()) break;
        auto end = value.find(' ', pos);
        if (end == std::string::npos) end = value.size();
        auto token = value.substr(pos, end - pos);
        if (token == "?" || token == "??") {
            out.bytes.push_back(0); out.known.push_back(false);
        } else {
            if (token.size() != 2) return false;
            auto digit = [](char c) -> int {
                if (c >= '0' && c <= '9') return c-'0';
                if (c >= 'a' && c <= 'f') return c-'a'+10;
                if (c >= 'A' && c <= 'F') return c-'A'+10;
                return -1;
            };
            int a = digit(token[0]), b = digit(token[1]);
            if (a < 0 || b < 0) return false;
            out.bytes.push_back(static_cast<std::uint8_t>((a << 4) | b));
            out.known.push_back(true);
        }
        pos = end;
    }
    if (out.bytes.empty()) return false;
    for (std::size_t i = 0; i < out.bytes.size();) {
        if (!out.known[i]) { ++i; continue; }
        std::size_t end = i;
        while (end < out.bytes.size() && out.known[end]) ++end;
        if (end-i > out.anchorLength) { out.anchorStart = i; out.anchorLength = end-i; }
        i = end;
    }
    return out.anchorLength != 0;
}
// Count at most `limit` matches. Callers must provide a valid readable range.
inline std::size_t countMatches(const std::uint8_t* data, std::size_t size,
                                const Pattern& p, std::size_t limit = 2) {
    if (!data || !limit || p.bytes.empty() || !p.anchorLength || size < p.bytes.size()) return 0;
    std::size_t matches = 0;
    const std::size_t last = size - p.bytes.size();
    std::size_t start = 0;
    while (start <= last) {
        const auto anchor = p.bytes[p.anchorStart];
        auto* found = static_cast<const std::uint8_t*>(
            std::memchr(data + start + p.anchorStart, anchor, last - start + 1));
        if (!found) break;
        const std::size_t candidate = static_cast<std::size_t>(found - data) - p.anchorStart;
        if (std::memcmp(found, p.bytes.data() + p.anchorStart, p.anchorLength) == 0) {
            bool valid = true;
            for (std::size_t i = 0; i < p.bytes.size(); ++i) {
                if (p.known[i] && data[candidate+i] != p.bytes[i]) { valid = false; break; }
            }
            if (valid && ++matches == limit) break;
        }
        start = candidate + 1;
    }
    return matches;
}
}
