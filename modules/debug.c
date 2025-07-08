/*
 * debug.c - Debugging System Implementation for Orange Pi 5 Plus Ultimate Interactive Builder
 * Version: 0.1.0a
 * 
 * This file implements the debugging and module system functionality.
 * Only compiled when DEBUG_ENABLED is set to 1.
 */

#include "builder.h"
#include "debug.h"

#if DEBUG_ENABLED

#include <execinfo.h>
#include <signal.h>
#include <sys/resource.h>
#include <dlfcn.h>
#include <dirent.h>

// Global debug state
debug_config_t debug_config = {
    .level = DEBUG_LEVEL_DEBUG,
    .log_to_file = 1,
    .log_to_console = 1,
    .show_timestamps = 1,
    .show_function_names = 1,
    .show_line_numbers = 1,
    .colorize_output = 1,
    .log_file = "/tmp/opi5plus_debug.log",
    .debug_fp = NULL
};

debug_timer_t debug_timers[32];
debug_alloc_t *debug_allocations = NULL;
custom_module_t *loaded_modules = NULL;
int debug_initialized = 0;

debug_build_options_t debug_build_options = {
    .enable_kernel_debug = 0,
    .enable_gpu_debug = 0,
    .enable_network_debug = 0,
    .verbose_commands = 1,
    .save_intermediate_files = 1,
    .enable_profiling = 1,
    .debug_output_dir = "/tmp/opi5plus_debug_output"
};

// Debug color codes
static const char* debug_colors[] = {
    "\033[37m",  // TRACE - white
    "\033[36m",  // DEBUG - cyan  
    "\033[32m",  // INFO - green
    "\033[33m",  // WARN - yellow
    "\033[31m",  // ERROR - red
    "\033[35m"   // FATAL - magenta
};

static const char* debug_level_names[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

// Initialize debug system
int debug_init(void) {
    if (debug_initialized) {
        return 0;
    }
    
    // Open debug log file
    if (debug_config.log_to_file) {
        debug_config.debug_fp = fopen(debug_config.log_file, "a");
        if (!debug_config.debug_fp) {
            fprintf(stderr, "Warning: Could not open debug log file: %s\n", debug_config.log_file);
            debug_config.log_to_file = 0;
        }
    }
    
    // Initialize timers
    memset(debug_timers, 0, sizeof(debug_timers));
    
    // Create debug output directory
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", debug_build_options.debug_output_dir);
    system(cmd);
    
    // Enable core dumps if requested
    debug_enable_core_dumps();
    
    // Install signal handler for crashes
    signal(SIGSEGV, debug_signal_handler);
    signal(SIGABRT, debug_signal_handler);
    signal(SIGFPE, debug_signal_handler);
    
    debug_initialized = 1;
    
    DEBUG_INFO("Debug system initialized successfully");
    return 0;
}

// Cleanup debug system
void debug_cleanup(void) {
    if (!debug_initialized) {
        return;
    }
    
    DEBUG_INFO("Cleaning up debug system");
    
    // Report any memory leaks
    debug_memory_report();
    debug_memory_cleanup();
    
    // Report timer summaries
    debug_timer_report_all();
    
    // Close debug log file
    if (debug_config.debug_fp) {
        fclose(debug_config.debug_fp);
        debug_config.debug_fp = NULL;
    }
    
    // Unload all modules
    custom_module_t *current = loaded_modules;
    while (current) {
        custom_module_t *next = current->next;
        if (current->cleanup_module) {
            current->cleanup_module();
        }
        free(current);
        current = next;
    }
    loaded_modules = NULL;
    
    debug_initialized = 0;
}

// Core debug logging function
void debug_log(debug_level_t level, const char *file, int line, const char *func, const char *format, ...) {
    if (level < debug_config.level) {
        return;
    }
    
    char message[2048];
    char timestamp[64];
    char final_message[4096];
    va_list args;
    
    // Format the message
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // Get timestamp
    if (debug_config.show_timestamps) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    }
    
    // Extract filename from path
    const char *filename = strrchr(file, '/');
    filename = filename ? filename + 1 : file;
    
    // Build final message
    int pos = 0;
    if (debug_config.show_timestamps) {
        pos += snprintf(final_message + pos, sizeof(final_message) - pos, "[%s] ", timestamp);
    }
    
    pos += snprintf(final_message + pos, sizeof(final_message) - pos, "[%s] ", debug_level_names[level]);
    
    if (debug_config.show_function_names && debug_config.show_line_numbers) {
        pos += snprintf(final_message + pos, sizeof(final_message) - pos, "%s:%d:%s() ", filename, line, func);
    } else if (debug_config.show_line_numbers) {
        pos += snprintf(final_message + pos, sizeof(final_message) - pos, "%s:%d ", filename, line);
    } else if (debug_config.show_function_names) {
        pos += snprintf(final_message + pos, sizeof(final_message) - pos, "%s() ", func);
    }
    
    snprintf(final_message + pos, sizeof(final_message) - pos, "%s", message);
    
    // Output to console
    if (debug_config.log_to_console) {
        if (debug_config.colorize_output) {
            printf("%s%s%s\n", debug_colors[level], final_message, COLOR_RESET);
        } else {
            printf("%s\n", final_message);
        }
    }
    
    // Output to file
    if (debug_config.log_to_file && debug_config.debug_fp) {
        fprintf(debug_config.debug_fp, "%s\n", final_message);
        fflush(debug_config.debug_fp);
    }
}

// Timer functions
void debug_timer_start(const char *name) {
    for (int i = 0; i < 32; i++) {
        if (!debug_timers[i].active) {
            strncpy(debug_timers[i].name, name, sizeof(debug_timers[i].name) - 1);
            debug_timers[i].name[sizeof(debug_timers[i].name) - 1] = '\0';
            gettimeofday(&debug_timers[i].start_time, NULL);
            debug_timers[i].active = 1;
            DEBUG_TRACE("Started timer: %s", name);
            return;
        }
    }
    DEBUG_WARN("No free timer slots for: %s", name);
}

void debug_timer_end(const char *name) {
    for (int i = 0; i < 32; i++) {
        if (debug_timers[i].active && strcmp(debug_timers[i].name, name) == 0) {
            gettimeofday(&debug_timers[i].end_time, NULL);
            debug_timers[i].duration_us = 
                (debug_timers[i].end_time.tv_sec - debug_timers[i].start_time.tv_sec) * 1000000 +
                (debug_timers[i].end_time.tv_usec - debug_timers[i].start_time.tv_usec);
            debug_timers[i].active = 0;
            DEBUG_TRACE("Ended timer: %s (%ld μs)", name, debug_timers[i].duration_us);
            return;
        }
    }
    DEBUG_WARN("Timer not found: %s", name);
}

void debug_timer_report(const char *name) {
    for (int i = 0; i < 32; i++) {
        if (strcmp(debug_timers[i].name, name) == 0) {
            if (debug_timers[i].duration_us > 1000000) {
                DEBUG_INFO("Timer %s: %.2f seconds", name, debug_timers[i].duration_us / 1000000.0);
            } else if (debug_timers[i].duration_us > 1000) {
                DEBUG_INFO("Timer %s: %.2f milliseconds", name, debug_timers[i].duration_us / 1000.0);
            } else {
                DEBUG_INFO("Timer %s: %ld microseconds", name, debug_timers[i].duration_us);
            }
            return;
        }
    }
    DEBUG_WARN("Timer not found for report: %s", name);
}

void debug_timer_report_all(void) {
    DEBUG_INFO("=== Timer Report ===");
    for (int i = 0; i < 32; i++) {
        if (strlen(debug_timers[i].name) > 0) {
            debug_timer_report(debug_timers[i].name);
        }
    }
    DEBUG_INFO("=== End Timer Report ===");
}

// Memory debugging functions
void* debug_malloc(size_t size, const char *file, int line, const char *func) {
    void *ptr = malloc(size);
    if (ptr) {
        debug_alloc_t *alloc = malloc(sizeof(debug_alloc_t));
        if (alloc) {
            alloc->ptr = ptr;
            alloc->size = size;
            strncpy(alloc->file, file, sizeof(alloc->file) - 1);
            alloc->file[sizeof(alloc->file) - 1] = '\0';
            alloc->line = line;
            strncpy(alloc->function, func, sizeof(alloc->function) - 1);
            alloc->function[sizeof(alloc->function) - 1] = '\0';
            alloc->timestamp = time(NULL);
            alloc->next = debug_allocations;
            debug_allocations = alloc;
            DEBUG_TRACE("Allocated %zu bytes at %p", size, ptr);
        }
    } else {
        DEBUG_ERROR("Failed to allocate %zu bytes", size);
    }
    return ptr;
}

void debug_free(void *ptr, const char *file, int line, const char *func) {
    if (!ptr) {
        DEBUG_WARN("Attempt to free NULL pointer");
        return;
    }
    
    debug_alloc_t *prev = NULL;
    debug_alloc_t *current = debug_allocations;
    
    while (current) {
        if (current->ptr == ptr) {
            if (prev) {
                prev->next = current->next;
            } else {
                debug_allocations = current->next;
            }
            DEBUG_TRACE("Freed %zu bytes at %p", current->size, ptr);
            free(current);
            free(ptr);
            return;
        }
        prev = current;
        current = current->next;
    }
    
    DEBUG_WARN("Attempt to free untracked pointer %p", ptr);
    free(ptr);
}

void debug_memory_report(void) {
    size_t total_leaked = 0;
    int leak_count = 0;
    
    DEBUG_INFO("=== Memory Leak Report ===");
    
    debug_alloc_t *current = debug_allocations;
    while (current) {
        total_leaked += current->size;
        leak_count++;
        DEBUG_WARN("Memory leak: %zu bytes at %p (allocated in %s:%d:%s())", 
                   current->size, current->ptr, current->file, current->line, current->function);
        current = current->next;
    }
    
    if (leak_count == 0) {
        DEBUG_INFO("No memory leaks detected");
    } else {
        DEBUG_ERROR("Total leaked memory: %zu bytes in %d allocations", total_leaked, leak_count);
    }
    
    DEBUG_INFO("=== End Memory Report ===");
}

void debug_memory_cleanup(void) {
    debug_alloc_t *current = debug_allocations;
    while (current) {
        debug_alloc_t *next = current->next;
        free(current->ptr);
        free(current);
        current = next;
    }
    debug_allocations = NULL;
}

// Configuration debugging
void debug_dump_config(build_config_t *config) {
    if (!config) {
        DEBUG_ERROR("Configuration is NULL");
        return;
    }
    
    DEBUG_INFO("=== Build Configuration Dump ===");
    DEBUG_INFO("Kernel version: %s", config->kernel_version);
    DEBUG_INFO("Build directory: %s", config->build_dir);
    DEBUG_INFO("Output directory: %s", config->output_dir);
    DEBUG_INFO("Cross compile: %s", config->cross_compile);
    DEBUG_INFO("Architecture: %s", config->arch);
    DEBUG_INFO("Ubuntu release: %s (%s)", config->ubuntu_release, config->ubuntu_codename);
    DEBUG_INFO("Distribution type: %d", config->distro_type);
    DEBUG_INFO("Emulation platform: %d", config->emu_platform);
    DEBUG_INFO("Jobs: %d", config->jobs);
    DEBUG_INFO("Verbose: %s", config->verbose ? "Yes" : "No");
    DEBUG_INFO("Clean build: %s", config->clean_build ? "Yes" : "No");
    DEBUG_INFO("GPU drivers: %s", config->install_gpu_blobs ? "Yes" : "No");
    DEBUG_INFO("OpenCL: %s", config->enable_opencl ? "Yes" : "No");
    DEBUG_INFO("Vulkan: %s", config->enable_vulkan ? "Yes" : "No");
    DEBUG_INFO("Image size: %s MB", config->image_size);
    DEBUG_INFO("Hostname: %s", config->hostname);
    DEBUG_INFO("Username: %s", config->username);
    DEBUG_INFO("=== End Configuration Dump ===");
}

// Debug menu system
void show_debug_menu(void) {
    clear_screen();
    print_header();
    
    printf("\n%s%sDEBUG MENU%s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("  %s901.%s Memory Report            - Show memory allocation status\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s902.%s Timer Report             - Show performance timers\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s903.%s Configuration Dump       - Dump current build configuration\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s904.%s System Information       - Show system and environment info\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s905.%s Build State              - Show current build state\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s906.%s Module Management        - Load/unload custom modules\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s907.%s Interactive Shell        - Launch debug shell\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s908.%s Debug Configuration      - Configure debug settings\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s909.%s Network Debug            - Network connectivity tests\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s910.%s Kernel Debug             - Kernel compilation debugging\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s911.%s GPU Debug                - GPU driver debugging\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s912.%s Save Debug State         - Save current debug information\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s0.%s   Back                     - Return to main menu\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
}

int handle_debug_menu_choice(int choice) {
    switch (choice) {
        case 901:
            debug_memory_report();
            pause_screen();
            break;
        case 902:
            debug_timer_report_all();
            pause_screen();
            break;
        case 903:
            if (global_config) {
                debug_dump_config(global_config);
            } else {
                DEBUG_ERROR("No global configuration available");
            }
            pause_screen();
            break;
        case 904:
            debug_system_info();
            pause_screen();
            break;
        case 905:
            debug_build_state();
            pause_screen();
            break;
        case 906:
            list_loaded_modules();
            pause_screen();
            break;
        case 907:
            debug_interactive_shell();
            break;
        case 908:
            // Debug configuration menu
            printf("Debug configuration options:\n");
            printf("Current level: %s\n", debug_level_names[debug_config.level]);
            // Add more configuration options
            pause_screen();
            break;
        case 909:
            debug_network_state();
            pause_screen();
            break;
        case 910:
            debug_kernel_state();
            pause_screen();
            break;
        case 911:
            debug_gpu_state();
            pause_screen();
            break;
        case 912:
            // Save debug state to file
            char filename[256];
            snprintf(filename, sizeof(filename), "%s/debug_state_%ld.txt", 
                     debug_build_options.debug_output_dir, time(NULL));
            FILE *fp = fopen(filename, "w");
            if (fp) {
                fprintf(fp, "Debug State Report\n");
                fprintf(fp, "==================\n\n");
                fclose(fp);
                DEBUG_INFO("Debug state saved to: %s", filename);
            }
            pause_screen();
            break;
        case 0:
            return 0;
        default:
            return -1;
    }
    return 1;
}

// System information debugging
void debug_system_info(void) {
    DEBUG_INFO("=== System Information ===");
    
    // CPU information
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        char line[256];
        int core_count = 0;
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "processor", 9) == 0) {
                core_count++;
            } else if (strncmp(line, "model name", 10) == 0) {
                DEBUG_INFO("CPU: %s", line + 13);
                break;
            }
        }
        fclose(fp);
        DEBUG_INFO("CPU cores: %d", core_count);
    }
    
    // Memory information
    fp = fopen("/proc/meminfo", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                DEBUG_INFO("Total RAM: %s", line + 10);
                break;
            }
        }
        fclose(fp);
    }
    
    // Disk space
    system("df -h / | tail -1 > /tmp/debug_df.txt");
    fp = fopen("/tmp/debug_df.txt", "r");
    if (fp) {
        char line[256];
        if (fgets(line, sizeof(line), fp)) {
            DEBUG_INFO("Root filesystem: %s", line);
        }
        fclose(fp);
        unlink("/tmp/debug_df.txt");
    }
    
    // Environment variables
    debug_environment_variables();
}

void debug_environment_variables(void) {
    DEBUG_INFO("=== Environment Variables ===");
    extern char **environ;
    for (int i = 0; environ[i] != NULL; i++) {
        if (strstr(environ[i], "GITHUB") || 
            strstr(environ[i], "BUILD") || 
            strstr(environ[i], "PATH") ||
            strstr(environ[i], "HOME") ||
            strstr(environ[i], "USER")) {
            DEBUG_INFO("ENV: %s", environ[i]);
        }
    }
}

// Module system implementation
int register_custom_module(custom_module_t *module) {
    if (!module) {
        DEBUG_ERROR("Cannot register NULL module");
        return -1;
    }
    
    // Check if module already exists
    if (find_module(module->name)) {
        DEBUG_WARN("Module %s already registered", module->name);
        return -1;
    }
    
    // Allocate new module entry
    custom_module_t *new_module = malloc(sizeof(custom_module_t));
    if (!new_module) {
        DEBUG_ERROR("Failed to allocate memory for module %s", module->name);
        return -1;
    }
    
    *new_module = *module;
    new_module->next = loaded_modules;
    loaded_modules = new_module;
    
    // Initialize the module if it has an init function
    if (new_module->init_module) {
        if (new_module->init_module() != 0) {
            DEBUG_ERROR("Failed to initialize module %s", module->name);
            // Remove from list
            loaded_modules = new_module->next;
            free(new_module);
            return -1;
        }
    }
    
    DEBUG_INFO("Registered module: %s v%s", module->name, module->version);
    return 0;
}

custom_module_t* find_module(const char *name) {
    custom_module_t *current = loaded_modules;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void list_loaded_modules(void) {
    DEBUG_INFO("=== Loaded Modules ===");
    
    custom_module_t *current = loaded_modules;
    int count = 0;
    
    while (current) {
        DEBUG_INFO("Module: %s v%s", current->name, current->version);
        DEBUG_INFO("  Type: %d, Priority: %d", current->type, current->priority);
        DEBUG_INFO("  Description: %s", current->description);
        if (current->menu_option_count > 0) {
            DEBUG_INFO("  Menu options: %d-%d", 
                       current->menu_option_start, 
                       current->menu_option_start + current->menu_option_count - 1);
        }
        current = current->next;
        count++;
    }
    
    if (count == 0) {
        DEBUG_INFO("No modules loaded");
    } else {
        DEBUG_INFO("Total modules loaded: %d", count);
    }
}

// Signal handler for crashes
void debug_signal_handler(int sig) {
    DEBUG_FATAL("Received signal %d", sig);
    debug_backtrace();
    debug_cleanup();
    exit(sig);
}

void debug_backtrace(void) {
    void *buffer[100];
    int nptrs = backtrace(buffer, 100);
    char **strings = backtrace_symbols(buffer, nptrs);
    
    if (strings) {
        DEBUG_FATAL("=== Stack Trace ===");
        for (int i = 0; i < nptrs; i++) {
            DEBUG_FATAL("%s", strings[i]);
        }
        free(strings);
    }
}

void debug_enable_core_dumps(void) {
    struct rlimit rlim;
    rlim.rlim_cur = RLIM_INFINITY;
    rlim.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rlim);
    DEBUG_TRACE("Core dumps enabled");
}

// Debug-specific implementations for component states
void debug_kernel_state(void) {
    DEBUG_INFO("=== Kernel Debug State ===");
    if (global_config) {
        DEBUG_INFO("Kernel version: %s", global_config->kernel_version);
        DEBUG_INFO("Architecture: %s", global_config->arch);
        DEBUG_INFO("Cross compile: %s", global_config->cross_compile);
        
        // Check if kernel source exists
        char kernel_dir[512];
        snprintf(kernel_dir, sizeof(kernel_dir), "%s/linux", global_config->build_dir);
        if (access(kernel_dir, F_OK) == 0) {
            DEBUG_INFO("Kernel source directory exists: %s", kernel_dir);
            
            // Check for .config file
            char config_file[512];
            snprintf(config_file, sizeof(config_file), "%s/.config", kernel_dir);
            if (access(config_file, F_OK) == 0) {
                DEBUG_INFO("Kernel config file exists");
            } else {
                DEBUG_WARN("Kernel config file missing");
            }
        } else {
            DEBUG_WARN("Kernel source directory missing: %s", kernel_dir);
        }
    }
}

void debug_gpu_state(void) {
    DEBUG_INFO("=== GPU Debug State ===");
    
    // Check Mali driver files
    const char *mali_files[] = {
        "/usr/lib/aarch64-linux-gnu/libmali.so.1",
        "/lib/firmware/mali/mali_csffw.bin",
        "/etc/OpenCL/vendors/mali.icd",
        "/usr/share/vulkan/icd.d/mali_icd.aarch64.json"
    };
    
    for (int i = 0; i < 4; i++) {
        if (access(mali_files[i], F_OK) == 0) {
            DEBUG_INFO("Mali file exists: %s", mali_files[i]);
        } else {
            DEBUG_WARN("Mali file missing: %s", mali_files[i]);
        }
    }
    
    // Check Mali installation directory
    if (access("/tmp/mali_install", F_OK) == 0) {
        DEBUG_INFO("Mali installation directory exists");
        system("ls -la /tmp/mali_install");
    } else {
        DEBUG_WARN("Mali installation directory missing");
    }
}

void debug_network_state(void) {
    DEBUG_INFO("=== Network Debug State ===");
    
    // Test basic connectivity
    DEBUG_INFO("Testing network connectivity...");
    if (system("ping -c 1 8.8.8.8 >/dev/null 2>&1") == 0) {
        DEBUG_INFO("Internet connectivity: OK");
    } else {
        DEBUG_WARN("Internet connectivity: FAILED");
    }
    
    // Test GitHub connectivity
    if (system("curl -s --head https://github.com >/dev/null 2>&1") == 0) {
        DEBUG_INFO("GitHub connectivity: OK");
    } else {
        DEBUG_WARN("GitHub connectivity: FAILED");
    }
    
    // Check GitHub token
    char* token = get_github_token();
    if (token && strlen(token) > 0) {
        DEBUG_INFO("GitHub token: Available (length: %zu)", strlen(token));
    } else {
        DEBUG_WARN("GitHub token: Not found");
    }
}

void debug_interactive_shell(void) {
    printf("\n%s%sDEBUG INTERACTIVE SHELL%s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("Type 'help' for available commands, 'exit' to quit\n\n");
    
    char command[256];
    while (1) {
        printf("debug> ");
        fflush(stdout);
        
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        // Remove newline
        command[strcspn(command, "\n")] = 0;
        
        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            break;
        } else if (strcmp(command, "help") == 0) {
            printf("Available commands:\n");
            printf("  memory    - Show memory report\n");
            printf("  timers    - Show timer report\n");
            printf("  config    - Dump configuration\n");
            printf("  modules   - List loaded modules\n");
            printf("  system    - Show system info\n");
            printf("  exec CMD  - Execute shell command\n");
            printf("  exit      - Exit debug shell\n");
        } else if (strcmp(command, "memory") == 0) {
            debug_memory_report();
        } else if (strcmp(command, "timers") == 0) {
            debug_timer_report_all();
        } else if (strcmp(command, "config") == 0) {
            if (global_config) {
                debug_dump_config(global_config);
            } else {
                printf("No configuration available\n");
            }
        } else if (strcmp(command, "modules") == 0) {
            list_loaded_modules();
        } else if (strcmp(command, "system") == 0) {
            debug_system_info();
        } else if (strncmp(command, "exec ", 5) == 0) {
            system(command + 5);
        } else if (strlen(command) > 0) {
            printf("Unknown command: %s\n", command);
        }
    }
}

// Build integration hooks
int debug_modify_build_config(build_config_t *config) {
    if (!config) return -1;
    
    DEBUG_ENTER();
    
    if (debug_build_options.verbose_commands) {
        config->verbose = 1;
    }
    
    DEBUG_EXIT_INT(0);
    return 0;
}

int debug_pre_build_hook(build_config_t *config) {
    DEBUG_ENTER();
    DEBUG_INFO("Starting build process");
    DEBUG_TIMER_START("total_build_time");
    debug_dump_config(config);
    DEBUG_EXIT_INT(0);
    return 0;
}

int debug_post_build_hook(build_config_t *config) {
    DEBUG_ENTER();
    DEBUG_TIMER_END("total_build_time");
    DEBUG_TIMER_REPORT("total_build_time");
    DEBUG_INFO("Build process completed");
    DEBUG_EXIT_INT(0);
    return 0;
}

int debug_build_step_hook(const char *step_name, build_config_t *config) {
    DEBUG_INFO("Build step: %s", step_name);
    char timer_name[128];
    snprintf(timer_name, sizeof(timer_name), "step_%s", step_name);
    DEBUG_TIMER_START(timer_name);
    return 0;
}

#endif // DEBUG_ENABLED