// Experimental, opt-in GLES post-process. Function signatures and camera offset
// derived from MIT-licensed BedrockTools; see third_party/BedrockTools/LICENSE.
#include "../probe/pattern.hpp"
#include "../beam/lag.hpp"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <link.h>
#include <dlfcn.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>

namespace {
using namespace flashlight::beam;
using Clock=std::chrono::steady_clock;
constexpr char TAG[]="FlashlightTest";
constexpr unsigned char BUILD_ID[20]={0xc6,0x0f,0xa3,0x29,0x08,0x01,0x9e,0xec,0xae,0x39,0xeb,0xfe,0x7b,0xac,0x57,0x55,0xdc,0x35,0xcd,0xe2};
struct Target {const char* name;std::uintptr_t rva;const char* pattern;};
constexpr Target targets[]={
 {"RenderLevel",0xb7a38a0,"? ? ? D1 ? ? ? FD ? ? ? 6D ? ? ? 6D ? ? ? 6D ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 57 D0 3B D5"},
 {"UpdatePlayerFromCamera",0x8a42820,"? ? ? D1 ? ? ? FD ? ? ? A9 ? ? ? F9 ? ? ? A9 ? ? ? A9 ? ? ? 91 54 D0 3B D5 ? ? ? 52"}
};
std::atomic<bool> running{false},light{false};std::atomic<int> status{0};std::atomic<unsigned> frames{0};
std::atomic<float> settings[10]={{1.f},{.93f},{.72f},{.62f},{.40f},{.55f},{.32f},{.50f},{.50f},{.25f}};
std::mutex lifecycle, cameraMutex;std::thread worker;Quaternion camera{0,0,0,1};Clock::time_point cameraTime{};bool cameraValid=false;
using Hook=int(*)(void*,void*,void**,int);using Unhook=bool(*)(void*,void*);
Hook hook=nullptr;Unhook unhook=nullptr;void* preloader=nullptr;void* targetAddresses[2]={};bool installed[2]={};
using RenderFn=void(*)(void*,void*,void*);using CameraFn=void(*)(void*,void*,void*);
RenderFn originalRender=nullptr;CameraFn originalCamera=nullptr;

constexpr const char* VS=R"(#version 300 es
precision highp float;out vec2 uv;
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));uv=p;gl_Position=vec4(p*2.0-1.0,0.0,1.0);})";
constexpr const char* FS=R"(#version 300 es
precision highp float;in vec2 uv;layout(location=0) out vec4 outputColor;
uniform sampler2D scene;uniform vec2 center;uniform float aspect;uniform float radius;uniform float softness;uniform float strength;uniform float ambient;uniform vec3 tint;
void main(){vec4 source=texture(scene,uv);vec2 d=(uv-center)*vec2(aspect,1.0);float dist=length(d);float edge=max(0.02,radius*(0.08+softness*0.9));float beam=1.0-smoothstep(max(0.0,radius-edge),radius+edge,dist);vec3 lit=source.rgb*(ambient+beam*strength*3.0*tint);outputColor=vec4(clamp(lit,0.0,1.0),source.a);})";
GLuint makeShader(GLenum type,const char* src){GLuint s=glCreateShader(type);if(!s)return 0;glShaderSource(s,1,&src,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(ok)return s;char msg[512]={};glGetShaderInfoLog(s,sizeof(msg),nullptr,msg);__android_log_print(ANDROID_LOG_ERROR,TAG,"Shader compile: %s",msg);glDeleteShader(s);return 0;}
GLuint makeProgram(){GLuint v=makeShader(GL_VERTEX_SHADER,VS),f=makeShader(GL_FRAGMENT_SHADER,FS);if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return 0;}GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(ok)return p;char msg[512]={};glGetProgramInfoLog(p,sizeof(msg),nullptr,msg);__android_log_print(ANDROID_LOG_ERROR,TAG,"Shader link: %s",msg);glDeleteProgram(p);return 0;}
struct GLState {
 GLint draw=0,read=0,program=0,vao=0,active=0,tex=0,sampler=0,unpack=0,viewport[4]={};GLboolean mask[4]={};
 static constexpr GLenum caps[]={GL_BLEND,GL_DEPTH_TEST,GL_STENCIL_TEST,GL_CULL_FACE,GL_SCISSOR_TEST,GL_RASTERIZER_DISCARD,GL_SAMPLE_ALPHA_TO_COVERAGE,GL_SAMPLE_COVERAGE};GLboolean enabled[8]={};
 GLState(){glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&tex);glGetIntegerv(GL_SAMPLER_BINDING,&sampler);glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&unpack);glGetIntegerv(GL_VIEWPORT,viewport);glGetBooleanv(GL_COLOR_WRITEMASK,mask);for(int i=0;i<8;i++)enabled[i]=glIsEnabled(caps[i]);}
 ~GLState(){glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);glBindFramebuffer(GL_READ_FRAMEBUFFER,read);glUseProgram(program);glBindVertexArray(vao);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,unpack);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);glColorMask(mask[0],mask[1],mask[2],mask[3]);for(int i=0;i<8;i++)if(enabled[i])glEnable(caps[i]);else glDisable(caps[i]);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,tex);glBindSampler(0,sampler);glActiveTexture(active);}
 void prepare()const{for(auto c:caps)glDisable(c);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glBindSampler(0,0);}
};
struct Renderer {
 EGLContext context=EGL_NO_CONTEXT;EGLSurface surface=EGL_NO_SURFACE;GLuint program=0,vao=0,texture=0,fbo=0;int width=0,height=0;bool failed=false;Lag lag;Clock::time_point last{};
 void forget(){program=vao=texture=fbo=0;width=height=0;failed=false;lag.reset();last={};}
 void release(){if(program)glDeleteProgram(program);if(vao)glDeleteVertexArrays(1,&vao);if(texture)glDeleteTextures(1,&texture);if(fbo)glDeleteFramebuffers(1,&fbo);forget();}
 bool init(int w,int h){if(failed)return false;if(!program){program=makeProgram();glGenVertexArrays(1,&vao);if(!program||!vao){failed=true;return false;}}
  if(w==width&&h==height&&texture&&fbo)return true;
  if(texture) glDeleteTextures(1,&texture);
  if(fbo) glDeleteFramebuffers(1,&fbo);
  texture=fbo=0;
  glGenTextures(1,&texture);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
  glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
  if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE||glGetError()!=GL_NO_ERROR){failed=true;__android_log_print(ANDROID_LOG_ERROR,TAG,"Scene framebuffer initialization failed");return false;}width=w;height=h;return true;
 }
};
thread_local Renderer renderer;
float val(int i){return settings[i].load(std::memory_order_relaxed);}
Quaternion mul(Quaternion a,Quaternion b){return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
void render(){
 if(!running.load()||!light.load())return;
 EGLContext context=eglGetCurrentContext();EGLSurface surface=eglGetCurrentSurface(EGL_DRAW);EGLDisplay display=eglGetCurrentDisplay();
 if(context==EGL_NO_CONTEXT||surface==EGL_NO_SURFACE||display==EGL_NO_DISPLAY){if(status.exchange(2)!=2)__android_log_print(ANDROID_LOG_WARN,TAG,"No EGL context at world render callback; effect skipped");return;}
 GLint major=0,viewport[4]={};glGetIntegerv(GL_MAJOR_VERSION,&major);glGetIntegerv(GL_VIEWPORT,viewport);EGLint w=0,h=0;
 if(major<3||!eglQuerySurface(display,surface,EGL_WIDTH,&w)||!eglQuerySurface(display,surface,EGL_HEIGHT,&h)||w<64||h<64||viewport[0]!=0||viewport[1]!=0||viewport[2]!=w||viewport[3]!=h){if(status.exchange(2)!=2)__android_log_print(ANDROID_LOG_WARN,TAG,"No compatible full-screen GLES 3 render target; effect skipped");return;}
 Quaternion q;Clock::time_point t;{std::lock_guard<std::mutex> lock(cameraMutex);if(!cameraValid)return;q=camera;t=cameraTime;}
 auto now=Clock::now();if(std::chrono::duration<double>(now-t).count()>.4)return;
 if(renderer.context!=context||renderer.surface!=surface){if(renderer.context==context)renderer.release();else renderer.forget();renderer.context=context;renderer.surface=surface;}
 const double dt=renderer.last==Clock::time_point{}?0:std::chrono::duration<double>(now-renderer.last).count();renderer.last=now;
 Quaternion delayed=renderer.lag.step(q,dt,val(6)*.35);Quaternion relative=mul({-q.x,-q.y,-q.z,q.w},delayed);Quaternion dir=mul(mul(relative,{0,0,-1,0}),{-relative.x,-relative.y,-relative.z,relative.w});
 const float aspect=float(w)/h;float px=.5f+(val(7)-.5f)*.8f,py=.5f+(val(8)-.5f)*.8f;
 if(dir.z<-.15){px+=float(dir.x/(-dir.z))/(2.f*.70f*aspect);py+=float(dir.y/(-dir.z))/(2.f*.70f);}px=std::fmax(-.5f,std::fmin(1.5f,px));py=std::fmax(-.5f,std::fmin(1.5f,py));
 if(glGetError()!=GL_NO_ERROR) return;
 GLState saved;saved.prepare();GLint framebuffer=saved.draw;
 if(framebuffer){GLint component=0;glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,&component);if(component!=GL_UNSIGNED_NORMALIZED)return;}
 if(!renderer.init(w,h))return;
 glBindFramebuffer(GL_READ_FRAMEBUFFER,framebuffer);GLint readBuffer=0;glGetIntegerv(GL_READ_BUFFER,&readBuffer);glReadBuffer(framebuffer?GL_COLOR_ATTACHMENT0:GL_BACK);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,renderer.fbo);glBlitFramebuffer(0,0,w,h,0,0,w,h,GL_COLOR_BUFFER_BIT,GL_NEAREST);bool copied=glGetError()==GL_NO_ERROR;glReadBuffer(readBuffer);if(!copied){if(status.exchange(2)!=2)__android_log_print(ANDROID_LOG_WARN,TAG,"World framebuffer copy failed; effect skipped");return;}
 glBindFramebuffer(GL_DRAW_FRAMEBUFFER,framebuffer);glViewport(0,0,w,h);glUseProgram(renderer.program);glBindVertexArray(renderer.vao);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,renderer.texture);
 glUniform1i(glGetUniformLocation(renderer.program,"scene"),0);glUniform2f(glGetUniformLocation(renderer.program,"center"),px,py);glUniform1f(glGetUniformLocation(renderer.program,"aspect"),aspect);glUniform1f(glGetUniformLocation(renderer.program,"radius"),.08f+val(4)*.50f);glUniform1f(glGetUniformLocation(renderer.program,"softness"),val(5));glUniform1f(glGetUniformLocation(renderer.program,"strength"),val(3));glUniform1f(glGetUniformLocation(renderer.program,"ambient"),val(9));glUniform3f(glGetUniformLocation(renderer.program,"tint"),val(0),val(1),val(2));glDrawArrays(GL_TRIANGLES,0,3);
 if(glGetError()!=GL_NO_ERROR){__android_log_print(ANDROID_LOG_WARN,TAG,"Draw error; test frame may be incomplete");return;}if(status.exchange(3)!=3)__android_log_print(ANDROID_LOG_INFO,TAG,"Experimental GLES screen-space beam active; UI isolation is not yet verified");frames.fetch_add(1,std::memory_order_relaxed);
}
void renderHook(void* a,void* b,void* c){if(originalRender)originalRender(a,b,c);render();}
void cameraHook(void* a,void* b,void* c){if(originalCamera)originalCamera(a,b,c);if(!running.load()||!light.load()||!a||!c)return;float v[4];std::memcpy(v,static_cast<unsigned char*>(a)+0x28,sizeof(v));Quaternion q{v[0],v[1],v[2],v[3]};double n=dot(q,q);if(!std::isfinite(n)||n<.5||n>1.5||!normalize(q))return;std::lock_guard<std::mutex> lock(cameraMutex);camera=q;cameraTime=Clock::now();cameraValid=true;}
struct Found{bool found=false,build=false,valid[2]={};std::uintptr_t base=0;};
std::size_t align4(std::size_t x){return (x+3u)&~std::size_t(3u);}
int inspect(dl_phdr_info* info,std::size_t,void* data){if(!info->dlpi_name||!std::strstr(info->dlpi_name,"libminecraftpe.so"))return 0;auto& out=*static_cast<Found*>(data);out.found=true;out.base=info->dlpi_addr;
 for(ElfW(Half) i=0;i<info->dlpi_phnum;i++){auto& ph=info->dlpi_phdr[i];if(ph.p_type!=PT_NOTE||ph.p_memsz<sizeof(ElfW(Nhdr)))continue;auto* p=reinterpret_cast<const unsigned char*>(out.base+ph.p_vaddr);std::size_t at=0,size=ph.p_memsz;while(at+sizeof(ElfW(Nhdr))<=size){ElfW(Nhdr) hdr{};std::memcpy(&hdr,p+at,sizeof(hdr));at+=sizeof(hdr);auto n=align4(hdr.n_namesz),d=align4(hdr.n_descsz);if(n>size-at||d>size-at-n)break;if(hdr.n_type==3&&hdr.n_namesz==4&&hdr.n_descsz==20&&std::memcmp(p+at,"GNU",4)==0&&std::memcmp(p+at+n,BUILD_ID,20)==0)out.build=true;at+=n+d;}}
 if(!out.build)return 1;
 for(int i=0;i<2;i++){flashlight::probe::Pattern p;if(!flashlight::probe::parsePattern(targets[i].pattern,p))continue;for(ElfW(Half) j=0;j<info->dlpi_phnum;j++){auto& ph=info->dlpi_phdr[j];if(ph.p_type!=PT_LOAD||!(ph.p_flags&PF_R)||!(ph.p_flags&PF_X)||targets[i].rva<ph.p_vaddr)continue;auto off=targets[i].rva-ph.p_vaddr;if(off>ph.p_memsz||p.bytes.size()>ph.p_memsz-off)continue;out.valid[i]=flashlight::probe::countMatches(reinterpret_cast<const std::uint8_t*>(out.base+targets[i].rva),p.bytes.size(),p,1)==1;}}
 return 1;}
void uninstall(){if(unhook){if(installed[1])unhook(targetAddresses[1],reinterpret_cast<void*>(cameraHook));if(installed[0])unhook(targetAddresses[0],reinterpret_cast<void*>(renderHook));}installed[0]=installed[1]=false;}
void work(){Found f;for(int i=0;i<600&&running.load();i++){dl_iterate_phdr(inspect,&f);if(f.found)break;std::this_thread::sleep_for(std::chrono::milliseconds(500));}if(!running.load())return;
 if(!f.found||!f.build||!f.valid[0]||!f.valid[1]){status.store(-1);__android_log_print(ANDROID_LOG_ERROR,TAG,"Unsupported game build or signatures; no hooks installed");return;}
 for(int i=0;i<2;i++)targetAddresses[i]=reinterpret_cast<void*>(f.base+targets[i].rva);
 hook=reinterpret_cast<Hook>(dlsym(RTLD_DEFAULT,"pl_hook"));unhook=reinterpret_cast<Unhook>(dlsym(RTLD_DEFAULT,"pl_unhook"));if(!hook||!unhook){preloader=dlopen("libpreloader.so",RTLD_NOW|RTLD_NOLOAD);if(preloader){hook=reinterpret_cast<Hook>(dlsym(preloader,"pl_hook"));unhook=reinterpret_cast<Unhook>(dlsym(preloader,"pl_unhook"));}}
 if(!hook||!unhook){status.store(-1);__android_log_print(ANDROID_LOG_ERROR,TAG,"Preloader hook API unavailable");return;}
 auto add=[&](int i,void* detour,void** orig){if(!running.load()||hook(targetAddresses[i],detour,orig,200)!=0||!*orig)return false;installed[i]=true;return true;};
 if(!add(0,reinterpret_cast<void*>(renderHook),reinterpret_cast<void**>(&originalRender))||!add(1,reinterpret_cast<void*>(cameraHook),reinterpret_cast<void**>(&originalCamera))){uninstall();status.store(-1);__android_log_print(ANDROID_LOG_ERROR,TAG,"Hook setup failed");return;}
 status.store(1);__android_log_print(ANDROID_LOG_INFO,TAG,"Hooks ready; experimental beam starts OFF");}
}
extern "C" bool FLStart(){std::lock_guard<std::mutex> lock(lifecycle);if(running.exchange(true))return true;if(worker.joinable())worker.join();if(installed[0]&&installed[1]){status.store(1);return true;}status.store(0);try{worker=std::thread(work);}catch(...){running.store(false);status.store(-1);return false;}return true;}
extern "C" bool FLStop(){std::lock_guard<std::mutex> lock(lifecycle);light.store(false);running.store(false);if(worker.joinable())worker.join();return true;}
extern "C" bool FLUnload(){FLStop();uninstall();if(preloader){dlclose(preloader);preloader=nullptr;}return true;}
extern "C" void FLEnable(bool b){if(status.load()>0&&running.load()){light.store(b);if(!b){std::lock_guard<std::mutex> lock(cameraMutex);cameraValid=false;}__android_log_print(ANDROID_LOG_INFO,TAG,"Beam %s",b?"on":"off");}}
extern "C" void FLSetting(int i,float v){if(i>=0&&i<10&&std::isfinite(v))settings[i].store(std::fmax(0,std::fmin(1,v)));}
extern "C" int FLStatus(){return status.load();}
extern "C" unsigned FLFrames(){return frames.load();}
