#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <wchar.h>
#include <wctype.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <setjmp.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dlfcn.h> // For dynamic linking
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "config.h"
#include "so_util.h" // Replace with Linux-compatible dynamic linking
#include "util.h"

extern uintptr_t __cxa_atexit;
extern uintptr_t __stack_chk_fail;

static char *__ctype_ = (char *)&__ctype_;

// Fake FILE array for stdout/stderr
static uint8_t fake_sF[3][0x100]; // stdout, stderr, stdin

static uint64_t __stack_chk_guard_fake = 0x4242424242424242;

FILE *stderr_fake = (FILE *)0x1337;


void __assert2(const char *file, int line, const char *func, const char *expr) {
    debugPrintf("assertion failed:\n%s:%d (%s): %s\n", file, line, func, expr);
    assert(0);
}

int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
#ifdef DEBUG_LOG
    va_list list;
    static char string[0x1000];

    va_start(list, fmt);
    vsnprintf(string, sizeof(string), fmt, list);
    va_end(list);

    debugPrintf("%s: %s\n", tag, string);
#endif
    return 0;
}

int fake_fprintf(FILE *stream, const char *fmt, ...) {
    int ret = 0;
#ifdef DEBUG_LOG
    va_list list;
    static char string[0x1000];

    va_start(list, fmt);
    ret = vsnprintf(string, sizeof(string), fmt, list);
    va_end(list);

    debugPrintf("%s", string);
#endif
    return ret;
}

// pthread wrappers for Linux
int pthread_mutex_init_fake(pthread_mutex_t **uid, const int *mutexattr) {
    pthread_mutex_t *m = calloc(1, sizeof(pthread_mutex_t));
    if (!m) return -1;

    const int recursive = (mutexattr && *mutexattr == 1);
    pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	if (recursive) {
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	}
	pthread_mutex_init(m, &attr);
	pthread_mutexattr_destroy(&attr);

    int ret = pthread_mutex_init(m, NULL);
    if (ret < 0) {
        free(m);
        return -1;
    }

    *uid = m;
    return 0;
}

int pthread_mutex_destroy_fake(pthread_mutex_t **uid) {
    if (uid && *uid && (uintptr_t)*uid > 0x8000) {
        pthread_mutex_destroy(*uid);
        free(*uid);
        *uid = NULL;
    }
    return 0;
}

int pthread_mutex_lock_fake(pthread_mutex_t **uid) {
    int ret = 0;
    if (!*uid) {
        ret = pthread_mutex_init_fake(uid, NULL);
    } else if ((uintptr_t)*uid == 0x4000) {
        int attr = 1; // recursive
        ret = pthread_mutex_init_fake(uid, &attr);
    }
    if (ret < 0) return ret;
    return pthread_mutex_lock(*uid);
}

int pthread_mutex_unlock_fake(pthread_mutex_t **uid) {
    int ret = 0;
    if (!*uid) {
        ret = pthread_mutex_init_fake(uid, NULL);
    } else if ((uintptr_t)*uid == 0x4000) {
        int attr = 1; // recursive
        ret = pthread_mutex_init_fake(uid, &attr);
    }
    if (ret < 0) return ret;
    return pthread_mutex_unlock(*uid);
}

// GL hooks
void glGetShaderInfoLogHook(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog) {
    glGetShaderInfoLog(shader, maxLength, length, infoLog);
    debugPrintf("shader info log:\n%s\n", infoLog);
}

void glCompressedTexImage2DHook(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
    // don't upload mips
    if (level == 0)
        glCompressedTexImage2D(target, level, format, width, height, border, imageSize, data);
}

void glTexParameteriHook(GLenum target, GLenum param, GLint val) {
    // force trilinear filtering instead of bilinear+nearest mipmap
    if (val == GL_LINEAR_MIPMAP_NEAREST)
        val = GL_LINEAR_MIPMAP_LINEAR;
    glTexParameteri(target, param, val);
}

// Import table
DynLibFunction dynlib_functions[] = {
    { "__sF", (uintptr_t)&fake_sF },
    { "__cxa_atexit", (uintptr_t)&__cxa_atexit },
    { "stderr", (uintptr_t)&stderr_fake },
    { "__android_log_print", (uintptr_t)__android_log_print },
    { "__stack_chk_fail", (uintptr_t)&__stack_chk_fail },
    { "__stack_chk_guard", (uintptr_t)&__stack_chk_guard_fake },
    { "_ctype_", (uintptr_t)&__ctype_ },

  { "AAssetManager_open", (uintptr_t)&ret0 },
  { "AAssetManager_fromJava", (uintptr_t)&ret0 },
  { "AAsset_close", (uintptr_t)&ret0 },
  { "AAsset_getLength", (uintptr_t)&ret0 },
  { "AAsset_getRemainingLength", (uintptr_t)&ret0 },
  { "AAsset_read", (uintptr_t)&ret0 },
  { "AAsset_seek", (uintptr_t)&ret0 },

  // Not sure how important this is. Used in some init_array.
  { "pthread_key_create", (uintptr_t)&ret0 },
  { "pthread_key_delete", (uintptr_t)&ret0 },

  { "pthread_getspecific", (uintptr_t)&ret0 },
  { "pthread_setspecific", (uintptr_t)&ret0 },

  { "pthread_join", (uintptr_t)&pthread_join },
  { "pthread_self", (uintptr_t)&pthread_self },

  { "pthread_setschedparam", (uintptr_t)&ret0 },

  { "pthread_mutexattr_init", (uintptr_t)&ret0 },
  { "pthread_mutexattr_settype", (uintptr_t)&ret0 },
  { "pthread_mutexattr_destroy", (uintptr_t)&ret0 },
  { "pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy_fake },
  { "pthread_mutex_init", (uintptr_t)&pthread_mutex_init_fake },
  { "pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock_fake },
  { "pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock_fake },

  { "sched_get_priority_min", (uintptr_t)&retm1 },

  { "__android_log_print", (uintptr_t)__android_log_print },

  { "__errno_location", (uintptr_t)&__errno_location },

  { "__stack_chk_fail", (uintptr_t)&__stack_chk_fail },
  // freezes with real __stack_chk_guard
  { "__stack_chk_guard", (uintptr_t)&__stack_chk_guard_fake },

  { "_ctype_", (uintptr_t)&__ctype_ },

   // TODO: use math neon?
  { "acos", (uintptr_t)&acos },
  { "acosf", (uintptr_t)&acosf },
  { "asinf", (uintptr_t)&asinf },
  { "atan2f", (uintptr_t)&atan2f },
  { "atanf", (uintptr_t)&atanf },
  { "cos", (uintptr_t)&cos },
  { "cosf", (uintptr_t)&cosf },
  { "exp", (uintptr_t)&exp },
  { "floor", (uintptr_t)&floor },
  { "floorf", (uintptr_t)&floorf },
  { "fmod", (uintptr_t)&fmod },
  { "fmodf", (uintptr_t)&fmodf },
  { "log", (uintptr_t)&log },
  { "log10f", (uintptr_t)&log10f },
  { "pow", (uintptr_t)&pow },
  { "powf", (uintptr_t)&powf },
  { "sin", (uintptr_t)&sin },
  { "sinf", (uintptr_t)&sinf },
  { "tan", (uintptr_t)&tan },
  { "tanf", (uintptr_t)&tanf },
  { "sqrt", (uintptr_t)&sqrt },
  { "sqrtf", (uintptr_t)&sqrtf },

  { "atoi", (uintptr_t)&atoi },
  { "atof", (uintptr_t)&atof },
  { "isspace", (uintptr_t)&isspace },
  { "tolower", (uintptr_t)&tolower },
  { "towlower", (uintptr_t)&towlower },
  { "toupper", (uintptr_t)&toupper },
  { "towupper", (uintptr_t)&towupper },

  { "calloc", (uintptr_t)&calloc },
  { "free", (uintptr_t)&free },
  { "malloc", (uintptr_t)&malloc },
  { "realloc", (uintptr_t)&realloc },

  { "clock_gettime", (uintptr_t)&clock_gettime },
  { "gettimeofday", (uintptr_t)&gettimeofday },
  { "time", (uintptr_t)&time },
  { "asctime", (uintptr_t)&asctime },
  { "localtime", (uintptr_t)&localtime },
  { "localtime_r", (uintptr_t)&localtime_r },
  { "strftime", (uintptr_t)&strftime },

  { "eglGetProcAddress", (uintptr_t)&eglGetProcAddress },
  { "eglGetDisplay", (uintptr_t)&eglGetDisplay },
  { "eglQueryString", (uintptr_t)&eglQueryString },

  { "abort", (uintptr_t)&abort },
  { "exit", (uintptr_t)&exit },

  { "fopen", (uintptr_t)&fopen },
  { "fclose", (uintptr_t)&fclose },
  { "fdopen", (uintptr_t)&fdopen },
  { "fflush", (uintptr_t)&fflush },
  { "fgetc", (uintptr_t)&fgetc },
  { "fgets", (uintptr_t)&fgets },
  { "fputs", (uintptr_t)&fputs },
  { "fputc", (uintptr_t)&fputc },
  { "fprintf", (uintptr_t)&fprintf },
  { "fread", (uintptr_t)&fread },
  { "fseek", (uintptr_t)&fseek },
  { "ftell", (uintptr_t)&ftell },
  { "fwrite", (uintptr_t)&fwrite },
  { "fstat", (uintptr_t)&fstat },
  { "ferror", (uintptr_t)&ferror },
  { "feof", (uintptr_t)&feof },
  { "setvbuf", (uintptr_t)&setvbuf },

  { "getenv", (uintptr_t)&getenv },

  { "glActiveTexture", (uintptr_t)&glActiveTexture },
  { "glAttachShader", (uintptr_t)&glAttachShader },
  { "glBindAttribLocation", (uintptr_t)&glBindAttribLocation },
  { "glBindBuffer", (uintptr_t)&glBindBuffer },
  { "glBindFramebuffer", (uintptr_t)&glBindFramebuffer },
  { "glBindRenderbuffer", (uintptr_t)&glBindRenderbuffer },
  { "glBindTexture", (uintptr_t)&glBindTexture },
  { "glBlendFunc", (uintptr_t)&glBlendFunc },
  { "glBlendFuncSeparate", (uintptr_t)&glBlendFuncSeparate },
  { "glBufferData", (uintptr_t)&glBufferData },
  { "glCheckFramebufferStatus", (uintptr_t)&glCheckFramebufferStatus },
  { "glClear", (uintptr_t)&glClear },
  { "glClearColor", (uintptr_t)&glClearColor },
  { "glClearDepthf", (uintptr_t)&glClearDepthf },
  { "glClearStencil", (uintptr_t)&glClearStencil },
  { "glCompileShader", (uintptr_t)&glCompileShader },
  { "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D },
  { "glCreateProgram", (uintptr_t)&glCreateProgram },
  { "glCreateShader", (uintptr_t)&glCreateShader },
  { "glCullFace", (uintptr_t)&glCullFace },
  { "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
  { "glDeleteFramebuffers", (uintptr_t)&glDeleteFramebuffers },
  { "glDeleteProgram", (uintptr_t)&glDeleteProgram },
  { "glDeleteRenderbuffers", (uintptr_t)&glDeleteRenderbuffers },
  { "glDeleteShader", (uintptr_t)&glDeleteShader },
  { "glDeleteTextures", (uintptr_t)&glDeleteTextures },
  { "glDepthFunc", (uintptr_t)&glDepthFunc },
  { "glDepthMask", (uintptr_t)&glDepthMask },
  { "glDepthRangef", (uintptr_t)&glDepthRangef },
  { "glDisable", (uintptr_t)&glDisable },
  { "glDisableVertexAttribArray", (uintptr_t)&glDisableVertexAttribArray },
  { "glDrawArrays", (uintptr_t)&glDrawArrays },
  { "glDrawElements", (uintptr_t)&glDrawElements },
  { "glEnable", (uintptr_t)&glEnable },
  { "glEnableVertexAttribArray", (uintptr_t)&glEnableVertexAttribArray },
  { "glFinish", (uintptr_t)&glFinish },
  { "glFramebufferRenderbuffer", (uintptr_t)&glFramebufferRenderbuffer },
  { "glFramebufferTexture2D", (uintptr_t)&glFramebufferTexture2D },
  { "glFrontFace", (uintptr_t)&glFrontFace },
  { "glGenBuffers", (uintptr_t)&glGenBuffers },
  { "glGenFramebuffers", (uintptr_t)&glGenFramebuffers },
  { "glGenRenderbuffers", (uintptr_t)&glGenRenderbuffers },
  { "glGenTextures", (uintptr_t)&glGenTextures },
  { "glGetAttribLocation", (uintptr_t)&glGetAttribLocation },
  { "glGetError", (uintptr_t)&glGetError },
  { "glGetBooleanv", (uintptr_t)&glGetBooleanv },
  { "glGetIntegerv", (uintptr_t)&glGetIntegerv },
  { "glGetProgramInfoLog", (uintptr_t)&glGetProgramInfoLog },
  { "glGetProgramiv", (uintptr_t)&glGetProgramiv },
  { "glGetShaderInfoLog", (uintptr_t)&glGetShaderInfoLogHook },
  { "glGetShaderiv", (uintptr_t)&glGetShaderiv },
  { "glGetString", (uintptr_t)&glGetString },
  { "glGetUniformLocation", (uintptr_t)&glGetUniformLocation },
  { "glHint", (uintptr_t)&glHint },
  { "glLinkProgram", (uintptr_t)&glLinkProgram },
  { "glPolygonOffset", (uintptr_t)&glPolygonOffset },
  { "glReadPixels", (uintptr_t)&glReadPixels },
  { "glRenderbufferStorage", (uintptr_t)&glRenderbufferStorage },
  { "glScissor", (uintptr_t)&glScissor },
  { "glShaderSource", (uintptr_t)&glShaderSource },
  { "glTexImage2D", (uintptr_t)&glTexImage2D },
  { "glTexParameterf", (uintptr_t)&glTexParameterf },
  { "glTexParameteri", (uintptr_t)&glTexParameteri },
  { "glUniform1f", (uintptr_t)&glUniform1f },
  { "glUniform1fv", (uintptr_t)&glUniform1fv },
  { "glUniform1i", (uintptr_t)&glUniform1i },
  { "glUniform2fv", (uintptr_t)&glUniform2fv },
  { "glUniform3f", (uintptr_t)&glUniform3f },
  { "glUniform3fv", (uintptr_t)&glUniform3fv },
  { "glUniform4fv", (uintptr_t)&glUniform4fv },
  { "glUniformMatrix3fv", (uintptr_t)&glUniformMatrix3fv },
  { "glUniformMatrix4fv", (uintptr_t)&glUniformMatrix4fv },
  { "glUseProgram", (uintptr_t)&glUseProgram },
  { "glVertexAttrib4fv", (uintptr_t)&glVertexAttrib4fv },
  { "glVertexAttribPointer", (uintptr_t)&glVertexAttribPointer },
  { "glViewport", (uintptr_t)&glViewport },

  // this only uses setjmp in the JPEG loader but not longjmp
  // probably doesn't matter if they're compatible or not
  { "setjmp", (uintptr_t)&setjmp },

  { "memcmp", (uintptr_t)&memcmp },
  { "wmemcmp", (uintptr_t)&wmemcmp },
  { "memcpy", (uintptr_t)&memcpy },
  { "memmove", (uintptr_t)&memmove },
  { "memset", (uintptr_t)&memset },
  { "memchr", (uintptr_t)&memchr },

  { "printf", (uintptr_t)&debugPrintf },

  { "bsearch", (uintptr_t)&bsearch },
  { "qsort", (uintptr_t)&qsort },

  { "snprintf", (uintptr_t)&snprintf },
  { "sprintf", (uintptr_t)&sprintf },
  { "vsnprintf", (uintptr_t)&vsnprintf },
  { "vsprintf", (uintptr_t)&vsprintf },

  { "sscanf", (uintptr_t)&sscanf },

  { "close", (uintptr_t)&close },
  { "lseek", (uintptr_t)&lseek },
  { "mkdir", (uintptr_t)&mkdir },
  { "open", (uintptr_t)&open },
  { "read", (uintptr_t)&read },
  { "stat", (uintptr_t)stat },
  { "write", (uintptr_t)&write },

  { "strcasecmp", (uintptr_t)&strcasecmp },
  { "strcat", (uintptr_t)&strcat },
  { "strchr", (uintptr_t)&strchr },
  { "strcmp", (uintptr_t)&strcmp },
  { "strcoll", (uintptr_t)&strcoll },
  { "strcpy", (uintptr_t)&strcpy },
  { "stpcpy", (uintptr_t)&stpcpy },
  { "strerror", (uintptr_t)&strerror },
  { "strlen", (uintptr_t)&strlen },
  { "strncasecmp", (uintptr_t)&strncasecmp },
  { "strncat", (uintptr_t)&strncat },
  { "strncmp", (uintptr_t)&strncmp },
  { "strncpy", (uintptr_t)&strncpy },
  { "strpbrk", (uintptr_t)&strpbrk },
  { "strrchr", (uintptr_t)&strrchr },
  { "strstr", (uintptr_t)&strstr },
  { "strtod", (uintptr_t)&strtod },
  { "strtok", (uintptr_t)&strtok },
  { "strtol", (uintptr_t)&strtol },
  { "strtoul", (uintptr_t)&strtoul },
  { "strtof", (uintptr_t)&strtof },
  { "strxfrm", (uintptr_t)&strxfrm },

  { "srand", (uintptr_t)&srand },
  { "rand", (uintptr_t)&rand },

  { "nanosleep", (uintptr_t)&nanosleep },
  { "usleep", (uintptr_t)&usleep },

  { "wctob", (uintptr_t)&wctob },
  { "wctype", (uintptr_t)&wctype },
  { "wcsxfrm", (uintptr_t)&wcsxfrm },
  { "iswctype", (uintptr_t)&iswctype },
  { "wcscoll", (uintptr_t)&wcscoll },
  { "wcsftime", (uintptr_t)&wcsftime },
  { "mbrtowc", (uintptr_t)&mbrtowc },
  { "wcrtomb", (uintptr_t)&wcrtomb },
  { "wcslen", (uintptr_t)&wcslen },
  { "btowc", (uintptr_t)&btowc },
};

size_t dynlib_numfunctions = sizeof(dynlib_functions) / sizeof(*dynlib_functions);

void update_imports(void) {
    // Apply hooks based on config
    if (config.disable_mipmaps)
        so_find_import(dynlib_functions, dynlib_numfunctions, "glCompressedTexImage2D")->func = (uintptr_t)glCompressedTexImage2DHook;
    if (config.trilinear_filter)
        so_find_import(dynlib_functions, dynlib_numfunctions, "glTexParameteri")->func = (uintptr_t)glTexParameteriHook;
}
