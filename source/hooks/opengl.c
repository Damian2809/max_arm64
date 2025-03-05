#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include "../config.h"
#include "../util.h"
#include "../so_util.h"

static EGLDisplay display = NULL;
static EGLSurface surface = NULL;
static EGLContext context = NULL;

void NVEventEGLSwapBuffers(void) {
    eglSwapBuffers(display, surface);
}

void NVEventEGLMakeCurrent(void) {
    eglMakeCurrent(display, surface, surface, context);
}

void NVEventEGLUnmakeCurrent(void) {
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

int NVEventEGLInit(void) {
    EGLint numConfigs = 0;
    EGLConfig eglConfig;

    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,     8,
        EGL_GREEN_SIZE,   8,
        EGL_BLUE_SIZE,    8,
        EGL_ALPHA_SIZE,   8,
        EGL_DEPTH_SIZE,   24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };

    // Initialize EGL display
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!display) {
        debugPrintf("EGL: Could not connect to display: %08x\n", eglGetError());
        return 0;
    }

    if (!eglInitialize(display, NULL, NULL)) {
        debugPrintf("EGL: Could not initialize: %08x\n", eglGetError());
        return 0;
    }

    if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
        debugPrintf("EGL: Could not set API: %08x\n", eglGetError());
        return 0;
    }

    if (!eglChooseConfig(display, configAttribs, &eglConfig, 1, &numConfigs)) {
        debugPrintf("EGL: No matching config: %08x\n", eglGetError());
        return 0;
    }

    // Create a window surface (use a dummy surface for now)
    surface = eglCreateWindowSurface(display, eglConfig, (EGLNativeWindowType)0, NULL);
    if (!surface) {
        debugPrintf("EGL: Could not create surface: %08x\n", eglGetError());
        return 0;
    }

    context = eglCreateContext(display, eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (!context) {
        debugPrintf("EGL: Could not create context: %08x\n", eglGetError());
        return 0;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        debugPrintf("EGL: Could not make current: %08x\n", eglGetError());
        return 0;
    }

    debugPrintf("GL_EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));

    return 1; // Success
}

void patch_opengl(void) {
    // Patch EGL functions
    hook_arm64(so_find_addr("_Z14NVEventEGLInitv"), (uintptr_t)NVEventEGLInit);
    hook_arm64(so_find_addr("_Z21NVEventEGLMakeCurrentv"), (uintptr_t)NVEventEGLMakeCurrent);
    hook_arm64(so_find_addr("_Z23NVEventEGLUnmakeCurrentv"), (uintptr_t)NVEventEGLUnmakeCurrent);
    hook_arm64(so_find_addr("_Z21NVEventEGLSwapBuffersv"), (uintptr_t)NVEventEGLSwapBuffers);
}

void deinit_opengl(void) {
    if (display) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context) eglDestroyContext(display, context);
        if (surface) eglDestroySurface(display, surface);
        eglTerminate(display);
    }
}