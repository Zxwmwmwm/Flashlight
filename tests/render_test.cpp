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
 EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig config;EGLint count;assert(eglChooseConfig(display,attrs,&config,1,&count)&&count==1);
 EGLint pbuf[]={EGL_WIDTH,128,EGL_HEIGHT,128,EGL_NONE};auto surface=eglCreatePbufferSurface(display,config,pbuf);assert(surface!=EGL_NO_SURFACE);EGLint ctxattr[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,ctxattr);assert(context!=EGL_NO_CONTEXT);assert(eglMakeCurrent(display,surface,surface,context));
 glViewport(0,0,128,128);glDisable(GL_SCISSOR_TEST);glClearColor(.25f,.20f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);glEnable(GL_BLEND);glColorMask(GL_TRUE,GL_FALSE,GL_TRUE,GL_TRUE);
 camera={0,0,0,1};cameraValid=true;cameraTime=Clock::now();running=true;light=true;status=1;
 render();std::cout<<"status="<<status.load()<<" frames="<<frames.load()<<"\n";assert(status==3&&frames==1);
 GLboolean masks[4];glGetBooleanv(GL_COLOR_WRITEMASK,masks);assert(masks[0]&&!masks[1]&&masks[2]&&masks[3]);assert(glIsEnabled(GL_BLEND));glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
 unsigned char center[4],edge[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,center);glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,edge);
 std::cout<<"center "<<(int)center[0]<<","<<(int)center[1]<<","<<(int)center[2]<<" edge "<<(int)edge[0]<<","<<(int)edge[1]<<","<<(int)edge[2]<<"\n";
 assert(center[0]>80&&edge[0]<25&&center[0]>edge[0]);
 // A camera turn should move the lagged beam off center; the frame should remain valid.
 glClearColor(.25f,.2f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);camera={0,std::sin(.25),0,std::cos(.25)};cameraTime=Clock::now();renderer.last=cameraTime-std::chrono::milliseconds(16);render();assert(frames==2);
 unsigned char left[4],right[4];glReadPixels(15,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,left);glReadPixels(113,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,right);assert(right[0]>left[0]);
 glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);cameraTime=Clock::now();render();assert(frames==3);unsigned char black[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,black);assert(black[0]==0&&black[1]==0&&black[2]==0);
 light=false;glClearColor(.1f,.1f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);render();assert(frames==3);unsigned char off[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,off);assert(off[0]>=24&&off[0]<=27);
 assert(glGetError()==GL_NO_ERROR);renderer.release();eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
}
