#ifndef __SO_UTIL_H__
#define __SO_UTIL_H__

#include <stdint.h>

#define ALIGN_MEM(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

// Structure to hold symbol-function mappings
typedef struct {
    const char *symbol; // Symbol name
    uintptr_t func;     // Function address
} DynLibFunction;

// Base addresses and sizes for text and data segments
extern void *text_base, *data_base;
extern size_t text_size, data_size;

// Function prototypes

// Hooks for different architectures
void hook_thumb(uintptr_t addr, uintptr_t dst);  // Thumb mode hook
void hook_arm(uintptr_t addr, uintptr_t dst);    // ARM mode hook
void hook_arm64(uintptr_t addr, uintptr_t dst);  // ARM64 mode hook

// Shared library utilities
void so_flush_caches(void); // Flush CPU caches
void so_free_temp(void);    // Free temporary memory
int so_load(const char *filename, void *base, size_t max_size); // Load a shared library
int so_relocate(void);      // Relocate the shared library
int so_resolve(DynLibFunction *funcs, int num_funcs, int taint_missing_imports); // Resolve symbols
void so_execute_init_array(void); // Execute .init_array section
uintptr_t so_find_addr(const char *symbol); // Find the address of a symbol
uintptr_t so_find_addr_rx(const char *symbol); // Find the address of a symbol in RX memory
uintptr_t so_find_rel_addr(const char *symbol); // Find the address of a relocated symbol
DynLibFunction *so_find_import(DynLibFunction *funcs, int num_funcs, const char *name); // Find an import
void so_finalize(void); // Finalize the shared library (e.g., set memory permissions)
int so_unload(void);    // Unload the shared library

#endif // __SO_UTIL_H__