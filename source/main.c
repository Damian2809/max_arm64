#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <dlfcn.h> // For dynamic linking

#include "config.h"
#include "util.h"
#include "error.h"
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
        // if this is missing, assets folder hasn't been merged in
        "es2/DefaultPixel.txt",
        // mod file goes here
        "",
    };
    struct stat st;
    unsigned int numfiles = (sizeof(files) / sizeof(*files)) - 1;
    // if mod is enabled, also check for mod file
    if (config.mod_file[0])
        files[numfiles++] = config.mod_file;
    // check if all the required files are present
    for (unsigned int i = 0; i < numfiles; ++i) {
        if (stat(files[i], &st) < 0) {
            fatal_error("Could not find\n%s.\nCheck your data files.", files[i]);
            break;
        }
    }
}

static void set_screen_size(int w, int h) {
    if (w <= 0 || h <= 0 || w > 1280 || h > 720) {
        // auto; pick resolution based on display mode
        screen_width = 1280;
        screen_height = 720;
    } else {
        screen_width = w;
        screen_height = h;
    }
    printf("screen mode: %dx%d\n", screen_width, screen_height);
}

int main(void) {
    // try to read the config file and create one with default values if it's missing
    if (read_config(CONFIG_NAME) < 0)
        write_config(CONFIG_NAME);

    check_data();

    // calculate actual screen size
    set_screen_size(config.screen_width, config.screen_height);

    printf("heap size = %u KB\n", MEMORY_MB * 1024);
    printf(" lib base = %p\n", heap_so_base);
    printf("  lib max = %u KB\n", heap_so_limit / 1024);

    // Load the shared library
    void *handle = dlopen(SO_NAME, RTLD_LAZY);
    if (!handle) {
        fatal_error("Could not load\n%s: %s", SO_NAME, dlerror());
    }

    // won't save without it
    mkdir("savegames", 0777);

    update_imports();

    // Resolve functions
    uint32_t (*initGraphics)(void) = dlsym(handle, "_Z12initGraphicsv");
    uint32_t (*ShowJoystick)(int show) = dlsym(handle, "_Z12ShowJoystickb");
    int (*NVEventAppMain)(int argc, char *argv[]) = dlsym(handle, "_Z14NVEventAppMainiPPc");

    if (!initGraphics || !ShowJoystick || !NVEventAppMain) {
        fatal_error("Could not resolve required functions: %s", dlerror());
    }

    patch_openal();
    patch_opengl();
    patch_game();

    // can't set it in the initializer because it's not constant
    stderr_fake = stderr;

    // Set up game-specific variables
    char *storageRootBuffer = dlsym(handle, "StorageRootBuffer");
    uint8_t *isAndroidPaused = dlsym(handle, "IsAndroidPaused");
    uint8_t *useRGBA8 = dlsym(handle, "UseRGBA8");

    if (storageRootBuffer && isAndroidPaused && useRGBA8) {
        strcpy(storageRootBuffer, ".");
        *isAndroidPaused = 0;
        *useRGBA8 = 1; // RGB565 FBOs suck
    } else {
        fatal_error("Could not resolve game-specific variables: %s", dlerror());
    }

    // Initialize graphics and start the game
    initGraphics();
    ShowJoystick(0);
    NVEventAppMain(0, NULL);

    // Clean up
    dlclose(handle);

    return 0;
}