#include "EGL/egl.h"

namespace {
  struct initializer {
    initializer() {
      // Calling eglGetDisplay initializes EGL
      eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }

    ~initializer() {
    }
  };
  static initializer i;
}
