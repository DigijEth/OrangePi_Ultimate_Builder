/*
 * builder.h - Main header file for Orange Pi 5 Plus Ultimate Interactive Builder
 * Version: 0.1.0a
 * 
 * This header contains all shared type definitions, constants, and function
 * prototypes used across the different modules.
 */

#ifndef BUILDER_H
#define BUILDER_H

#define _GNU_SOURCE  // For setenv
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/statvfs.h>
#include <ctype.h>
#include <stdarg.h>

// Version and paths
#define VERSION "0.1.0a"
#define BUILD_DIR "/tmp/opi5plus_build"
#define LOG_FILE "/tmp/opi5plus_build.log"
#define ERROR_LOG_FILE "/tmp/opi5plus_build_errors.log"
#define MAX_CMD_LEN 2048
#define MAX_PATH_LEN 512
#define MAX_ERROR_MSG 1024

// GitHub token and environment constants
#define GITHUB_TOKEN_MAX_LEN 255
#define GITHUB_TOKEN_ENV "GITHUB_TOKEN"
#define ENV_FILE ".env"

// Color codes for output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_DIM     "\033[2m"
#define COLOR_UNDERLINE "\033[4m"
#define COLOR_BLINK   "\033[5m"
#define COLOR_REVERSE "\033[7m"
#define COLOR_HIDDEN  "\033[8m"

// Background colors
#define BG_BLACK   "\033[40m"
#define BG_RED     "\033[41m"
#define BG_GREEN   "\033[42m"
#define BG_YELLOW  "\033[43m"
#define BG_BLUE    "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN    "\033[46m"
#define BG_WHITE   "\033[47m"

// Clear screen and cursor movement
#define CLEAR_SCREEN "\033[2J\033[H"
#define MOVE_CURSOR(x,y) printf("\033[%d;%dH", (y), (x))
#define SAVE_CURSOR "\033[s"
#define RESTORE_CURSOR "\033[u"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"

// Log levels
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_CRITICAL = 4
} log_level_t;

// Error codes
typedef enum {
    ERROR_SUCCESS = 0,
    ERROR_PERMISSION_DENIED = 1,
    ERROR_FILE_NOT_FOUND = 2,
    ERROR_NETWORK_FAILURE = 3,
    ERROR_COMPILATION_FAILED = 4,
    ERROR_INSUFFICIENT_SPACE = 5,
    ERROR_DEPENDENCY_MISSING = 6,
    ERROR_GPU_DRIVER_FAILED = 7,
    ERROR_KERNEL_CONFIG_FAILED = 8,
    ERROR_INSTALLATION_FAILED = 9,
    ERROR_USER_CANCELLED = 10,
    ERROR_UNKNOWN = 99
} error_code_t;

// Distribution types
typedef enum {
    DISTRO_DESKTOP = 0,
    DISTRO_SERVER = 1,
    DISTRO_EMULATION = 2,
    DISTRO_MINIMAL = 3,
    DISTRO_CUSTOM = 4
} distro_type_t;

// Emulation platforms
typedef enum {
    EMU_NONE = 0,
    EMU_LIBREELEC = 1,
    EMU_EMULATIONSTATION = 2,
    EMU_RETROPIE = 3,
    EMU_LAKKA = 4,
    EMU_BATOCERA = 5,
    EMU_ALL = 99
} emulation_platform_t;

// Ubuntu release information
typedef struct {
    char version[16];
    char codename[32];
    char full_name[64];
    char kernel_version[16];
    int is_lts;
    int is_supported;
    char git_branch[32];
} ubuntu_release_t;

// Mali driver information
typedef struct {
    char description[128];
    char url[512];
    char filename[64];
    int required;
} mali_driver_t;

// Build configuration
typedef struct {
    // Basic configuration
    char kernel_version[64];
    char build_dir[MAX_PATH_LEN];
    char output_dir[MAX_PATH_LEN];
    char cross_compile[128];
    char arch[16];
    char defconfig[64];
    
    // Ubuntu release
    char ubuntu_release[16];
    char ubuntu_codename[32];
    
    // Distribution type
    distro_type_t distro_type;
    emulation_platform_t emu_platform;
    
    // Build options
    int jobs;
    int verbose;
    int clean_build;
    int continue_on_error;
    log_level_t log_level;
    
    // GPU options
    int install_gpu_blobs;
    int enable_opencl;
    int enable_vulkan;
    
    // Component selection
    int build_kernel;
    int build_rootfs;
    int build_uboot;
    int create_image;
    
    // Image settings
    char image_size[32];
    char hostname[64];
    char username[32];
    char password[32];
} build_config_t;

// Menu state
typedef struct {
    int current_menu;
    int current_selection;
    int menu_depth;
    int menu_stack[10];
} menu_state_t;

// Error context
typedef struct {
    error_code_t code;
    char message[MAX_ERROR_MSG];
    char file[64];
    int line;
    time_t timestamp;
} error_context_t;

// Global variables (defined in builder.c)
extern FILE *log_fp;
extern FILE *error_log_fp;
extern build_config_t *global_config;
extern volatile sig_atomic_t interrupted;
extern menu_state_t menu_state;
extern ubuntu_release_t ubuntu_releases[];
extern mali_driver_t mali_drivers[];

// Logging macros
#define LOG_DEBUG(msg) log_message_detailed(LOG_LEVEL_DEBUG, msg, __FILE__, __LINE__)
#define LOG_INFO(msg) log_message_detailed(LOG_LEVEL_INFO, msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) log_message_detailed(LOG_LEVEL_WARNING, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) log_message_detailed(LOG_LEVEL_ERROR, msg, __FILE__, __LINE__)
#define LOG_CRITICAL(msg) log_message_detailed(LOG_LEVEL_CRITICAL, msg, __FILE__, __LINE__)

// Function prototypes from system.c
void setup_signal_handlers(void);
void cleanup_on_signal(int signal);
void log_message_detailed(log_level_t level, const char *message, const char *file, int line);
void log_error_context(error_context_t *error_ctx);
int execute_command_with_retry(const char *cmd, int show_output, int max_retries);
int execute_command_safe(const char *cmd, int show_output, error_context_t *error_ctx);
int check_root_permissions(void);
int check_dependencies(void);
int check_disk_space(const char *path, long required_mb);
int create_directory_safe(const char *path, error_context_t *error_ctx);
int validate_config(build_config_t *config);
int setup_build_environment(void);
int install_prerequisites(void);
int cleanup_build(build_config_t *config);
ubuntu_release_t* find_ubuntu_release(const char *version_or_codename);
int detect_current_ubuntu_release(build_config_t *config);
char* get_github_token(void);
char* add_github_token_to_url(const char* url);
int create_env_template(void);  // system.c version returns int

// Function prototypes from ui.c
void print_header(void);
void print_legal_notice(void);
void clear_screen(void);
void pause_screen(void);
char* get_user_input(const char *prompt, char *buffer, size_t size);
int get_user_choice(const char *prompt, int min, int max);
void show_main_menu(void);
void show_quick_setup_menu(void);
void show_custom_build_menu(void);
void show_distro_selection_menu(void);
void show_emulation_menu(void);
void show_ubuntu_selection_menu(void);
void show_gpu_options_menu(build_config_t *config);
void show_build_options_menu(void);
void show_advanced_menu(void);
void show_help_menu(void);
void show_build_progress(const char *stage, int percent);
void show_build_summary(build_config_t *config);
int confirm_action(const char *message);

// Function prototypes from kernel.c
int download_kernel_source(build_config_t *config);
int download_ubuntu_rockchip_patches(void);
int configure_kernel(build_config_t *config);
int build_kernel(build_config_t *config);
int install_kernel(build_config_t *config);
int download_uboot_source(build_config_t *config);
int build_uboot(build_config_t *config);
int build_ubuntu_rootfs(build_config_t *config);
int create_system_image(build_config_t *config);
int install_system_packages(build_config_t *config);
int configure_system_services(build_config_t *config);
int install_emulation_packages(build_config_t *config);
int setup_libreelec(build_config_t *config);
int setup_emulationstation(build_config_t *config);
int setup_retropie(build_config_t *config);

// Function prototypes from gpu.c
int download_mali_blobs(build_config_t *config);
int install_mali_drivers(build_config_t *config);
int setup_opencl_support(build_config_t *config);
int setup_vulkan_support(build_config_t *config);
int verify_gpu_installation(void);
int integrate_mali_into_kernel(build_config_t *config);

// Function prototypes from builder.c (main build logic)
int start_interactive_build(build_config_t *config);
int perform_quick_setup(build_config_t *config);
int perform_custom_build(build_config_t *config);
void init_build_config(build_config_t *config);
void process_args(int argc, char *argv[], build_config_t *config);
void create_env_template_builder(void);  // builder.c version
char* get_github_token(void);    // Function is defined in system.c
char* add_github_token_to_url(const char *url);
int ensure_directories_exist(build_config_t *config);

#endif // BUILDER_H