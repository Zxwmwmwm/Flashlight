// Experimental, opt-in GLES post-process. Function signatures and camera offset
// derived from MIT-licensed BedrockTools; see README.md (BedrockTools license section).
#include "../probe/pattern.hpp"
#include "../beam/lag.hpp"
#include "../beam/pass_tracker.hpp"
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
std::atomic<bool> running{false},light{false},worldVisible{false};std::atomic<int> status{0};std::atomic<unsigned> frames{0},clearHits{0},candidateHits{0},cameraHits{0},swapHits{0};
std::atomic<float> settings[14]={{1.f},{.93f},{.72f},{.62f},{.40f},{.55f},{.32f},{.50f},{.50f},{.25f},{1.f},{0.f},{.94f},{.92f}};
std::mutex lifecycle, cameraMutex;std::thread worker;Quaternion camera{0,0,0,1};Clock::time_point cameraTime{};bool cameraValid=false;
using Hook=int(*)(void*,void*,void**,int);using Unhook=bool(*)(void*,void*);
Hook hook=nullptr;Unhook unhook=nullptr;void* preloader=nullptr;void* glLibrary=nullptr;void* eglLibrary=nullptr;
void* hookTargets[6]={};void* hookDetours[6]={};bool installed[6]={};
using CameraFn=void(*)(void*,void*,void*);CameraFn originalCamera=nullptr;
using ClearFn=void(*)(GLbitfield);using ClearFvFn=void(*)(GLenum,GLint,const GLfloat*);using ClearFiFn=void(*)(GLenum,GLint,GLfloat,GLint);
using SwapFn=EGLBoolean(*)(EGLDisplay,EGLSurface);
ClearFn originalClear=nullptr;ClearFvFn originalClearFv=nullptr;ClearFiFn originalClearFi=nullptr;SwapFn originalSwap=nullptr;
thread_local bool processing=false;thread_local PassTracker passTracker;thread_local EGLContext passContext=EGL_NO_CONTEXT;thread_local EGLSurface passSurface=EGL_NO_SURFACE;

constexpr const char* VS=R"(#version 300 es
precision highp float;out vec2 uv;
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));uv=p;gl_Position=vec4(p*2.0-1.0,0.0,1.0);})";
constexpr const char* FS=R"(#version 300 es
precision highp float;precision highp sampler2D;in vec2 uv;layout(location=0) out vec4 outputColor;
uniform sampler2D scene;uniform sampler2D sceneDepth;uniform vec2 center;uniform vec3 direction;
uniform float aspect;uniform float innerCos;uniform float outerCos;uniform float strength;uniform float ambient;uniform vec3 tint;
uniform float depthAvailable;uniform float reversedDepth;uniform float coneEnabled;uniform float coneRadius;uniform vec2 coneOrigin;
void main(){
 vec4 source=texture(scene,uv);
 vec3 ray=normalize(vec3((uv-center)*vec2(aspect,1.0)*1.4,-1.0));
 float beam=smoothstep(outerCos,innerCos,dot(ray,direction));
 float worldMask=1.0;
 if(depthAvailable>0.5){float d=texture(sceneDepth,uv).r;float nearDepth=mix(d,1.0-d,reversedDepth);worldMask=1.0-smoothstep(0.99980,0.99998,nearDepth);}
 beam*=worldMask;
 vec3 lit=source.rgb*(ambient+beam*strength*3.0*tint);
 if(coneEnabled>0.5){
  // Stylized screen-space shaft, not volumetric geometry or a held item.
  vec2 origin=coneOrigin*vec2(aspect,1.0);
  vec2 aim=center+direction.xy/(vec2(aspect,1.0)*1.4*max(0.12,-direction.z));
  aim=clamp(aim,vec2(-2.5),vec2(3.5))*vec2(aspect,1.0);
  vec2 axis=aim-origin;float axisLen=length(axis);
  float along=dot(uv*vec2(aspect,1.0)-origin,axis)/max(dot(axis,axis),0.0001);
  float dist=length((uv*vec2(aspect,1.0)-origin)-axis*along);
  // At far/sky depth extend the shaft past the aim point, while keeping
  // the surface spotlight masked. With no depth copy, use the sky profile.
  float farBackground=depthAvailable>0.5?1.0-worldMask:1.0;
  float width=mix(0.008,max(0.025,min(0.22,coneRadius*0.65)),clamp(along,0.0,mix(1.0,2.5,farBackground)));
  float silhouette=1.0-smoothstep(width,width+max(0.015,width*0.35),dist);
  float surfaceEnd=1.0-smoothstep(0.85,1.08,along);
  float skyEnd=1.0-smoothstep(2.4,3.0,along);
  float lengthFade=smoothstep(0.0,0.08,along)*mix(surfaceEnd,skyEnd,farBackground);
  float cone=silhouette*lengthFade*smoothstep(0.04,0.30,-direction.z)*smoothstep(0.02,0.12,axisLen);
  lit+=tint*cone*strength*0.14;
 }
 outputColor=vec4(clamp(lit,0.0,1.0),source.a);
})";
GLuint makeShader(GLenum type,const char* src){GLuint s=glCreateShader(type);if(!s)return 0;glShaderSource(s,1,&src,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(ok)return s;char msg[512]={};glGetShaderInfoLog(s,sizeof(msg),nullptr,msg);__android_log_print(ANDROID_LOG_ERROR,TAG,"Shader compile: %s",msg);glDeleteShader(s);return 0;}
GLuint makeProgram(){GLuint v=makeShader(GL_VERTEX_SHADER,VS),f=makeShader(GL_FRAGMENT_SHADER,FS);if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return 0;}GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(ok)return p;char msg[512]={};glGetProgramInfoLog(p,sizeof(msg),nullptr,msg);__android_log_print(ANDROID_LOG_ERROR,TAG,"Shader link: %s",msg);glDeleteProgram(p);return 0;}
struct GLState {
 GLint draw=0,read=0,program=0,vao=0,active=0,tex[2]={},sampler[2]={},unpack=0,viewport[4]={};GLboolean mask[4]={};
 static constexpr GLenum caps[]={GL_BLEND,GL_DEPTH_TEST,GL_STENCIL_TEST,GL_CULL_FACE,GL_SCISSOR_TEST,GL_RASTERIZER_DISCARD,GL_SAMPLE_ALPHA_TO_COVERAGE,GL_SAMPLE_COVERAGE};GLboolean enabled[8]={};
 GLState(){glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);glGetIntegerv(GL_ACTIVE_TEXTURE,&active);for(int i=0;i<2;i++){glActiveTexture(GL_TEXTURE0+i);glGetIntegerv(GL_TEXTURE_BINDING_2D,&tex[i]);glGetIntegerv(GL_SAMPLER_BINDING,&sampler[i]);}glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&unpack);glGetIntegerv(GL_VIEWPORT,viewport);glGetBooleanv(GL_COLOR_WRITEMASK,mask);for(int i=0;i<8;i++)enabled[i]=glIsEnabled(caps[i]);}
 ~GLState(){glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);glBindFramebuffer(GL_READ_FRAMEBUFFER,read);glUseProgram(program);glBindVertexArray(vao);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,unpack);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);glColorMask(mask[0],mask[1],mask[2],mask[3]);for(int i=0;i<8;i++)if(enabled[i])glEnable(caps[i]);else glDisable(caps[i]);for(int i=0;i<2;i++){glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,tex[i]);glBindSampler(i,sampler[i]);}glActiveTexture(active);}
 void prepare()const{for(auto c:caps)glDisable(c);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glBindSampler(0,0);glBindSampler(1,0);}
};
struct Renderer {
 EGLContext context=EGL_NO_CONTEXT;EGLSurface surface=EGL_NO_SURFACE;GLuint program=0,vao=0,texture=0,depthTexture=0,fbo=0;int width=0,height=0;GLenum depthFormat=0,rejectedDepthFormat=0;bool failed=false;Lag lag;Clock::time_point last{};
 void forget(){program=vao=texture=depthTexture=fbo=0;width=height=0;depthFormat=rejectedDepthFormat=0;failed=false;lag.reset();last={};}
 void release(){if(program)glDeleteProgram(program);if(vao)glDeleteVertexArrays(1,&vao);if(texture)glDeleteTextures(1,&texture);if(depthTexture)glDeleteTextures(1,&depthTexture);if(fbo)glDeleteFramebuffers(1,&fbo);forget();}
 bool init(int w,int h,GLenum format){if(failed)return false;if(!program){program=makeProgram();glGenVertexArrays(1,&vao);if(!program||!vao){failed=true;return false;}}
  if(w==width&&h==height&&texture&&fbo&&format==depthFormat)return true;
  if(texture) glDeleteTextures(1,&texture);
  if(depthTexture) glDeleteTextures(1,&depthTexture);
  if(fbo) glDeleteFramebuffers(1,&fbo);
  texture=depthTexture=fbo=0;
  glGenTextures(1,&texture);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
  glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
  if(format){
   bool packed=format==GL_DEPTH24_STENCIL8||format==GL_DEPTH32F_STENCIL8;
   GLenum type=format==GL_DEPTH_COMPONENT16?GL_UNSIGNED_SHORT:format==GL_DEPTH_COMPONENT32F?GL_FLOAT:format==GL_DEPTH32F_STENCIL8?GL_FLOAT_32_UNSIGNED_INT_24_8_REV:packed?GL_UNSIGNED_INT_24_8:GL_UNSIGNED_INT;
   glGenTextures(1,&depthTexture);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,depthTexture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);glTexImage2D(GL_TEXTURE_2D,0,format,w,h,0,packed?GL_DEPTH_STENCIL:GL_DEPTH_COMPONENT,type,nullptr);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,packed?GL_DEPTH_STENCIL_ATTACHMENT:GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depthTexture,0);
   bool depthOk=glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE&&glGetError()==GL_NO_ERROR;
   if(!depthOk){rejectedDepthFormat=format;glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,packed?GL_DEPTH_STENCIL_ATTACHMENT:GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);glDeleteTextures(1,&depthTexture);depthTexture=0;format=0;for(int i=0;i<8&&glGetError()!=GL_NO_ERROR;i++){}__android_log_print(ANDROID_LOG_WARN,TAG,"Depth texture unavailable; sky masking disabled");}
  }
  if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE||glGetError()!=GL_NO_ERROR){failed=true;__android_log_print(ANDROID_LOG_ERROR,TAG,"Scene framebuffer initialization failed");return false;}
  width=w;height=h;depthFormat=format;return true;
 }
};
thread_local Renderer renderer;
float val(int i){return settings[i].load(std::memory_order_relaxed);}
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
 Quaternion delayed=renderer.lag.step(q,dt,val(6)*.35);Quaternion dir=relativeForward(q,delayed);
 const float aspect=float(w)/h;float px=.5f+(val(7)-.5f)*.8f,py=.5f+(val(8)-.5f)*.8f;
 const float radius=.08f+val(4)*.50f,edge=std::fmax(.02f,radius*(.08f+val(5)*.9f));
 const float innerCos=std::cos(std::atan(std::fmax(.001f,radius-edge)*1.4f));
 const float outerCos=std::cos(std::atan((radius+edge)*1.4f));
 GLint depthBits=0,stencilBits=0;GLfloat clearDepth=1.f;glGetIntegerv(GL_DEPTH_BITS,&depthBits);glGetIntegerv(GL_STENCIL_BITS,&stencilBits);glGetFloatv(GL_DEPTH_CLEAR_VALUE,&clearDepth);
 bool maskRequested=val(10)>.5f;
 GLenum format=!maskRequested||depthBits==0?0:depthBits<=16?GL_DEPTH_COMPONENT16:depthBits<=24?(stencilBits?GL_DEPTH24_STENCIL8:GL_DEPTH_COMPONENT24):(stencilBits?GL_DEPTH32F_STENCIL8:GL_DEPTH_COMPONENT32F);
 if(format==renderer.rejectedDepthFormat)format=0;
 if(glGetError()!=GL_NO_ERROR) return;
 GLState saved;saved.prepare();GLint framebuffer=saved.draw;
 if(framebuffer){GLint component=0;glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,&component);if(component!=GL_UNSIGNED_NORMALIZED)return;}
 if(!renderer.init(w,h,format))return;
 glBindFramebuffer(GL_READ_FRAMEBUFFER,framebuffer);GLint readBuffer=0;glGetIntegerv(GL_READ_BUFFER,&readBuffer);glReadBuffer(framebuffer?GL_COLOR_ATTACHMENT0:GL_BACK);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,renderer.fbo);
 glBlitFramebuffer(0,0,w,h,0,0,w,h,GL_COLOR_BUFFER_BIT,GL_NEAREST);bool copied=glGetError()==GL_NO_ERROR,depthCopied=false;
 if(copied&&renderer.depthTexture){glBlitFramebuffer(0,0,w,h,0,0,w,h,GL_DEPTH_BUFFER_BIT,GL_NEAREST);depthCopied=glGetError()==GL_NO_ERROR;if(!depthCopied){renderer.rejectedDepthFormat=renderer.depthFormat;__android_log_print(ANDROID_LOG_WARN,TAG,"Depth copy unavailable; sky masking disabled");}}
 glReadBuffer(readBuffer);if(!copied){if(status.exchange(2)!=2)__android_log_print(ANDROID_LOG_WARN,TAG,"World framebuffer copy failed; effect skipped");return;}
 glBindFramebuffer(GL_DRAW_FRAMEBUFFER,framebuffer);glViewport(0,0,w,h);glUseProgram(renderer.program);glBindVertexArray(renderer.vao);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,renderer.texture);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,depthCopied?renderer.depthTexture:renderer.texture);
 glUniform1i(glGetUniformLocation(renderer.program,"scene"),0);glUniform1i(glGetUniformLocation(renderer.program,"sceneDepth"),1);
 glUniform2f(glGetUniformLocation(renderer.program,"center"),px,py);glUniform3f(glGetUniformLocation(renderer.program,"direction"),dir.x,dir.y,dir.z);glUniform1f(glGetUniformLocation(renderer.program,"aspect"),aspect);
 glUniform1f(glGetUniformLocation(renderer.program,"innerCos"),innerCos);glUniform1f(glGetUniformLocation(renderer.program,"outerCos"),outerCos);glUniform1f(glGetUniformLocation(renderer.program,"strength"),val(3));glUniform1f(glGetUniformLocation(renderer.program,"ambient"),val(9));glUniform3f(glGetUniformLocation(renderer.program,"tint"),val(0),val(1),val(2));glUniform1f(glGetUniformLocation(renderer.program,"depthAvailable"),depthCopied?1.f:0.f);glUniform1f(glGetUniformLocation(renderer.program,"reversedDepth"),clearDepth<.5f?1.f:0.f);glUniform1f(glGetUniformLocation(renderer.program,"coneEnabled"),val(11));glUniform1f(glGetUniformLocation(renderer.program,"coneRadius"),radius);glUniform2f(glGetUniformLocation(renderer.program,"coneOrigin"),val(12),1.f-val(13));glDrawArrays(GL_TRIANGLES,0,3);
 if(glGetError()!=GL_NO_ERROR){__android_log_print(ANDROID_LOG_WARN,TAG,"Draw error; test frame may be incomplete");return;}int state=maskRequested&&!depthCopied?4:3;if(status.exchange(state)!=state)__android_log_print(ANDROID_LOG_INFO,TAG,"Experimental beam active; sky mask %s",!maskRequested?"off":depthCopied?"available":"unavailable");frames.fetch_add(1,std::memory_order_relaxed);
}
void cameraHook(void* a,void* b,void* c){if(originalCamera)originalCamera(a,b,c);if(!running.load()||!light.load()||!a||!c)return;float v[4];std::memcpy(v,static_cast<unsigned char*>(a)+0x28,sizeof(v));Quaternion q{v[0],v[1],v[2],v[3]};double n=dot(q,q);if(!std::isfinite(n)||n<.5||n>1.5||!normalize(q))return;std::lock_guard<std::mutex> lock(cameraMutex);camera=q;cameraTime=Clock::now();cameraValid=true;cameraHits.fetch_add(1,std::memory_order_relaxed);}
struct Found{bool found=false,build=false,valid[2]={};std::uintptr_t base=0;};
std::size_t align4(std::size_t x){return (x+3u)&~std::size_t(3u);}
int inspect(dl_phdr_info* info,std::size_t,void* data){if(!info->dlpi_name||!std::strstr(info->dlpi_name,"libminecraftpe.so"))return 0;auto& out=*static_cast<Found*>(data);out.found=true;out.base=info->dlpi_addr;
 for(ElfW(Half) i=0;i<info->dlpi_phnum;i++){auto& ph=info->dlpi_phdr[i];if(ph.p_type!=PT_NOTE||ph.p_memsz<sizeof(ElfW(Nhdr)))continue;auto* p=reinterpret_cast<const unsigned char*>(out.base+ph.p_vaddr);std::size_t at=0,size=ph.p_memsz;while(at+sizeof(ElfW(Nhdr))<=size){ElfW(Nhdr) hdr{};std::memcpy(&hdr,p+at,sizeof(hdr));at+=sizeof(hdr);auto n=align4(hdr.n_namesz),d=align4(hdr.n_descsz);if(n>size-at||d>size-at-n)break;if(hdr.n_type==3&&hdr.n_namesz==4&&hdr.n_descsz==20&&std::memcmp(p+at,"GNU",4)==0&&std::memcmp(p+at+n,BUILD_ID,20)==0)out.build=true;at+=n+d;}}
 if(!out.build)return 1;
 for(int i=0;i<2;i++){flashlight::probe::Pattern p;if(!flashlight::probe::parsePattern(targets[i].pattern,p))continue;for(ElfW(Half) j=0;j<info->dlpi_phnum;j++){auto& ph=info->dlpi_phdr[j];if(ph.p_type!=PT_LOAD||!(ph.p_flags&PF_R)||!(ph.p_flags&PF_X)||targets[i].rva<ph.p_vaddr)continue;auto off=targets[i].rva-ph.p_vaddr;if(off>ph.p_memsz||p.bytes.size()>ph.p_memsz-off)continue;out.valid[i]=flashlight::probe::countMatches(reinterpret_cast<const std::uint8_t*>(out.base+targets[i].rva),p.bytes.size(),p,1)==1;}}
 return 1;}
void beforeClear(GLbitfield mask){
 if(processing||!running.load(std::memory_order_relaxed)||!light.load(std::memory_order_relaxed)||!worldVisible.load(std::memory_order_relaxed))return;
 clearHits.fetch_add(1,std::memory_order_relaxed);
 if(!(mask&(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)))return;
 EGLContext context=eglGetCurrentContext();EGLSurface surface=eglGetCurrentSurface(EGL_DRAW);EGLDisplay display=eglGetCurrentDisplay();
 if(context==EGL_NO_CONTEXT||surface==EGL_NO_SURFACE||display==EGL_NO_DISPLAY)return;
 GLint major=0,v[4]={},depth=0;EGLint w=0,h=0;glGetIntegerv(GL_MAJOR_VERSION,&major);glGetIntegerv(GL_VIEWPORT,v);
 if(major<3||!eglQuerySurface(display,surface,EGL_WIDTH,&w)||!eglQuerySurface(display,surface,EGL_HEIGHT,&h)||w<64||h<64||v[0]!=0||v[1]!=0||v[2]!=w||v[3]!=h)return;
 if(glIsEnabled(GL_SCISSOR_TEST)){GLint sc[4]={};glGetIntegerv(GL_SCISSOR_BOX,sc);if(sc[0]>0||sc[1]>0||sc[0]+sc[2]<w||sc[1]+sc[3]<h)return;}
 glGetIntegerv(GL_DEPTH_BITS,&depth);if(depth==0)return;
 if(passContext!=context||passSurface!=surface){passTracker.reset();passContext=context;passSurface=surface;}
 GLint framebuffer=0,program=0;glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&framebuffer);glGetIntegerv(GL_CURRENT_PROGRAM,&program);
 if(!passTracker.observe((mask&GL_COLOR_BUFFER_BIT)!=0,(mask&GL_DEPTH_BUFFER_BIT)!=0,framebuffer,program))return;
 candidateHits.fetch_add(1,std::memory_order_relaxed);processing=true;render();processing=false;
}
void clearHook(GLbitfield mask){beforeClear(mask);if(originalClear)originalClear(mask);}
void clearFvHook(GLenum buffer,GLint draw,const GLfloat* v){if(draw==0)beforeClear(buffer==GL_DEPTH?GL_DEPTH_BUFFER_BIT:buffer==GL_COLOR?GL_COLOR_BUFFER_BIT:0);if(originalClearFv)originalClearFv(buffer,draw,v);}
void clearFiHook(GLenum buffer,GLint draw,GLfloat depth,GLint stencil){if(draw==0&&buffer==GL_DEPTH_STENCIL)beforeClear(GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);if(originalClearFi)originalClearFi(buffer,draw,depth,stencil);}
EGLBoolean swapHook(EGLDisplay display,EGLSurface surface){
 if(eglGetCurrentContext()==passContext&&surface==passSurface)passTracker.reset();
 unsigned swaps=swapHits.fetch_add(1,std::memory_order_relaxed)+1;
 if(swaps%300==0&&running.load()&&light.load())__android_log_print(ANDROID_LOG_INFO,TAG,"Graphics observations: hud=%d clears=%u candidates=%u camera=%u frames=%u",worldVisible.load()?1:0,clearHits.load(),candidateHits.load(),cameraHits.load(),frames.load());
 return originalSwap?originalSwap(display,surface):EGL_FALSE;
}
void uninstall(){if(unhook)for(int i=5;i>=0;--i)if(installed[i]){unhook(hookTargets[i],hookDetours[i]);installed[i]=false;}}
void work(){Found f;for(int i=0;i<600&&running.load();i++){dl_iterate_phdr(inspect,&f);if(f.found)break;std::this_thread::sleep_for(std::chrono::milliseconds(500));}if(!running.load())return;
 if(!f.found||!f.build||!f.valid[0]||!f.valid[1]){status.store(-1);__android_log_print(ANDROID_LOG_ERROR,TAG,"Unsupported game build or signatures; no hooks installed");return;}
 hook=reinterpret_cast<Hook>(dlsym(RTLD_DEFAULT,"pl_hook"));unhook=reinterpret_cast<Unhook>(dlsym(RTLD_DEFAULT,"pl_unhook"));if(!hook||!unhook){preloader=dlopen("libpreloader.so",RTLD_NOW|RTLD_NOLOAD);if(preloader){hook=reinterpret_cast<Hook>(dlsym(preloader,"pl_hook"));unhook=reinterpret_cast<Unhook>(dlsym(preloader,"pl_unhook"));}}
 if(!hook||!unhook){status.store(-1);__android_log_print(ANDROID_LOG_ERROR,TAG,"Preloader hook API unavailable");return;}
 glLibrary=dlopen("libGLESv2.so",RTLD_NOW|RTLD_NOLOAD);eglLibrary=dlopen("libEGL.so",RTLD_NOW|RTLD_NOLOAD);
 if(!glLibrary||!eglLibrary){status.store(-1);__android_log_print(ANDROID_LOG_ERROR,TAG,"OpenGL ES libraries not loaded; rendering path unavailable");return;}
 auto add=[&](int i,void* address,void* detour,void** orig){if(!running.load()||!address||hook(address,detour,orig,200)!=0)return false;if(!*orig){unhook(address,detour);return false;}hookTargets[i]=address;hookDetours[i]=detour;installed[i]=true;return true;};
 if(!add(0,reinterpret_cast<void*>(f.base+targets[1].rva),reinterpret_cast<void*>(cameraHook),reinterpret_cast<void**>(&originalCamera))||
    !add(1,dlsym(glLibrary,"glClear"),reinterpret_cast<void*>(clearHook),reinterpret_cast<void**>(&originalClear))||
    !add(2,dlsym(eglLibrary,"eglSwapBuffers"),reinterpret_cast<void*>(swapHook),reinterpret_cast<void**>(&originalSwap))){uninstall();status.store(-1);__android_log_print(ANDROID_LOG_ERROR,TAG,"Required render-thread hook setup failed");return;}
 // Optional ES 3 clear functions are separate entry points on some devices.
 if(!add(3,dlsym(glLibrary,"glClearBufferfv"),reinterpret_cast<void*>(clearFvHook),reinterpret_cast<void**>(&originalClearFv)))__android_log_print(ANDROID_LOG_WARN,TAG,"glClearBufferfv hook unavailable");
 if(!add(4,dlsym(glLibrary,"glClearBufferfi"),reinterpret_cast<void*>(clearFiHook),reinterpret_cast<void**>(&originalClearFi)))__android_log_print(ANDROID_LOG_WARN,TAG,"glClearBufferfi hook unavailable");
 status.store(1);__android_log_print(ANDROID_LOG_INFO,TAG,"Render-thread hooks ready; experimental beam starts OFF");}
}
extern "C" bool FLStart(){std::lock_guard<std::mutex> lock(lifecycle);if(running.exchange(true))return true;if(worker.joinable())worker.join();if(installed[0]&&installed[1]&&installed[2]){status.store(1);return true;}status.store(0);try{worker=std::thread(work);}catch(...){running.store(false);status.store(-1);return false;}return true;}
extern "C" bool FLStop(){std::lock_guard<std::mutex> lock(lifecycle);light.store(false);running.store(false);if(worker.joinable())worker.join();return true;}
extern "C" bool FLUnload(){FLStop();uninstall();if(preloader){dlclose(preloader);preloader=nullptr;}if(glLibrary){dlclose(glLibrary);glLibrary=nullptr;}if(eglLibrary){dlclose(eglLibrary);eglLibrary=nullptr;}return true;}
extern "C" void FLEnable(bool b){if(status.load()>0&&running.load()){light.store(b);if(!b){std::lock_guard<std::mutex> lock(cameraMutex);cameraValid=false;}__android_log_print(ANDROID_LOG_INFO,TAG,"Beam %s",b?"on":"off");}}
extern "C" void FLSetting(int i,float v){if(i>=0&&i<14&&std::isfinite(v))settings[i].store(std::fmax(0,std::fmin(1,v)));}
extern "C" int FLStatus(){return status.load();}
extern "C" unsigned FLFrames(){return frames.load();}

extern "C" void FLWorldVisible(bool b){worldVisible.store(b,std::memory_order_release);}
