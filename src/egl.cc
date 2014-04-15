#include "EGL/egl.h"
#include "GLES/gl.h"
#include <SDL.h>
#include <SDL_syswm.h>

/* SDL2 defines this, but it will only confuse v8 */
#undef None
#undef True
#undef False
#include "egl.h"

#include "argchecks.h"

using namespace v8;
using namespace node;

egl::state_t egl::State;
EGLConfig egl::Config;

extern void egl::InitBindings(Handle<Object> target) {
  NODE_SET_METHOD(target, "getError"      , egl::GetError);
  NODE_SET_METHOD(target, "swapBuffers"   , egl::SwapBuffers);
  NODE_SET_METHOD(target, "createPbufferFromClientBuffer",
                          egl::CreatePbufferFromClientBuffer);
  NODE_SET_METHOD(target, "destroySurface", egl::DestroySurface);

  NODE_SET_METHOD(target, "createContext" , egl::CreateContext);
  NODE_SET_METHOD(target, "destroyContext", egl::DestroyContext);
  NODE_SET_METHOD(target, "makeCurrent"   , egl::MakeCurrent);
}

extern void egl::Init() {
  EGLBoolean result;

  static const EGLint attribute_list[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_ALPHA_MASK_SIZE, 8,
    EGL_NONE
  };

  EGLint num_config;

  NativeWindowType windowtype;
  SDL_Window *window;
  SDL_SysWMinfo wminfo;

  State.display = NULL;
  SDL_Init(SDL_INIT_VIDEO);

  window = SDL_CreateWindow("",
			  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			  320, 240, SDL_WINDOW_SHOWN);
  SDL_VERSION(&wminfo.version);
  SDL_GetWindowWMInfo(window, &wminfo);

  switch (wminfo.subsystem) {
#ifdef SDL_VIDEO_DRIVER_WINDOWS
	  case SDL_SYSWM_WINDOWS:
		  windowtype = wminfo.info.win.window;
		  break;
#endif
#ifdef SDL_VIDEO_DRIVER_X11
	  case SDL_SYSWM_X11:
		  State.display = eglGetDisplay(wminfo.info.x11.display);
		  windowtype = wminfo.info.x11.window;
		  break;
#endif
#ifdef SDL_VIDEO_DRIVER_DIRECTFB
	  case SDL_SYSWM_DIRECTFB:
		  windowtype = wminfo.dfb.window;
		  break;
#endif
#ifdef SDL_VIDEO_DRIVER_COCOA
	  case SDL_SYSWM_COCOA:
		  windowtype = wminfo.cocoa.window;
		  break;
#endif
#ifdef SDL_VIDEO_DRIVER_UIKIT
	  case SDL_SYSWM_UIKIT:
		  windowtype = wminfo.uikit.window;
		  break;
#endif
	  default:
		  windowtype = 0;
		  break;
  }

  if (!State.display)
	  State.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  result = eglInitialize(State.display, NULL, NULL);
  assert(EGL_FALSE != result);

  // bind OpenVG API
  eglBindAPI(EGL_OPENVG_API);

  // get an appropriate EGL frame buffer configuration
  result = eglChooseConfig(State.display, attribute_list,
                           &egl::Config, 1, &num_config);
  assert(EGL_FALSE != result);

  // create an EGL rendering context
  State.context =
    eglCreateContext(State.display, egl::Config, EGL_NO_CONTEXT, NULL);
  assert(State.context != EGL_NO_CONTEXT);

  State.surface =
    eglCreateWindowSurface(State.display, egl::Config, windowtype, NULL);
  assert(State.surface != EGL_NO_SURFACE);

  // connect the context to the surface
  result =
    eglMakeCurrent(State.display, State.surface, State.surface, State.context);
  assert(EGL_FALSE != result);

  // preserve color buffer when swapping
  eglSurfaceAttrib(State.display, State.surface,
                   EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED);
}

// Code from https://github.com/ajstarks/openvg/blob/master/oglinit.c doesn't
// seem necessary.
extern void egl::InitOpenGLES() {
  //DAVE - Set up screen ratio
  glViewport(0, 0, (GLsizei) State.screen_width, (GLsizei) State.screen_height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  float ratio = (float)State.screen_width / (float)State.screen_height;
  glFrustumf(-ratio, ratio, -1.0f, 1.0f, 1.0f, 10.0f);
}

extern void egl::Finish() {
  glClear(GL_COLOR_BUFFER_BIT);
  eglSwapBuffers(State.display, State.surface);
  eglMakeCurrent(State.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroySurface(State.display, State.surface);
  eglDestroyContext(State.display, State.context);
  eglTerminate(State.display);
}


V8_METHOD(egl::GetError) {
  HandleScope scope;

  CheckArgs0(getError);

  V8_RETURN(scope.Close(Integer::New(eglGetError())));
}


V8_METHOD(egl::SwapBuffers) {
  HandleScope scope;

  CheckArgs1(swapBuffers, surface, External);

  EGLSurface surface = (EGLSurface) External::Cast(*args[0])->Value();

  EGLBoolean result = eglSwapBuffers(State.display, surface);

  V8_RETURN(scope.Close(Boolean::New(result)));
}

V8_METHOD(egl::CreatePbufferFromClientBuffer) {
  HandleScope scope;

  // According to the spec (sec. 4.2.2 EGL Functions)
  // The buffer is a VGImage: "The VGImage to be targeted is cast to the
  // EGLClientBuffer type and passed as the buffer parameter."
  // So, check for a Number (as VGImages are checked on openvg.cc) and
  // cast to a EGLClientBuffer.

  CheckArgs1(CreatePbufferFromClientBuffer, vgImage, Number);

  EGLClientBuffer buffer =
    reinterpret_cast<EGLClientBuffer>(args[0]->Uint32Value());

  static const EGLint attribute_list[] = {
    EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
    EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
    EGL_MIPMAP_TEXTURE, EGL_FALSE,
    EGL_NONE
  };

  EGLSurface surface =
    eglCreatePbufferFromClientBuffer(State.display,
                                     EGL_OPENVG_IMAGE,
                                     buffer,
                                     egl::Config,
                                     attribute_list);

  V8_RETURN(scope.Close(External::New(surface)));
}

V8_METHOD(egl::DestroySurface) {
  HandleScope scope;

  CheckArgs1(destroySurface, surface, External);

  EGLSurface surface = (EGLSurface) External::Cast(*args[0])->Value();

  EGLBoolean result = eglDestroySurface(State.display, surface);

  V8_RETURN(scope.Close(Boolean::New(result)));
}

V8_METHOD(egl::MakeCurrent) {
  HandleScope scope;

  CheckArgs2(makeCurrent, surface, External, context, External);

  EGLSurface surface = (EGLSurface) External::Cast(*args[0])->Value();
  EGLContext context = (EGLContext) External::Cast(*args[1])->Value();

  // According to EGL 1.4 spec, 3.7.3, for OpenVG contexts, draw and read
  // surfaces must be the same
  EGLBoolean result = eglMakeCurrent(State.display, surface, surface, context);

  V8_RETURN(scope.Close(Boolean::New(result)));
}

V8_METHOD(egl::CreateContext) {
  HandleScope scope;

  // No arg checks

  EGLContext shareContext = args.Length() == 0 ?
    EGL_NO_CONTEXT :
    (EGLContext) External::Cast(*args[0])->Value();

  // According to EGL 1.4 spec, 3.7.3, for OpenVG contexts, draw and read
  // surfaces must be the same
  EGLContext result =
    eglCreateContext(State.display, egl::Config, shareContext, NULL);

  V8_RETURN(scope.Close(External::New(result)));
}

V8_METHOD(egl::DestroyContext) {
  HandleScope scope;

  CheckArgs1(destroyContext, context, External);

  EGLContext context = (EGLContext) External::Cast(*args[0])->Value();

  EGLBoolean result = eglDestroyContext(State.display, context);

  V8_RETURN(scope.Close(Boolean::New(result)));
}
