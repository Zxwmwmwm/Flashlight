#define EGL_NO_PLATFORM_SPECIFIC_TYPES 1
#include <cstdio>
#include <cstdarg>
extern "C" int __android_log_print(int,const char* tag,const char* fmt,...){va_list args;va_start(args,fmt);std::fprintf(stderr,"%s: ",tag);int n=std::vfprintf(stderr,fmt,args);std::fprintf(stderr,"\n");va_end(args);return n;}
#include "../src/experiment/engine.cpp"
#include <cassert>
#include <iostream>
int main(){
 using GetPlatform=EGLDisplay(*)(EGLenum,void*,const EGLint*);auto getPlatform=reinterpret_cast<GetPlatform>(eglGetProcAddress("eglGetPlatformDisplayEXT"));assert(getPlatform);
 auto display=getPlatform(0x31DD,nullptr,nullptr);assert(display!=EGL_NO_DISPLAY);assert(eglInitialize(display,nullptr,nullptr));assert(eglBindAPI(EGL_OPENGL_ES_API));
 EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,16,EGL_NONE};EGLConfig config;EGLint count;assert(eglChooseConfig(display,attrs,&config,1,&count)&&count==1);
 EGLint pbuf[]={EGL_WIDTH,128,EGL_HEIGHT,128,EGL_NONE};auto surface=eglCreatePbufferSurface(display,config,pbuf);assert(surface!=EGL_NO_SURFACE);EGLint ctxattr[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,ctxattr);assert(context!=EGL_NO_CONTEXT);assert(eglMakeCurrent(display,surface,surface,context));
 glViewport(0,0,128,128);glDisable(GL_SCISSOR_TEST);glDepthMask(GL_TRUE);glClearDepthf(.5f);glClear(GL_DEPTH_BUFFER_BIT);glClearDepthf(1.f);glClearColor(.25f,.20f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);glEnable(GL_BLEND);glColorMask(GL_TRUE,GL_FALSE,GL_TRUE,GL_TRUE);
 GLuint savedTextures[2],savedSamplers[2];glGenTextures(2,savedTextures);glGenSamplers(2,savedSamplers);for(int i=0;i<2;i++){glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,savedTextures[i]);glBindSampler(i,savedSamplers[i]);}glActiveTexture(GL_TEXTURE3);
 camera={0,0,0,1};cameraValid=true;cameraTime=Clock::now();running=true;light=true;status=1;
 render();std::cout<<"status="<<status.load()<<" frames="<<frames.load()<<"\n";assert(status==3&&frames==1);
 GLint active=0;glGetIntegerv(GL_ACTIVE_TEXTURE,&active);assert(active==GL_TEXTURE3);for(int i=0;i<2;i++){GLint tex=0,sampler=0;glActiveTexture(GL_TEXTURE0+i);glGetIntegerv(GL_TEXTURE_BINDING_2D,&tex);glGetIntegerv(GL_SAMPLER_BINDING,&sampler);assert(tex==static_cast<GLint>(savedTextures[i])&&sampler==static_cast<GLint>(savedSamplers[i]));}glActiveTexture(GL_TEXTURE3);
 GLboolean masks[4];glGetBooleanv(GL_COLOR_WRITEMASK,masks);assert(masks[0]&&!masks[1]&&masks[2]&&masks[3]);assert(glIsEnabled(GL_BLEND));glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
 unsigned char center[4],edge[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,center);glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,edge);
 std::cout<<"center "<<(int)center[0]<<","<<(int)center[1]<<","<<(int)center[2]<<" edge "<<(int)edge[0]<<","<<(int)edge[1]<<","<<(int)edge[2]<<"\n";
 assert(center[0]>80&&edge[0]<25&&center[0]>edge[0]);
 // A camera turn should move the lagged beam off center; the frame should remain valid.
 glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);camera={0,std::sin(.25),0,std::cos(.25)};cameraTime=Clock::now();renderer.last=cameraTime-std::chrono::milliseconds(16);render();assert(frames==2);
 unsigned char left[4],right[4];glReadPixels(15,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,left);glReadPixels(113,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,right);assert(right[0]>left[0]);
 glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);cameraTime=Clock::now();render();assert(frames==3);unsigned char black[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,black);assert(black[0]==0&&black[1]==0&&black[2]==0);
 light=false;glClearColor(.1f,.1f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);render();assert(frames==3);unsigned char off[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,off);assert(off[0]>=24&&off[0]<=27);
 // Exercise the render-thread boundary heuristic against a real software EGL context.
 light=true;worldVisible=true;camera={0,0,0,1};cameraTime=Clock::now();glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);passTracker.reset();glUseProgram(0);beforeClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);GLuint dummy=makeProgram();assert(dummy);glUseProgram(dummy);beforeClear(GL_DEPTH_BUFFER_BIT);assert(frames==4);beforeClear(GL_DEPTH_BUFFER_BIT);assert(frames==4);
 // Menu/non-world frames and cropped scissors cannot trigger processing.
 passTracker.reset();worldVisible=false;beforeClear(GL_COLOR_BUFFER_BIT);beforeClear(GL_DEPTH_BUFFER_BIT);assert(frames==4);worldVisible=true;glEnable(GL_SCISSOR_TEST);glScissor(0,0,32,32);passTracker.reset();beforeClear(GL_COLOR_BUFFER_BIT);beforeClear(GL_DEPTH_BUFFER_BIT);assert(frames==4);glDisable(GL_SCISSOR_TEST);glUseProgram(0);glDeleteProgram(dummy);
 // With dark-world off, pixels well outside the beam retain their source brightness.
 passTracker.reset();light=true;camera={0,0,0,1};cameraTime=Clock::now();renderer.lag.reset();FLSetting(9,1.0f);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);render();unsigned char brightEdge[4];glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,brightEdge);assert(brightEdge[0]>=63&&brightEdge[0]<=65);
 // Dark-world on uses the saved ambient value.
 FLSetting(9,.25f);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);cameraTime=Clock::now();render();unsigned char darkEdge[4];glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,darkEdge);assert(darkEdge[0]>=15&&darkEdge[0]<=17);
 // Sky depth must not receive a bright spotlight; near surfaces should.
 FLSetting(9,1.0f);camera={0,0,0,1};cameraTime=Clock::now();renderer.lag.reset();glClearDepthf(1.f);glClear(GL_DEPTH_BUFFER_BIT);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);render();
 unsigned char sky[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,sky);assert(status==3);assert(sky[0]>=63&&sky[0]<=65);
 glClearDepthf(.5f);glClear(GL_DEPTH_BUFFER_BIT);glClearDepthf(1.f);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);cameraTime=Clock::now();render();unsigned char surfacePixel[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,surfacePixel);assert(surfacePixel[0]>sky[0]);
 // When camera spins around, the beam passes out of view instead of snapping to center.
 glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);camera={0,1,0,0};cameraTime=Clock::now();renderer.last=cameraTime-std::chrono::milliseconds(16);render();unsigned char turned[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,turned);assert(turned[0]>=63&&turned[0]<=65);
 // Reverse-depth far clear remains unlit as well.
 camera={0,0,0,1};cameraTime=Clock::now();renderer.lag.reset();glClearDepthf(0.f);glClear(GL_DEPTH_BUFFER_BIT);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);render();glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,sky);assert(sky[0]>=63&&sky[0]<=65);
 // Depth copy failures must fall back safely and report the unavailable sky mask.
 assert(renderer.depthFormat!=0);renderer.rejectedDepthFormat=renderer.depthFormat;glClearDepthf(1.f);glClear(GL_DEPTH_BUFFER_BIT);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);cameraTime=Clock::now();render();assert(status==4);glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,sky);assert(sky[0]>65);
 // Turning the mask off deliberately should keep rendering without a warning status.
 FLSetting(10,0.f);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);cameraTime=Clock::now();render();assert(status==3);glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,sky);assert(sky[0]>65);
 // Packed depth/stencil offscreen targets are common on Android.
 GLuint sourceFbo=0,sourceColor=0,sourceDepth=0;glGenFramebuffers(1,&sourceFbo);glBindFramebuffer(GL_FRAMEBUFFER,sourceFbo);glGenTextures(1,&sourceColor);glBindTexture(GL_TEXTURE_2D,sourceColor);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,128,128,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,sourceColor,0);glGenRenderbuffers(1,&sourceDepth);glBindRenderbuffer(GL_RENDERBUFFER,sourceDepth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,128,128);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,sourceDepth);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
 renderer.rejectedDepthFormat=0;FLSetting(10,1.f);camera={0,0,0,1};cameraTime=Clock::now();renderer.lag.reset();glClearDepthf(.5f);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glClearDepthf(1.f);render();assert(status==3);glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,surfacePixel);assert(surfacePixel[0]>65);
 glClearDepthf(1.f);glClear(GL_DEPTH_BUFFER_BIT);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);cameraTime=Clock::now();render();glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,sky);assert(sky[0]>=63&&sky[0]<=65);
 // The optional visible cone must add a light shaft, leave other pixels alone,
 // and disappear again when disabled. It must also honor sky-depth masking.
 camera={0,0,0,1};cameraTime=Clock::now();renderer.lag.reset();FLSetting(9,1.f);FLSetting(10,1.f);FLSetting(11,0.f);glClearDepthf(.5f);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glClearDepthf(1.f);render();unsigned char coneOff[4],outsideOff[4],coneOn[4],outsideOn[4];glReadPixels(92,37,1,1,GL_RGBA,GL_UNSIGNED_BYTE,coneOff);glReadPixels(10,10,1,1,GL_RGBA,GL_UNSIGNED_BYTE,outsideOff);
 FLSetting(11,1.f);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);cameraTime=Clock::now();render();glReadPixels(92,37,1,1,GL_RGBA,GL_UNSIGNED_BYTE,coneOn);glReadPixels(10,10,1,1,GL_RGBA,GL_UNSIGNED_BYTE,outsideOn);assert(coneOn[0]>coneOff[0]+3);assert(outsideOn[0]==outsideOff[0]);
 FLSetting(11,0.f);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);cameraTime=Clock::now();render();glReadPixels(92,37,1,1,GL_RGBA,GL_UNSIGNED_BYTE,coneOn);assert(coneOn[0]==coneOff[0]);
 FLSetting(11,1.f);glClearDepthf(1.f);glClear(GL_DEPTH_BUFFER_BIT);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);cameraTime=Clock::now();render();glReadPixels(92,37,1,1,GL_RGBA,GL_UNSIGNED_BYTE,coneOn);assert(coneOn[0]>=63&&coneOn[0]<=65);
 glClearDepthf(.5f);glClear(GL_DEPTH_BUFFER_BIT);glClearDepthf(1.f);glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);camera={0,1,0,0};cameraTime=Clock::now();renderer.last=cameraTime-std::chrono::milliseconds(16);render();glReadPixels(92,37,1,1,GL_RGBA,GL_UNSIGNED_BYTE,coneOn);assert(coneOn[0]>=63&&coneOn[0]<=65);
 glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteRenderbuffers(1,&sourceDepth);glDeleteTextures(1,&sourceColor);glDeleteFramebuffers(1,&sourceFbo);
 assert(glGetError()==GL_NO_ERROR);renderer.release();eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
}
