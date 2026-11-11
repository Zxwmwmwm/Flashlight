#include "pattern.hpp"
#include "signatures.hpp"
#include <android/log.h>
#include <jni.h>
#include <link.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

namespace {
std::atomic<bool> running{false};
std::thread worker;
using namespace flashlight::probe;
constexpr const char* TAG = "FlashlightProbe";
int inspect(struct dl_phdr_info* info, std::size_t, void* arg) {
    if (!info->dlpi_name || !std::strstr(info->dlpi_name, "libminecraftpe.so")) return 0;
    auto* found = static_cast<bool*>(arg);
    *found = true;
    __android_log_print(ANDROID_LOG_INFO, TAG, "Minecraft library loaded; inspecting readable executable segments");
    for (const auto& sig : signatures) {
        if (!running.load()) return 1;
        Pattern pattern;
        if (!parsePattern(sig.pattern, pattern)) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "%s: invalid pattern", sig.name);
            continue;
        }
        std::size_t count = 0;
        for (ElfW(Half) i = 0; i < info->dlpi_phnum && count < 2; ++i) {
            const auto& ph = info->dlpi_phdr[i];
            if (ph.p_type != PT_LOAD || !(ph.p_flags & PF_X) || !(ph.p_flags & PF_R) || !ph.p_memsz) continue;
            auto address = static_cast<std::uintptr_t>(info->dlpi_addr) + ph.p_vaddr;
            count += countMatches(reinterpret_cast<const std::uint8_t*>(address), ph.p_memsz, pattern, 2-count);
        }
        __android_log_print(ANDROID_LOG_INFO, TAG, "%s: %s", sig.name,
            count == 0 ? "no match" : count == 1 ? "one candidate" : "multiple candidates");
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "Scan complete. Candidates are not verified hooks; no game memory was modified.");
    return 1;
}
void scanLoop() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "Probe enabled; waiting for Minecraft library");
    for (int i = 0; i < 600 && running.load(); ++i) {
        bool found = false;
        dl_iterate_phdr(inspect, &found);
        if (found) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    if (running.load()) __android_log_print(ANDROID_LOG_WARN, TAG, "Library not detected within 5 minutes");
}
bool start() {
    if (running.exchange(true)) return true;
    if (worker.joinable()) worker.join();
    try { worker = std::thread(scanLoop); }
    catch (...) { running.store(false); __android_log_print(ANDROID_LOG_ERROR, TAG, "Could not start scanner"); return false; }
    return true;
}
bool stop() {
    running.store(false);
    if (worker.joinable()) worker.join();
    __android_log_print(ANDROID_LOG_INFO, TAG, "Probe stopped");
    return true;
}
}
extern "C" __attribute__((visibility("default"))) bool PLMod_Load(JavaVM*, const void*) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "Probe loaded; this build is read-only and does not change rendering");
    return true;
}
extern "C" __attribute__((visibility("default"))) bool PLMod_Enable() { return start(); }
extern "C" __attribute__((visibility("default"))) bool PLMod_Disable() { return stop(); }
extern "C" __attribute__((visibility("default"))) bool PLMod_Unload() { return stop(); }
