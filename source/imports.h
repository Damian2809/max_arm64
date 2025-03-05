#ifndef __IMPORTS_H__
#define __IMPORTS_H__

#include <stdio.h>
#include <stdlib.h>
#include "so_util.h" // Ensure this is compatible with Linux

// Fake stderr for compatibility
extern FILE *stderr_fake;

// Table of dynamic library functions
extern DynLibFunction dynlib_functions[];

// Number of functions in the table
extern size_t dynlib_numfunctions;

// Function to update imports (e.g., apply hooks)
void update_imports(void);

#endif // __IMPORTS_H__