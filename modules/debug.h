/*
 * debug.h - Debugging and Custom Module System for Orange Pi 5 Plus Ultimate Interactive Builder
 * Version: 0.1.0a
 * 
 * This header provides debugging capabilities and a modular system for custom extensions.
 * Can be easily removed for public releases by simply not including this file.
 */

#ifndef DEBUG_H
#define DEBUG_H

// Include the main builder header for types
#ifndef BUILDER_H
#include "builder.h"
#endif

// Compile-time debug control - set to 0 for public releases
#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED 1
#endif

#if DEBUG_ENABLED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

// Debug levels
typedef enum {
    DEBUG_LEVEL_TRACE = 0,
    DEBUG_LEVEL_DEBUG = 1,
    DEBUG_LEVEL_INFO = 2,
    DEBUG_LEVEL_WARN = 3,
    DEBUG_LEVEL_ERROR = 4,
    DEBUG_LEVEL_FATAL = 5
} debug_level_t;

// Debug configuration
typedef struct {
    debug_level_t level;
    int log_to_file;
    int log_to_console;
    int show_timestamps;
    int show_function_names;
    int show_line_numbers;
    int colorize_output;
    char log_file[256];
    FILE *debug_fp;
} debug_config_t;

// Performance profiling
typedef struct {
    char name[64];
    struct timeval start_time;
    struct timeval end_time;
    long duration_us;  // microseconds
    int active;
} debug_timer_t;

// Memory tracking
typedef struct debug_alloc {
    void *ptr;
    size_t size;
    char file[64];
    int line;
    char function[64];
    time_t timestamp;
    struct debug_alloc *next;
} debug_alloc_t;

// Module system structures
typedef enum {
    MODULE_TYPE_UI = 0,
    MODULE_TYPE_BUILD = 1,
    MODULE_TYPE_KERNEL = 2,
    MODULE_TYPE_GPU = 3,
    MODULE_TYPE_SYSTEM = 4,
    MODULE_TYPE_CUSTOM = 5
} module_type_t;

typedef struct custom_module {
    char name[64];
    char version[16];
    char description[256];
    module_type_t type;
    int priority;  // Higher priority = loaded later, can override
    
    // Function pointers for module capabilities
    int (*init_module)(void);
    int (*cleanup_module)(void);
    void (*show_menu)(void);
    int (*handle_menu_choice)(int choice);
    int (*add_build_options)(build_config_t *config);
    int (*execute_build_step)(build_config_t *config);
    char* (*get_help_text)(void);
    
    // Menu integration
    int menu_option_start;  // Starting menu option number
    int menu_option_count;  // Number of menu options this module adds
    char menu_items[10][128];  // Up to 10 menu items per module
    
    struct custom_module *next;
} custom_module_t;

// Global debug state
extern debug_config_t debug_config;
extern debug_timer_t debug_timers[32];
extern debug_alloc_t *debug_allocations;
extern custom_module_t *loaded_modules;
extern int debug_initialized;

// Debug initialization and cleanup
int debug_init(void);
void debug_cleanup(void);
void debug_set_level(debug_level_t level);
void debug_set_output(int console, int file, const char *filename);

// Core debug logging macros
#define DEBUG_TRACE(msg, ...) debug_log(DEBUG_LEVEL_TRACE, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define DEBUG_DEBUG(msg, ...) debug_log(DEBUG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define DEBUG_INFO(msg, ...) debug_log(DEBUG_LEVEL_INFO, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define DEBUG_WARN(msg, ...) debug_log(DEBUG_LEVEL_WARN, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define DEBUG_ERROR(msg, ...) debug_log(DEBUG_LEVEL_ERROR, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define DEBUG_FATAL(msg, ...) debug_log(DEBUG_LEVEL_FATAL, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)

// Function entry/exit tracing
#define DEBUG_ENTER() DEBUG_TRACE("Entering function")
#define DEBUG_EXIT() DEBUG_TRACE("Exiting function")
#define DEBUG_EXIT_INT(val) DEBUG_TRACE("Exiting function with return value: %d", val)
#define DEBUG_EXIT_PTR(ptr) DEBUG_TRACE("Exiting function with return pointer: %p", ptr)

// Variable dumping
#define DEBUG_VAR_INT(var) DEBUG_DEBUG("Variable %s = %d", #var, var)
#define DEBUG_VAR_STR(var) DEBUG_DEBUG("Variable %s = '%s'", #var, var ? var : "(null)")
#define DEBUG_VAR_PTR(var) DEBUG_DEBUG("Variable %s = %p", #var, var)
#define DEBUG_VAR_HEX(var) DEBUG_DEBUG("Variable %s = 0x%x", #var, var)

// Performance profiling macros
#define DEBUG_TIMER_START(name) debug_timer_start(name)
#define DEBUG_TIMER_END(name) debug_timer_end(name)
#define DEBUG_TIMER_REPORT(name) debug_timer_report(name)

// Memory debugging macros
#define DEBUG_MALLOC(size) debug_malloc(size, __FILE__, __LINE__, __func__)
#define DEBUG_CALLOC(count, size) debug_calloc(count, size, __FILE__, __LINE__, __func__)
#define DEBUG_REALLOC(ptr, size) debug_realloc(ptr, size, __FILE__, __LINE__, __func__)
#define DEBUG_FREE(ptr) debug_free(ptr, __FILE__, __LINE__, __func__)
#define DEBUG_MEMORY_REPORT() debug_memory_report()

// Command execution debugging
#define DEBUG_EXEC(cmd) debug_execute_command(cmd, __FILE__, __LINE__, __func__)

// File operation debugging
#define DEBUG_FILE_OPEN(path, mode) debug_file_open(path, mode, __FILE__, __LINE__, __func__)
#define DEBUG_FILE_CLOSE(fp) debug_file_close(fp, __FILE__, __LINE__, __func__)

// Network debugging
#define DEBUG_DOWNLOAD(url, dest) debug_download(url, dest, __FILE__, __LINE__, __func__)

// Configuration debugging
#define DEBUG_CONFIG_DUMP(config) debug_dump_config(config)
#define DEBUG_CONFIG_VALIDATE(config) debug_validate_config(config)

// Module system macros
#define REGISTER_MODULE(mod) register_custom_module(mod)
#define LOAD_MODULES_FROM_DIR(dir) load_modules_from_directory(dir)
#define MODULE_OVERRIDE(func_name, new_func) override_module_function(func_name, new_func)

// Debug menu options
#define DEBUG_MENU_OPTION_START 900  // Start debug menu options at 900+

// Function prototypes
void debug_log(debug_level_t level, const char *file, int line, const char *func, const char *format, ...);
void debug_timer_start(const char *name);
void debug_timer_end(const char *name);
void debug_timer_report(const char *name);
void debug_timer_report_all(void);

void* debug_malloc(size_t size, const char *file, int line, const char *func);
void* debug_calloc(size_t count, size_t size, const char *file, int line, const char *func);
void* debug_realloc(void *ptr, size_t size, const char *file, int line, const char *func);
void debug_free(void *ptr, const char *file, int line, const char *func);
void debug_memory_report(void);
void debug_memory_cleanup(void);

int debug_execute_command(const char *cmd, const char *file, int line, const char *func);
FILE* debug_file_open(const char *path, const char *mode, const char *file, int line, const char *func);
int debug_file_close(FILE *fp, const char *file, int line, const char *func);
int debug_download(const char *url, const char *dest, const char *file, int line, const char *func);

void debug_dump_config(build_config_t *config);
int debug_validate_config(build_config_t *config);

// Debug menu functions
void show_debug_menu(void);
int handle_debug_menu_choice(int choice);
void debug_interactive_shell(void);
void debug_system_info(void);
void debug_build_state(void);

// Module system functions
int register_custom_module(custom_module_t *module);
int load_modules_from_directory(const char *directory);
int unload_module(const char *name);
void list_loaded_modules(void);
custom_module_t* find_module(const char *name);
int module_menu_integration(void);
int override_module_function(const char *func_name, void *new_func);

// Utility functions for debugging specific components
void debug_kernel_state(void);
void debug_gpu_state(void);
void debug_network_state(void);
void debug_filesystem_state(void);
void debug_environment_variables(void);

// Advanced debugging features
void debug_backtrace(void);
void debug_core_dump_info(void);
void debug_signal_handler(int sig);
void debug_enable_core_dumps(void);

// Debug-specific build options
typedef struct {
    int enable_kernel_debug;
    int enable_gpu_debug;
    int enable_network_debug;
    int verbose_commands;
    int save_intermediate_files;
    int enable_profiling;
    char debug_output_dir[256];
} debug_build_options_t;

extern debug_build_options_t debug_build_options;

// Debug build integration
int debug_modify_build_config(build_config_t *config);
int debug_pre_build_hook(build_config_t *config);
int debug_post_build_hook(build_config_t *config);
int debug_build_step_hook(const char *step_name, build_config_t *config);

// Conditional compilation helpers
#define DEBUG_ONLY(code) do { code } while(0)
#define DEBUG_CODE(code) code

#else // DEBUG_ENABLED == 0

// When debugging is disabled, all macros become no-ops
#define DEBUG_TRACE(msg, ...)
#define DEBUG_DEBUG(msg, ...)
#define DEBUG_INFO(msg, ...)
#define DEBUG_WARN(msg, ...)
#define DEBUG_ERROR(msg, ...)
#define DEBUG_FATAL(msg, ...)

#define DEBUG_ENTER()
#define DEBUG_EXIT()
#define DEBUG_EXIT_INT(val)
#define DEBUG_EXIT_PTR(ptr)

#define DEBUG_VAR_INT(var)
#define DEBUG_VAR_STR(var)
#define DEBUG_VAR_PTR(var)
#define DEBUG_VAR_HEX(var)

#define DEBUG_TIMER_START(name)
#define DEBUG_TIMER_END(name)
#define DEBUG_TIMER_REPORT(name)

#define DEBUG_MALLOC(size) malloc(size)
#define DEBUG_CALLOC(count, size) calloc(count, size)
#define DEBUG_REALLOC(ptr, size) realloc(ptr, size)
#define DEBUG_FREE(ptr) free(ptr)
#define DEBUG_MEMORY_REPORT()

#define DEBUG_EXEC(cmd) system(cmd)
#define DEBUG_FILE_OPEN(path, mode) fopen(path, mode)
#define DEBUG_FILE_CLOSE(fp) fclose(fp)
#define DEBUG_DOWNLOAD(url, dest) 0

#define DEBUG_CONFIG_DUMP(config)
#define DEBUG_CONFIG_VALIDATE(config) 0

#define REGISTER_MODULE(mod) 0
#define LOAD_MODULES_FROM_DIR(dir) 0
#define MODULE_OVERRIDE(func_name, new_func) 0

#define DEBUG_ONLY(code)
#define DEBUG_CODE(code)

// Stub functions for when debugging is disabled
static inline int debug_init(void) { return 0; }
static inline void debug_cleanup(void) { }
static inline void show_debug_menu(void) { }
static inline int handle_debug_menu_choice(int choice) { return 0; }
static inline int module_menu_integration(void) { return 0; }
static inline int debug_modify_build_config(build_config_t *config) { return 0; }
static inline int debug_pre_build_hook(build_config_t *config) { return 0; }
static inline int debug_post_build_hook(build_config_t *config) { return 0; }
static inline int debug_build_step_hook(const char *step_name, build_config_t *config) { return 0; }

#endif // DEBUG_ENABLED

#endif // DEBUG_H