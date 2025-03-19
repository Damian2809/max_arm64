#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dlfcn.h>

#include "config.h"
#include "util.h"
#include "error.h"
#include "so_util.h"
#include "hooks.h"
#include "imports.h"

static void *heap_so_base = NULL;
static size_t heap_so_limit = 0;

static void check_data(void) {
    const char *files[] = {
        "MaxPayneSoundsv2.msf",
        "x_data.ras",
        "x_english.ras",
        "x_level1.ras",
        "x_level2.ras",
        "x_level3.ras",
        "data",
        "es2",
        "es2/DefaultPixel.txt",
        "",
    };
    struct stat st;
    unsigned int numfiles = (sizeof(files) / sizeof(*files)) - 1;
    if (config.mod_file[0])
        files[numfiles++] = config.mod_file;
    for (unsigned int i = 0; i < numfiles; ++i) {
        if (stat(files[i], &st) < 0) {
            fatal_error("Could not find\n%s.\nCheck your data files.", files[i]);
            break;
        }
    }
}

static void set_screen_size(int w, int h) {
    if (w <= 0 || h <= 0 || w > 1280 || h > 720) {
        screen_width = 1280;
        screen_height = 720;
    } else {
        screen_width = w;
        screen_height = h;
    }
    printf("screen mode: %dx%d\n", screen_width, screen_height);
}

int main(void) {
    if (read_config(CONFIG_NAME) < 0)
        write_config(CONFIG_NAME);

    check_data();

    set_screen_size(config.screen_width, config.screen_height);

    printf("heap size = %u KB\n", MEMORY_MB * 1024);
    printf(" lib base = %p\n", heap_so_base);
    printf("  lib max = %zu KB\n", heap_so_limit / 1024);

    if (so_load(SO_NAME, heap_so_base, heap_so_limit) < 0)
		fatal_error("Could not load\n%s.", SO_NAME);

    mkdir("savegames", 0777);

    update_imports();

    so_relocate();
    so_resolve(dynlib_functions, dynlib_numfunctions, 1);

    patch_openal();
    patch_opengl();
    patch_game();

    stderr_fake = stderr;

    char *storageRootBuffer = dlsym(handle, "StorageRootBuffer");
    uint8_t *isAndroidPaused = dlsym(handle, "IsAndroidPaused");
    uint8_t *useRGBA8 = dlsym(handle, "UseRGBA8");

    if (storageRootBuffer && isAndroidPaused && useRGBA8) {
        strcpy(storageRootBuffer, ".");
        *isAndroidPaused = 0;
        *useRGBA8 = 1;
    } else {
        fatal_error("Could not resolve game-specific variables: %s", dlerror());
    }

    uint32_t (*initGraphics)(void) = dlsym(handle, "_Z12initGraphicsv");
    uint32_t (*ShowJoystick)(int show) = dlsym(handle, "_Z12ShowJoystickb");
    int (*NVEventAppMain)(int argc, char *argv[]) = dlsym(handle, "_Z14NVEventAppMainiPPc");

    so_finalize();
    so_flush_caches();

    so_execute_init_array();

    so_free_temp();

    initGraphics();
    ShowJoystick(0);
    NVEventAppMain(0, NULL);

    return 0;
}
