#pragma once
namespace flashlight::beam {
// A conservative heuristic, not a verified Minecraft render-pass API.
// A full-screen color clear begins a scene; a later depth-only clear on the
// same framebuffer, with a different active program, is a candidate boundary.
class PassTracker {
    bool started_=false,processed_=false;
    int framebuffer_=-1,program_=0;
public:
    void reset(){started_=false;processed_=false;framebuffer_=-1;program_=0;}
    bool observe(bool color,bool depth,int framebuffer,int program){
        if(color){if(!processed_){started_=true;framebuffer_=framebuffer;program_=program;}return false;}
        if(!depth||!started_||processed_||framebuffer!=framebuffer_||program==0||program==program_)return false;
        processed_=true;return true;
    }
};
}
