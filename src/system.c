/*
 * system.c - System utility functions for Orange Pi 5 Plus Ultimate Interactive Builder
 * Version: 0.1.0a
 * 
 * This file contains system-level utilities including command execution,
 * logging, signal handling, and system checks.
 */

#include "builder.h"

// Enhanced logging function
void log_message_detailed(log_level_t level, const char *message, const char *file, int line) {
    const char *level_names[] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
    const char *level_colors[] = {COLOR_RESET, COLOR_CYAN, COLOR_YELLOW, COLOR_RED, COLOR_MAGENTA};
    
    time_t now;
    char *timestamp;
    char short_file[32];
    
    if (global_config && level < global_config->log_level) {
        return;
    }
    
    time(&now);
    timestamp = ctime(&now);
    if (timestamp) {
        timestamp[strlen(timestamp) - 1] = '\0';
    }
    
    const char *basename = strrchr(file, '/');
    strncpy(short_file, basename ? basename + 1 : file, 31);
    short_file[31] = '\0';
    
    printf("[%s%s%s] %s%s:%d%s %s%s%s\n", 
           COLOR_CYAN, timestamp ? timestamp : "Unknown", COLOR_RESET,
           COLOR_BLUE, short_file, line, COLOR_RESET,
           level_colors[level], message, COLOR_RESET);
    
    if (log_fp) {
        fprintf(log_fp, "[%s] [%s] %s:%d %s\n", 
                timestamp ? timestamp : "Unknown", level_names[level], short_file, line, message);
        fflush(log_fp);
    }
    
    if (level >= LOG_LEVEL_ERROR && error_log_fp) {
        fprintf(error_log_fp, "[%s] [%s] %s:%d %s\n", 
                timestamp ? timestamp : "Unknown", level_names[level], short_file, line, message);
        fflush(error_log_fp);
    }
}

// Log error context
void log_error_context(error_context_t *error_ctx) {
    char timestamp_str[64];
    struct tm *tm_info;
    
    if (!error_ctx) return;
    
    tm_info = localtime(&error_ctx->timestamp);
    strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char full_msg[MAX_ERROR_MSG + 128];
    snprintf(full_msg, sizeof(full_msg), "Error %d: %s", error_ctx->code, error_ctx->message);
    
    log_message_detailed(LOG_LEVEL_ERROR, full_msg, error_ctx->file, error_ctx->line);
}

// Read GitHub token from environment or .env file
char* get_github_token(void) {
    static char token[GITHUB_TOKEN_MAX_LEN + 1] = {0};
    
    // First check environment variable
    char* env_token = getenv(GITHUB_TOKEN_ENV);
    if (env_token != NULL && strlen(env_token) > 0) {
        strncpy(token, env_token, GITHUB_TOKEN_MAX_LEN);
        token[GITHUB_TOKEN_MAX_LEN] = '\0';
        return token;
    }
    
    // Check .env file
    FILE* env_file = fopen(ENV_FILE, "r");
    if (env_file != NULL) {
        char line[GITHUB_TOKEN_MAX_LEN + 100] = {0};
        
        while (fgets(line, sizeof(line), env_file) != NULL) {
            // Skip comments and empty lines
            if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
                continue;
            }
            
            // Remove leading whitespace
            char* line_start = line;
            while (*line_start == ' ' || *line_start == '\t') {
                line_start++;
            }
            
            // Check if this line contains GITHUB_TOKEN
            if (strncmp(line_start, GITHUB_TOKEN_ENV, strlen(GITHUB_TOKEN_ENV)) == 0) {
                char* equals = strchr(line_start, '=');
                if (equals != NULL) {
                    char* value_start = equals + 1;
                    
                    // Skip whitespace after =
                    while (*value_start == ' ' || *value_start == '\t') {
                        value_start++;
                    }
                    
                    // Copy the token
                    strncpy(token, value_start, GITHUB_TOKEN_MAX_LEN);
                    token[GITHUB_TOKEN_MAX_LEN] = '\0';
                    
                    // Remove trailing whitespace and newlines
                    size_t len = strlen(token);
                    while (len > 0 && (token[len-1] == '\n' || token[len-1] == '\r' || 
                                       token[len-1] == ' ' || token[len-1] == '\t')) {
                        token[len-1] = '\0';
                        len--;
                    }
                    
                    // Remove quotes if present
                    if (len >= 2) {
                        if ((token[0] == '"' && token[len-1] == '"') || 
                            (token[0] == '\'' && token[len-1] == '\'')) {
                            memmove(token, token + 1, len - 2);
                            token[len - 2] = '\0';
                        }
                    }
                    
                    fclose(env_file);
                    
                    // Validate we got something
                    if (strlen(token) > 0) {
                        return token;
                    }
                }
            }
        }
        fclose(env_file);
    }
    
    return NULL; // No token found
}

// Add GitHub token to URL for authentication
char* add_github_token_to_url(const char* url) {
    static char auth_url[MAX_CMD_LEN] = {0};
    char* token = get_github_token();
    
    // If no token or not a GitHub URL, return original URL
    if (token == NULL || strlen(token) == 0 || strstr(url, "github.com") == NULL) {
        strncpy(auth_url, url, sizeof(auth_url) - 1);
        auth_url[sizeof(auth_url) - 1] = '\0';
        return auth_url;
    }
    
    // For git clone operations, we need to use the token as the username with 'x-oauth-basic' as password
    // or just the token as the username with empty password
    if (strncmp(url, "https://", 8) == 0) {
        // Extract the URL after https://
        const char* url_after_protocol = url + 8;
        
        // Use the token as username (GitHub accepts this format)
        snprintf(auth_url, sizeof(auth_url), "https://%s:x-oauth-basic@%s", token, url_after_protocol);
        return auth_url;
    } else if (strncmp(url, "git@github.com:", 16) == 0) {
        // Convert SSH URL to HTTPS with authentication
        const char* repo_path = url + 15; // Skip "git@github.com:"
        snprintf(auth_url, sizeof(auth_url), "https://%s:x-oauth-basic@github.com/%s", token, repo_path);
        return auth_url;
    }
    
    // Otherwise, return original URL
    strncpy(auth_url, url, sizeof(auth_url) - 1);
    auth_url[sizeof(auth_url) - 1] = '\0';
    return auth_url;
}

// Create a template .env file if it doesn't exist
int create_env_template(void) {
    if (access(ENV_FILE, F_OK) != 0) {
        FILE* env_file = fopen(ENV_FILE, "w");
        if (env_file != NULL) {
            fprintf(env_file, "# Environment variables for Orange Pi 5 Plus Ultimate Interactive Builder\n\n");
            fprintf(env_file, "# GitHub personal access token for authentication\n");
            fprintf(env_file, "# Create one at: https://github.com/settings/tokens\n");
            fprintf(env_file, "# Required scopes: repo, read:packages\n");
            fprintf(env_file, "# GITHUB_TOKEN=your_token_here\n\n");
            fclose(env_file);
            
            // Set permissions
            chmod(ENV_FILE, 0600);
            
            LOG_INFO("Created template .env file. Please edit it to add your GitHub token.");
            return 0;
        } else {
            LOG_WARNING("Failed to create .env template file");
            return -1;
        }
    }
    
    return 0; // File already exists
}

// Signal handlers
void setup_signal_handlers(void) {
    signal(SIGINT, cleanup_on_signal);
    signal(SIGTERM, cleanup_on_signal);
    signal(SIGQUIT, cleanup_on_signal);
}

void cleanup_on_signal(int sig) {
    interrupted = 1;
    LOG_WARNING("Build interrupted by signal, cleaning up...");
    
    printf(SHOW_CURSOR); // Make sure cursor is visible
    
    if (global_config) {
        cleanup_build(global_config);
    }
    
    if (log_fp) {
        fclose(log_fp);
    }
    if (error_log_fp) {
        fclose(error_log_fp);
    }
    
    exit(sig + 128);
}

// Execute command with retry
int execute_command_with_retry(const char *cmd, int show_output, int max_retries) {
    error_context_t error_ctx = {0};
    int attempt;
    
    for (attempt = 1; attempt <= max_retries; attempt++) {
        if (interrupted) {
            LOG_WARNING("Build interrupted, stopping command execution");
            return -1;
        }
        
        int result = execute_command_safe(cmd, show_output, &error_ctx);
        
        if (result == 0) {
            if (attempt > 1) {
                char msg[512];
                snprintf(msg, sizeof(msg), "Command succeeded on attempt %d: %s", attempt, cmd);
                LOG_INFO(msg);
            }
            return 0;
        }
        
        if (attempt < max_retries) {
            char msg[512];
            snprintf(msg, sizeof(msg), "Command failed (attempt %d/%d), retrying: %s", 
                    attempt, max_retries, cmd);
            LOG_WARNING(msg);
            sleep(2);
        }
    }
    
    error_ctx.code = ERROR_UNKNOWN;
    strncpy(error_ctx.message, "Command failed after all retries", MAX_ERROR_MSG - 1);
    log_error_context(&error_ctx);
    return -1;
}

// Safe command execution
int execute_command_safe(const char *cmd, int show_output, error_context_t *error_ctx) {
    char log_cmd[MAX_CMD_LEN + 100];
    int result;
    
    if (!cmd || strlen(cmd) == 0) {
        if (error_ctx) {
            error_ctx->code = ERROR_UNKNOWN;
            strncpy(error_ctx->message, "Empty command provided", MAX_ERROR_MSG - 1);
        }
        return -1;
    }
    
    if (show_output) {
        printf("%s%s%s\n", COLOR_BLUE, cmd, COLOR_RESET);
    }
    
    if (global_config && global_config->verbose) {
        char msg[MAX_CMD_LEN + 50];
        snprintf(msg, sizeof(msg), "Executing: %s", cmd);
        LOG_DEBUG(msg);
    }
    
    if (show_output) {
        snprintf(log_cmd, sizeof(log_cmd), "%s 2>&1 | tee -a %s", cmd, LOG_FILE);
    } else {
        snprintf(log_cmd, sizeof(log_cmd), "%s >> %s 2>&1", cmd, LOG_FILE);
    }
    
    result = system(log_cmd);
    
    if (result != 0) {
        char error_msg[512];
        if (WIFEXITED(result)) {
            snprintf(error_msg, sizeof(error_msg), 
                    "Command exited with code %d: %s", WEXITSTATUS(result), cmd);
            
            // Log the error message
            LOG_ERROR(error_msg);
        } else if (WIFSIGNALED(result)) {
            snprintf(error_msg, sizeof(error_msg), 
                    "Command terminated by signal %d: %s", WTERMSIG(result), cmd);
            
            // Log the error message
            LOG_ERROR(error_msg);
        } else {
            snprintf(error_msg, sizeof(error_msg), 
                    "Command failed with unknown status %d: %s", result, cmd);
            
            // Log the error message
            LOG_ERROR(error_msg);
        }
        
        if (error_ctx) {
            error_ctx->code = ERROR_UNKNOWN;
            strncpy(error_ctx->message, error_msg, MAX_ERROR_MSG - 1);
            error_ctx->message[MAX_ERROR_MSG - 1] = '\0';
        }
        return -1;
    }
    
    return 0;
}

// Check root permissions
int check_root_permissions(void) {
    if (geteuid() != 0) {
        LOG_ERROR("This tool requires root privileges. Please run with sudo.");
        return ERROR_PERMISSION_DENIED;
    }
    LOG_DEBUG("Root permissions verified");
    return ERROR_SUCCESS;
}

// Create directory safely
int create_directory_safe(const char *path, error_context_t *error_ctx) {
    struct stat st = {0};
    
    if (!path || strlen(path) == 0) {
        if (error_ctx) {
            error_ctx->code = ERROR_UNKNOWN;
            strncpy(error_ctx->message, "Empty path provided", MAX_ERROR_MSG - 1);
            error_ctx->message[MAX_ERROR_MSG - 1] = '\0';
        }
        return -1;
    }
    
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0) {
            char error_msg[512];
            snprintf(error_msg, sizeof(error_msg), 
                    "Failed to create directory '%s': %s", path, strerror(errno));
            if (error_ctx) {
                error_ctx->code = ERROR_FILE_NOT_FOUND;
                strncpy(error_ctx->message, error_msg, MAX_ERROR_MSG - 1);
                error_ctx->message[MAX_ERROR_MSG - 1] = '\0';
            }
            return -1;
        }
        
        char msg[256];
        snprintf(msg, sizeof(msg), "Created directory: %s", path);
        LOG_DEBUG(msg);
    } else {
        char msg[256];
        snprintf(msg, sizeof(msg), "Directory already exists: %s", path);
        LOG_DEBUG(msg);
    }
    
    return 0;
}

// Check disk space
int check_disk_space(const char *path, long required_mb) {
    struct statvfs stat;
    
    if (statvfs(path, &stat) != 0) {
        LOG_ERROR("Failed to check disk space");
        return ERROR_UNKNOWN;
    }
    
    long available_mb = (stat.f_bavail * stat.f_frsize) / (1024 * 1024);
    
    if (available_mb < required_mb) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), 
                "Insufficient disk space: %ld MB available, %ld MB required", 
                available_mb, required_mb);
        LOG_ERROR(error_msg);
        return ERROR_INSUFFICIENT_SPACE;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Disk space check passed: %ld MB available", available_mb);
    LOG_DEBUG(msg);
    return ERROR_SUCCESS;
}

// Find Ubuntu release
ubuntu_release_t* find_ubuntu_release(const char *version_or_codename) {
    int i;
    
    if (!version_or_codename) return NULL;
    
    for (i = 0; strlen(ubuntu_releases[i].version) > 0; i++) {
        if (strcmp(ubuntu_releases[i].version, version_or_codename) == 0 ||
            strcmp(ubuntu_releases[i].codename, version_or_codename) == 0) {
            return &ubuntu_releases[i];
        }
    }
    
    return NULL;
}

// Validate configuration
int validate_config(build_config_t *config) {
    if (!config) {
        LOG_ERROR("Configuration is NULL");
        return ERROR_UNKNOWN;
    }
    
    LOG_DEBUG("Validating build configuration...");
    
    // Validate Ubuntu release
    if (strlen(config->ubuntu_release) == 0) {
        LOG_ERROR("No Ubuntu release specified");
        return ERROR_UNKNOWN;
    }
    
    ubuntu_release_t *release = find_ubuntu_release(config->ubuntu_release);
    if (!release) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Invalid Ubuntu release: %s", config->ubuntu_release);
        LOG_ERROR(msg);
        return ERROR_UNKNOWN;
    }
    
    // Validate kernel version
    if (strlen(config->kernel_version) < 3) {
        LOG_ERROR("Invalid kernel version");
        return ERROR_UNKNOWN;
    }
    
    // Validate paths
    if (strlen(config->build_dir) == 0) {
        LOG_ERROR("Build directory not specified");
        return ERROR_FILE_NOT_FOUND;
    }
    
    if (strlen(config->output_dir) == 0) {
        LOG_ERROR("Output directory not specified");
        return ERROR_FILE_NOT_FOUND;
    }
    
    // Validate build options
    if (config->jobs < 1 || config->jobs > 128) {
        LOG_WARNING("Invalid job count, resetting to default");
        config->jobs = sysconf(_SC_NPROCESSORS_ONLN);
        if (config->jobs <= 0) config->jobs = 4;
    }
    
    // Validate image size
    int image_size = atoi(config->image_size);
    if (image_size < 4096) {
        LOG_WARNING("Image size too small, setting to 8192 MB");
        strcpy(config->image_size, "8192");
    }
    
    // GPU options validation
    if (config->enable_opencl && !config->install_gpu_blobs) {
        LOG_WARNING("OpenCL enabled but GPU drivers disabled, enabling GPU drivers");
        config->install_gpu_blobs = 1;
    }
    
    if (config->enable_vulkan && !config->install_gpu_blobs) {
        LOG_WARNING("Vulkan enabled but GPU drivers disabled, enabling GPU drivers");
        config->install_gpu_blobs = 1;
    }
    
    LOG_DEBUG("Configuration validation completed");
    return ERROR_SUCCESS;
}

// Check dependencies
int check_dependencies(void) {
    LOG_DEBUG("Checking system dependencies...");
    
    const char *required_tools[] = {
        "git", "make", "gcc", "wget", "curl", "bc", "debootstrap", 
        "device-tree-compiler", "u-boot-tools", NULL
    };
    
    int missing = 0;
    
    for (int i = 0; required_tools[i] != NULL; i++) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "which %s >/dev/null 2>&1", required_tools[i]);
        if (system(cmd) != 0) {
            char msg[128];
            snprintf(msg, sizeof(msg), "Required tool missing: %s", required_tools[i]);
            LOG_WARNING(msg);
            missing++;
        }
    }
    
    if (missing > 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "%d required tools are missing. They will be installed automatically.", missing);
        LOG_INFO(msg);
    }
    
    LOG_DEBUG("Dependency check completed");
    return ERROR_SUCCESS;
}

// Setup build environment
int setup_build_environment(void) {
    error_context_t error_ctx = {0};
    
    LOG_INFO("Setting up build environment...");
    
    // Configure git to use the GitHub token if available
    char* token = get_github_token();
    if (token != NULL && strlen(token) > 0) {
        LOG_INFO("Configuring git to use GitHub token...");
        
        // Configure git to use the token for GitHub
        char git_config_cmd[MAX_CMD_LEN];
        
        // Set up git credential helper to use the token
        snprintf(git_config_cmd, sizeof(git_config_cmd), 
                 "git config --global credential.helper '!f() { echo \"username=x-access-token\"; echo \"password=%s\"; }; f'", 
                 token);
        execute_command_safe(git_config_cmd, 0, &error_ctx);
        
        // Also set up the token for direct HTTPS URLs
        snprintf(git_config_cmd, sizeof(git_config_cmd),
                 "git config --global url.\"https://x-access-token:%s@github.com/\".insteadOf \"https://github.com/\"",
                 token);
        execute_command_safe(git_config_cmd, 0, &error_ctx);
        
        // And for git protocol URLs
        snprintf(git_config_cmd, sizeof(git_config_cmd),
                 "git config --global url.\"https://x-access-token:%s@github.com/\".insteadOf \"git@github.com:\"",
                 token);
        execute_command_safe(git_config_cmd, 0, &error_ctx);
        
        LOG_INFO("Git configured to use GitHub token for authentication");
    }
    
    // Check if /proc is mounted
    if (access("/proc/self", F_OK) != 0) {
        LOG_WARNING("/proc is not mounted, attempting to mount it...");
        
        // Try to mount /proc
        if (system("mount -t proc /proc /proc") != 0) {
            LOG_ERROR("/proc could not be mounted. This may cause issues.");
            LOG_ERROR("Try running: sudo mount -t proc /proc /proc");
            LOG_ERROR("Or run this tool outside of a chroot/container environment");
            
            // Don't fail completely, but warn the user
            printf("\n%sWARNING:%s /proc is not mounted. Some features may not work correctly.\n", 
                   COLOR_YELLOW, COLOR_RESET);
            printf("To fix: sudo mount -t proc /proc /proc\n\n");
            pause_screen();
        } else {
            LOG_INFO("Successfully mounted /proc");
        }
    }
    
    // Check if other important filesystems are mounted
    if (access("/sys/class", F_OK) != 0) {
        LOG_WARNING("/sys is not mounted, attempting to mount it...");
        system("mount -t sysfs /sys /sys");
    }
    
    if (access("/dev/null", F_OK) != 0) {
        LOG_WARNING("/dev is not properly set up, attempting to fix...");
        system("mount -t devtmpfs /dev /dev");
    }
    
    // Check disk space (15GB minimum)
    int space_result = check_disk_space("/tmp", 15000);
    if (space_result != ERROR_SUCCESS) {
        if (global_config && !global_config->continue_on_error) {
            return space_result;
        }
        LOG_WARNING("Continuing despite insufficient disk space warning");
    }
    
    // Create build directory
    if (create_directory_safe(BUILD_DIR, &error_ctx) != 0) {
        return ERROR_FILE_NOT_FOUND;
    }
    
    // Create output directory
    if (global_config && strlen(global_config->output_dir) > 0) {
        if (create_directory_safe(global_config->output_dir, &error_ctx) != 0) {
            LOG_WARNING("Failed to create output directory");
        }
    }
    
    // Open log files
    log_fp = fopen(LOG_FILE, "a");
    if (!log_fp) {
        LOG_WARNING("Could not open main log file, continuing without file logging");
    } else {
        LOG_DEBUG("Main log file opened successfully");
    }
    
    error_log_fp = fopen(ERROR_LOG_FILE, "a");
    if (!error_log_fp) {
        LOG_WARNING("Could not open error log file");
    } else {
        LOG_DEBUG("Error log file opened successfully");
    }
    
    // Update package lists
    LOG_INFO("Updating package lists...");
    if (execute_command_with_retry("apt update", 1, 3) != 0) {
        error_ctx.code = ERROR_NETWORK_FAILURE;
        strncpy(error_ctx.message, "Failed to update package lists after retries", MAX_ERROR_MSG - 1);
        error_ctx.message[MAX_ERROR_MSG - 1] = '\0';
        log_error_context(&error_ctx);
        if (global_config && !global_config->continue_on_error) {
            return ERROR_NETWORK_FAILURE;
        }
        LOG_WARNING("Continuing despite package update failure");
    }
    
    LOG_INFO("Build environment setup completed successfully");
    return ERROR_SUCCESS;
}

// Install prerequisites
int install_prerequisites(void) {
    LOG_INFO("Installing build prerequisites...");
    
    const char *packages[] = {
        // Basic build tools
        "build-essential",
        "gcc-aarch64-linux-gnu",
        "g++-aarch64-linux-gnu",
        "libncurses-dev",
        "gawk",
        "flex",
        "bison",
        "openssl",
        "libssl-dev",
        "dkms",
        "libelf-dev",
        "libudev-dev",
        "libpci-dev",
        "libiberty-dev",
        "autoconf",
        "llvm",
        // Additional tools
        "git",
        "wget",
        "curl",
        "bc",
        "rsync",
        "kmod",
        "cpio",
        "python3",
        "python3-pip",
        "device-tree-compiler",
        // Ubuntu kernel build dependencies
        "fakeroot",
        "u-boot-tools",
        // Mali GPU and OpenCL/Vulkan support
        "mesa-opencl-icd",
        "vulkan-tools",
        "libvulkan-dev",
        "ocl-icd-opencl-dev",
        "opencl-headers",
        "clinfo",
        // Media and hardware acceleration
        "va-driver-all",
        "vdpau-driver-all",
        "mesa-va-drivers",
        "mesa-vdpau-drivers",
        // Development libraries
        "libegl1-mesa-dev",
        "libgles2-mesa-dev",
        "libgl1-mesa-dev",
        "libdrm-dev",
        "libgbm-dev",
        "libwayland-dev",
        "libx11-dev",
        "meson",
        "ninja-build",
        // For rootfs creation
        "debootstrap",
        "qemu-user-static",
        "parted",
        "dosfstools",
        "e2fsprogs",
        NULL
    };
    
    char cmd[MAX_CMD_LEN * 2];
    int i;
    
    // Install all packages at once
    strcpy(cmd, "DEBIAN_FRONTEND=noninteractive apt install -y");
    for (i = 0; packages[i] != NULL; i++) {
        strcat(cmd, " ");
        strcat(cmd, packages[i]);
    }
    
    if (execute_command_with_retry(cmd, 1, 2) != 0) {
        error_context_t error_ctx = {0};
        error_ctx.code = ERROR_DEPENDENCY_MISSING;
        strncpy(error_ctx.message, "Failed to install prerequisites after retries", MAX_ERROR_MSG - 1);
        error_ctx.message[MAX_ERROR_MSG - 1] = '\0';
        log_error_context(&error_ctx);
        return ERROR_DEPENDENCY_MISSING;
    }
    
    LOG_INFO("Prerequisites installed successfully");
    return ERROR_SUCCESS;
}

// Cleanup build
int cleanup_build(build_config_t *config) {
    if (!config) {
        return ERROR_UNKNOWN;
    }
    
    LOG_INFO("Cleaning up build artifacts...");
    
    char cmd[MAX_CMD_LEN];
    snprintf(cmd, sizeof(cmd), "rm -rf %s/* 2>/dev/null || true", config->build_dir);
    execute_command_safe(cmd, 0, NULL);
    
    execute_command_safe("rm -rf /tmp/mali_install 2>/dev/null || true", 0, NULL);
    
    LOG_INFO("Cleanup completed");
    return ERROR_SUCCESS;
}

// Detect current Ubuntu release
int detect_current_ubuntu_release(build_config_t *config) {
    FILE *fp;
    char line[256];
    
    LOG_DEBUG("Detecting current Ubuntu release...");
    
    // Try to read /etc/os-release
    fp = fopen("/etc/os-release", "r");
    if (!fp) {
        LOG_WARNING("Could not open /etc/os-release");
        return ERROR_FILE_NOT_FOUND;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VERSION_ID=", 11) == 0) {
            char *version = line + 11;
            // Remove quotes and newline
            if (*version == '"') version++;
            char *end = strchr(version, '"');
            if (end) *end = '\0';
            end = strchr(version, '\n');
            if (end) *end = '\0';
            
            strncpy(config->ubuntu_release, version, sizeof(config->ubuntu_release) - 1);
            config->ubuntu_release[sizeof(config->ubuntu_release) - 1] = '\0';
        }
        else if (strncmp(line, "VERSION_CODENAME=", 17) == 0) {
            char *codename = line + 17;
            // Remove newline
            char *end = strchr(codename, '\n');
            if (end) *end = '\0';
            
            strncpy(config->ubuntu_codename, codename, sizeof(config->ubuntu_codename) - 1);
            config->ubuntu_codename[sizeof(config->ubuntu_codename) - 1] = '\0';
        }
    }
    
    fclose(fp);
    
    if (strlen(config->ubuntu_release) > 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Detected Ubuntu %s (%s)", 
                 config->ubuntu_release, config->ubuntu_codename);
        LOG_INFO(msg);
        return ERROR_SUCCESS;
    }
    
    LOG_WARNING("Could not detect Ubuntu release");
    return ERROR_UNKNOWN;
}
