/*
 * builder.c - Main program for Orange Pi 5 Plus Ultimate Interactive Builder
 * Version: 0.1.0a
 * 
 * This file contains the program entry point and main build logic.
 */

#include "builder.h"
#include "modules/debug.h"


#if DEBUG_ENABLED
    // Initialize debug system
    debug_init();
#endif

// Global variables
FILE *log_fp = NULL;
FILE *error_log_fp = NULL;
build_config_t *global_config = NULL;
volatile sig_atomic_t interrupted = 0;
menu_state_t menu_state = {0};

// Ubuntu release information
ubuntu_release_t ubuntu_releases[] = {
    {"20.04", "focal", "Ubuntu 20.04 LTS (Focal Fossa)", "5.4", 1, 1, "ubuntu-20.04"},
    {"22.04", "jammy", "Ubuntu 22.04 LTS (Jammy Jellyfish)", "5.15", 1, 1, "ubuntu-22.04"},
    {"24.04", "noble", "Ubuntu 24.04 LTS (Noble Numbat)", "6.8", 1, 1, "ubuntu-24.04"},
    {"25.04", "plucky", "Ubuntu 25.04 (Plucky Puffin)", "6.9", 0, 1, "ubuntu-25.04"},
    {"25.10", "vivid", "Ubuntu 25.10 (Vibrant Vervet)", "6.10", 0, 0, "ubuntu-devel"},
    {"", "", "", "", 0, 0, ""}  // Sentinel
};

// Mali driver information
mali_driver_t mali_drivers[] = {
    {
        "Mali G610 CSF Firmware", 
        "https://github.com/JeffyCN/mirrors/raw/libmali/firmware/g610/mali_csffw.bin",
        "mali_csffw.bin",
        1  // Required
    },
    {
        "Mali G610 Wayland Driver", 
        "https://github.com/JeffyCN/mirrors/raw/libmali/lib/aarch64-linux-gnu/libmali-valhall-g610-g6p0-wayland-gbm.so",
        "libmali-valhall-g610-g6p0-wayland-gbm.so",
        1  // Required
    },
    {
        "Mali G610 X11+Wayland Driver", 
        "https://github.com/JeffyCN/mirrors/raw/libmali/lib/aarch64-linux-gnu/libmali-valhall-g610-g6p0-x11-wayland-gbm.so",
        "libmali-valhall-g610-g6p0-x11-wayland-gbm.so",
        1  // Required
    },
    {
        "Mali G610 Vulkan Driver", 
        "https://github.com/JeffyCN/mirrors/raw/libmali/lib/aarch64-linux-gnu/libmali-valhall-g610-g6p0-wayland-gbm-vulkan.so",
        "libmali-valhall-g610-g6p0-wayland-gbm-vulkan.so",
        0  // Optional
    },
    {
        "", "", "", 0  // Sentinel
    }
};

// Create .env template file (builder.c version - wrapper)
void create_env_template_builder(void) {
    // Just call the system.c version
    create_env_template();
}

// Initialize build configuration
void init_build_config(build_config_t *config) {
    if (!config) return;
    
    // Set default values
    strcpy(config->kernel_version, "6.1.0");  // Use a version that's likely to exist
    strcpy(config->build_dir, BUILD_DIR);
    strcpy(config->output_dir, "/tmp/opi5plus_output");
    strcpy(config->cross_compile, "aarch64-linux-gnu-");
    strcpy(config->arch, "arm64");
    strcpy(config->defconfig, "rockchip_defconfig");
    
    // Ubuntu release - default to a stable, supported version
    strcpy(config->ubuntu_release, "24.04");
    strcpy(config->ubuntu_codename, "noble");
    
    // Distribution type
    config->distro_type = DISTRO_DESKTOP;
    config->emu_platform = EMU_NONE;
    
    // Build options
    config->jobs = sysconf(_SC_NPROCESSORS_ONLN);
    if (config->jobs <= 0) config->jobs = 4;
    
    // Check .env for custom settings
    FILE *fp = fopen(".env", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "BUILD_JOBS=", 11) == 0) {
                int jobs = atoi(line + 11);
                if (jobs > 0 && jobs <= 128) {
                    config->jobs = jobs;
                }
            } else if (strncmp(line, "OUTPUT_DIR=", 11) == 0) {
                char *value = line + 11;
                char *nl = strchr(value, '\n');
                if (nl) *nl = '\0';
                strncpy(config->output_dir, value, sizeof(config->output_dir) - 1);
                config->output_dir[sizeof(config->output_dir) - 1] = '\0';
            }
        }
        fclose(fp);
    }
    
    config->verbose = 0;
    config->clean_build = 0;
    config->continue_on_error = 0;
    config->log_level = LOG_LEVEL_INFO;
    
    // GPU options
    config->install_gpu_blobs = 1;
    config->enable_opencl = 1;
    config->enable_vulkan = 1;
    
    // Component selection
    config->build_kernel = 1;
    config->build_rootfs = 1;
    config->build_uboot = 1;
    config->create_image = 1;
    
    // Image settings
    strcpy(config->image_size, "8192");
    strcpy(config->hostname, "orangepi");
    strcpy(config->username, "orangepi");
    strcpy(config->password, "orangepi");
}

// Create required directories
int ensure_directories_exist(build_config_t *config) {
    char cmd[MAX_CMD_LEN * 2];  // Increased buffer size
    error_context_t error_ctx = {0};
    
    // Create directories one at a time to avoid buffer overflow
    const char *dirs[] = {
        "%s/rootfs/boot",
        "%s/rootfs/etc",
        "%s/rootfs/lib",
        "%s/rootfs/usr/bin",
        "%s/rootfs/usr/lib",
        NULL
    };
    
    for (int i = 0; dirs[i] != NULL; i++) {
        snprintf(cmd, sizeof(cmd), "mkdir -p ");
        int len = strlen(cmd);
        snprintf(cmd + len, sizeof(cmd) - len, dirs[i], config->output_dir);
        
        if (execute_command_safe(cmd, 0, &error_ctx) != 0) {
            LOG_ERROR("Failed to create output directory");
            return ERROR_FILE_NOT_FOUND;
        }
    }
    
    // Create build directory
    if (create_directory_safe(config->build_dir, &error_ctx) != 0) {
        LOG_ERROR("Failed to create build directory");
        return ERROR_FILE_NOT_FOUND;
    }
    
    LOG_DEBUG("All required directories created successfully");
    return ERROR_SUCCESS;
}

// Process command line arguments
void process_args(int argc, char *argv[], build_config_t *config) {
    int i;
    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("Options:\n");
            printf("  --kernel-version VERSION   Kernel version (default: %s)\n", config->kernel_version);
            printf("  --build-dir DIR           Build directory (default: %s)\n", config->build_dir);
            printf("  --output-dir DIR          Output directory (default: %s)\n", config->output_dir);
            printf("  --jobs N                  Number of parallel jobs (default: %d)\n", config->jobs);
            printf("  --ubuntu VERSION          Ubuntu release (default: %s)\n", config->ubuntu_release);
            printf("  --disable-gpu             Disable Mali GPU support\n");
            printf("  --disable-opencl          Disable OpenCL support\n");
            printf("  --disable-vulkan          Disable Vulkan support\n");
            printf("  --no-kernel               Skip kernel building\n");
            printf("  --no-rootfs               Skip rootfs building\n");
            printf("  --no-uboot                Skip U-Boot building\n");
            printf("  --no-image                Skip image creation\n");
            printf("  --clean                   Clean previous build\n");
            printf("  --verbose                 Verbose output\n");
            printf("  --help                    Show this help\n");
            exit(0);
        } else if (strcmp(argv[i], "--kernel-version") == 0) {
            if (i + 1 < argc) {
                strncpy(config->kernel_version, argv[i + 1], sizeof(config->kernel_version) - 1);
                config->kernel_version[sizeof(config->kernel_version) - 1] = '\0';
                i++;
            }
        } else if (strcmp(argv[i], "--build-dir") == 0) {
            if (i + 1 < argc) {
                strncpy(config->build_dir, argv[i + 1], sizeof(config->build_dir) - 1);
                config->build_dir[sizeof(config->build_dir) - 1] = '\0';
                i++;
            }
        } else if (strcmp(argv[i], "--output-dir") == 0) {
            if (i + 1 < argc) {
                strncpy(config->output_dir, argv[i + 1], sizeof(config->output_dir) - 1);
                config->output_dir[sizeof(config->output_dir) - 1] = '\0';
                i++;
            }
        } else if (strcmp(argv[i], "--jobs") == 0) {
            if (i + 1 < argc) {
                config->jobs = atoi(argv[i + 1]);
                i++;
            }
        } else if (strcmp(argv[i], "--ubuntu") == 0) {
            if (i + 1 < argc) {
                strncpy(config->ubuntu_release, argv[i + 1], sizeof(config->ubuntu_release) - 1);
                config->ubuntu_release[sizeof(config->ubuntu_release) - 1] = '\0';
                ubuntu_release_t *release = find_ubuntu_release(config->ubuntu_release);
                if (release) {
                    strncpy(config->ubuntu_codename, release->codename, sizeof(config->ubuntu_codename) - 1);
                    config->ubuntu_codename[sizeof(config->ubuntu_codename) - 1] = '\0';
                }
                i++;
            }
        } else if (strcmp(argv[i], "--disable-gpu") == 0) {
            config->install_gpu_blobs = 0;
        } else if (strcmp(argv[i], "--disable-opencl") == 0) {
            config->enable_opencl = 0;
        } else if (strcmp(argv[i], "--disable-vulkan") == 0) {
            config->enable_vulkan = 0;
        } else if (strcmp(argv[i], "--no-kernel") == 0) {
            config->build_kernel = 0;
        } else if (strcmp(argv[i], "--no-rootfs") == 0) {
            config->build_rootfs = 0;
        } else if (strcmp(argv[i], "--no-uboot") == 0) {
            config->build_uboot = 0;
        } else if (strcmp(argv[i], "--no-image") == 0) {
            config->create_image = 0;
        } else if (strcmp(argv[i], "--clean") == 0) {
            config->clean_build = 1;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config->verbose = 1;
        } else {
            printf("Unknown option: %s\n", argv[i]);
        }
    }
}

// Start interactive build
int start_interactive_build(build_config_t *config) {
    int choice;
    
    print_header();
    print_legal_notice();
    
    while (1) {
        show_main_menu();
        
        choice = get_user_choice("Enter your choice", 0, 6);
        
        switch (choice) {
            case 0:  // Exit
                if (confirm_action("Exit the builder?")) {
                    LOG_INFO("Exiting builder");
                    return ERROR_SUCCESS;
                }
                break;
                
            case 1:  // Quick Setup
                show_quick_setup_menu();
                if (confirm_action("Proceed with quick setup?")) {
                    return perform_quick_setup(config);
                }
                break;
                
            case 2:  // Custom Build
                return perform_custom_build(config);
                
            case 3:  // Emulation Focus
                config->distro_type = DISTRO_EMULATION;
                show_emulation_menu();
                choice = get_user_choice("Select emulation platform", 0, 5);
                if (choice > 0) {
                    config->emu_platform = choice;
                    if (confirm_action("Proceed with emulation build?")) {
                        return perform_custom_build(config);
                    }
                }
                break;
                
            case 4:  // Documentation
                show_help_menu();
                break;
                
            case 5:  // System Requirements
                LOG_INFO("Checking system requirements...");
                check_root_permissions();
                check_dependencies();
                check_disk_space("/tmp", 15000);
                pause_screen();
                break;
                
            case 6:  // About
                clear_screen();
                print_header();
                printf("\n%s%sABOUT ORANGE PI 5 PLUS ULTIMATE INTERACTIVE BUILDER%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
                printf("════════════════════════════════════════════════════════════════════════\n");
                printf("\n");
                printf("Version: %s\n", VERSION);
                printf("License: GPLv3\n");
                printf("Author: Setec Labs\n");
                printf("\n");
                printf("Features:\n");
                printf("• Build custom Ubuntu distributions for Orange Pi 5 Plus\n");
                printf("• Full Mali G610 GPU support (OpenCL 2.2, Vulkan 1.2)\n");
                printf("• Multiple Ubuntu versions (20.04 LTS through 25.04)\n");
                printf("• Desktop, Server, Minimal, or Emulation-focused builds\n");
                printf("• Automated kernel compilation with Rockchip patches\n");
                printf("• U-Boot bootloader support\n");
                printf("• Legal emulation platform support (NO copyrighted content)\n");
                printf("\n");
                printf("For more information, see https://github.com/seteclabs/orangepi-builder\n");
                printf("\n");
                pause_screen();
                break;
        }
    }
    
    return ERROR_SUCCESS;
}

// Perform quick setup - FIXED VERSION
int perform_quick_setup(build_config_t *config) {
    int result;
    
    // Set default quick setup options
    config->distro_type = DISTRO_DESKTOP;
    config->emu_platform = EMU_NONE;
    strcpy(config->ubuntu_release, "24.04");  // Use stable Noble instead of development version
    strcpy(config->ubuntu_codename, "noble");
    config->install_gpu_blobs = 1;
    config->enable_opencl = 1;
    config->enable_vulkan = 1;
    config->build_kernel = 1;
    config->build_rootfs = 1;
    config->build_uboot = 1;
    config->create_image = 1;
    
    clear_screen();
    print_header();
    show_build_summary(config);
    
    if (!confirm_action("Start quick setup build?")) {
        return ERROR_USER_CANCELLED;
    }
    
    LOG_INFO("Starting quick setup build...");
    
    // Ensure output directories exist
    result = ensure_directories_exist(config);
    if (result != ERROR_SUCCESS && !config->continue_on_error) {
        return result;
    }
    
    // Setup build environment
    result = setup_build_environment();
    if (result != ERROR_SUCCESS && !config->continue_on_error) {
        return result;
    }
    
    // Install prerequisites
    result = install_prerequisites();
    if (result != ERROR_SUCCESS && !config->continue_on_error) {
        return result;
    }
    
    // Download kernel source
    result = download_kernel_source(config);
    if (result != ERROR_SUCCESS && !config->continue_on_error) {
        return result;
    }
    
    // Configure kernel
    result = configure_kernel(config);
    if (result != ERROR_SUCCESS && !config->continue_on_error) {
        return result;
    }
    
    // Build kernel
    result = build_kernel(config);
    if (result != ERROR_SUCCESS && !config->continue_on_error) {
        return result;
    }
    
    // Download Mali blobs
    if (config->install_gpu_blobs) {
        result = download_mali_blobs(config);
        if (result != ERROR_SUCCESS && !config->continue_on_error) {
            return result;
        }
    }
    
    // Build rootfs
    if (config->build_rootfs) {
        result = build_ubuntu_rootfs(config);
        if (result != ERROR_SUCCESS && !config->continue_on_error) {
            return result;
        }
    }
    
    // Install kernel
    result = install_kernel(config);
    if (result != ERROR_SUCCESS && !config->continue_on_error) {
        return result;
    }
    
    // Install Mali drivers
    if (config->install_gpu_blobs) {
        result = install_mali_drivers(config);
        if (result != ERROR_SUCCESS && !config->continue_on_error) {
            return result;
        }
        
        if (config->enable_opencl) {
            result = setup_opencl_support(config);
            if (result != ERROR_SUCCESS && !config->continue_on_error) {
                return result;
            }
        }
        
        if (config->enable_vulkan) {
            result = setup_vulkan_support(config);
            if (result != ERROR_SUCCESS && !config->continue_on_error) {
                return result;
            }
        }
    }
    
    // Install system packages
    result = install_system_packages(config);
    if (result != ERROR_SUCCESS && !config->continue_on_error) {
        return result;
    }
    
    // Configure system services
    result = configure_system_services(config);
    if (result != ERROR_SUCCESS && !config->continue_on_error) {
        return result;
    }
    
    // Build U-Boot
    if (config->build_uboot) {
        result = download_uboot_source(config);
        if (result != ERROR_SUCCESS && !config->continue_on_error) {
            return result;
        }
        
        result = build_uboot(config);
        if (result != ERROR_SUCCESS && !config->continue_on_error) {
            return result;
        }
    }
    
    // Create system image
    if (config->create_image) {
        result = create_system_image(config);
        if (result != ERROR_SUCCESS && !config->continue_on_error) {
            return result;
        }
    }
    
    LOG_INFO("Build completed successfully!");
    
    clear_screen();
    print_header();
    printf("\n%s%sBUILD COMPLETED SUCCESSFULLY!%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Output files:\n");
    
    if (config->create_image) {
        // Fix for format truncation warning - use larger buffer and safer construction
        char image_path[MAX_PATH_LEN + 256];  // Much larger buffer
        int ret = snprintf(image_path, sizeof(image_path), 
                          "%s/orangepi5plus-%s-%s.img",
                          config->output_dir, config->ubuntu_codename, config->kernel_version);
        
        // Check for truncation
        if (ret >= (int)sizeof(image_path)) {
            LOG_WARNING("Image path was truncated - using shorter fallback name");
            snprintf(image_path, sizeof(image_path), 
                     "%s/orangepi5plus.img", config->output_dir);
        }
        
        printf("• System image: %s\n", image_path);
        printf("\n");
        printf("Flash the image to your SD card with:\n");
        printf("  dd if=%s of=/dev/sdX bs=4M status=progress\n", image_path);
    } else {
        printf("• Kernel: %s/rootfs/boot/vmlinuz-%s\n", 
               config->output_dir, config->kernel_version);
        printf("• Device tree: %s/rootfs/boot/rk3588*.dtb\n", config->output_dir);
        if (config->build_uboot) {
            printf("• U-Boot: %s/idbloader.img\n", config->output_dir);
        }
    }
    
    printf("\n");
    pause_screen();
    
    return ERROR_SUCCESS;
}

// Perform custom build
int perform_custom_build(build_config_t *config) {
    int choice;
    int submenu_active = 1;
    
    while (submenu_active) {
        show_custom_build_menu();
        
        choice = get_user_choice("Enter your choice", 0, 7);
        
        switch (choice) {
            case 0:  // Back
                submenu_active = 0;
                break;
                
            case 1:  // Distribution Type
                show_distro_selection_menu();
                choice = get_user_choice("Select distribution type", 0, 4);
                if (choice > 0) {
                    config->distro_type = choice - 1;
                    
                    if (config->distro_type == DISTRO_EMULATION) {
                        show_emulation_menu();
                        choice = get_user_choice("Select emulation platform", 0, 5);
                        if (choice > 0) {
                            config->emu_platform = choice;
                        }
                    }
                }
                break;
                
            case 2:  // Ubuntu Version
                show_ubuntu_selection_menu();
                // Count available releases
                int release_count = 0;
                while (strlen(ubuntu_releases[release_count].version) > 0) {
                    release_count++;
                }
                
                choice = get_user_choice("Select Ubuntu version", 0, release_count);
                if (choice > 0) {
                    strncpy(config->ubuntu_release, ubuntu_releases[choice-1].version, 
                            sizeof(config->ubuntu_release) - 1);
                    config->ubuntu_release[sizeof(config->ubuntu_release) - 1] = '\0';
                    strncpy(config->ubuntu_codename, ubuntu_releases[choice-1].codename, 
                            sizeof(config->ubuntu_codename) - 1);
                    config->ubuntu_codename[sizeof(config->ubuntu_codename) - 1] = '\0';
                }
                break;
                
            case 3:  // Kernel Options
                clear_screen();
                print_header();
                printf("\n%s%sKERNEL OPTIONS%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
                printf("════════════════════════════════════════════════════════════════════════\n");
                printf("\n");
                printf("Current kernel version: %s\n", config->kernel_version);
                printf("\n");
                printf("1. Change kernel version\n");
                printf("2. Back\n");
                printf("\n");
                
                choice = get_user_choice("Select option", 1, 2);
                if (choice == 1) {
                    char buffer[64];
                    get_user_input("Enter kernel version (e.g. 6.1.0): ", buffer, sizeof(buffer));
                    if (strlen(buffer) > 0) {
                        strncpy(config->kernel_version, buffer, sizeof(config->kernel_version) - 1);
                        config->kernel_version[sizeof(config->kernel_version) - 1] = '\0';
                    }
                }
                break;
                
            case 4:  // GPU Configuration
                show_gpu_options_menu(config);
                choice = get_user_choice("Select option", 0, 5);
                switch (choice) {
                    case 1:  // Toggle Mali GPU drivers
                        config->install_gpu_blobs = !config->install_gpu_blobs;
                        break;
                    case 2:  // Toggle OpenCL support
                        config->enable_opencl = !config->enable_opencl;
                        break;
                    case 3:  // Toggle Vulkan support
                        config->enable_vulkan = !config->enable_vulkan;
                        break;
                    case 4:  // Enable all GPU features
                        config->install_gpu_blobs = 1;
                        config->enable_opencl = 1;
                        config->enable_vulkan = 1;
                        break;
                    case 5:  // Disable all GPU features
                        config->install_gpu_blobs = 0;
                        config->enable_opencl = 0;
                        config->enable_vulkan = 0;
                        break;
                }
                break;
                
            case 5:  // Build Components
                clear_screen();
                print_header();
                printf("\n%s%sBUILD COMPONENTS%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
                printf("════════════════════════════════════════════════════════════════════════\n");
                printf("\n");
                printf("Current settings:\n");
                printf("• Kernel: %s\n", config->build_kernel ? "Yes" : "No");
                printf("• Root filesystem: %s\n", config->build_rootfs ? "Yes" : "No");
                printf("• U-Boot: %s\n", config->build_uboot ? "Yes" : "No");
                printf("• System image: %s\n", config->create_image ? "Yes" : "No");
                printf("\n");
                printf("1. Toggle kernel building\n");
                printf("2. Toggle rootfs building\n");
                printf("3. Toggle U-Boot building\n");
                printf("4. Toggle system image creation\n");
                printf("5. Enable all components\n");
                printf("6. Back\n");
                printf("\n");
                
                choice = get_user_choice("Select option", 1, 6);
                switch (choice) {
                    case 1:
                        config->build_kernel = !config->build_kernel;
                        break;
                    case 2:
                        config->build_rootfs = !config->build_rootfs;
                        break;
                    case 3:
                        config->build_uboot = !config->build_uboot;
                        break;
                    case 4:
                        config->create_image = !config->create_image;
                        break;
                    case 5:
                        config->build_kernel = 1;
                        config->build_rootfs = 1;
                        config->build_uboot = 1;
                        config->create_image = 1;
                        break;
                }
                break;
                
            case 6:  // Image Settings
                clear_screen();
                print_header();
                printf("\n%s%sIMAGE SETTINGS%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
                printf("════════════════════════════════════════════════════════════════════════\n");
                printf("\n");
                printf("Current settings:\n");
                printf("• Image size: %s MB\n", config->image_size);
                printf("• Hostname: %s\n", config->hostname);
                printf("• Username: %s\n", config->username);
                printf("• Password: %s\n", config->password);
                printf("\n");
                printf("1. Change image size\n");
                printf("2. Change hostname\n");
                printf("3. Change username\n");
                printf("4. Change password\n");
                printf("5. Back\n");
                printf("\n");
                
                choice = get_user_choice("Select option", 1, 5);
                
                char buffer[64];
                switch (choice) {
                    case 1:
                        get_user_input("Enter image size in MB (min 4096): ", buffer, sizeof(buffer));
                        if (strlen(buffer) > 0 && atoi(buffer) >= 4096) {
                            strncpy(config->image_size, buffer, sizeof(config->image_size) - 1);
                            config->image_size[sizeof(config->image_size) - 1] = '\0';
                        }
                        break;
                    case 2:
                        get_user_input("Enter hostname: ", buffer, sizeof(buffer));
                        if (strlen(buffer) > 0) {
                            strncpy(config->hostname, buffer, sizeof(config->hostname) - 1);
                            config->hostname[sizeof(config->hostname) - 1] = '\0';
                        }
                        break;
                    case 3:
                        get_user_input("Enter username: ", buffer, sizeof(buffer));
                        if (strlen(buffer) > 0) {
                            strncpy(config->username, buffer, sizeof(config->username) - 1);
                            config->username[sizeof(config->username) - 1] = '\0';
                        }
                        break;
                    case 4:
                        get_user_input("Enter password: ", buffer, sizeof(buffer));
                        if (strlen(buffer) > 0) {
                            strncpy(config->password, buffer, sizeof(config->password) - 1);
                            config->password[sizeof(config->password) - 1] = '\0';
                        }
                        break;
                }
                break;
                
            case 7:  // Start Build
                show_build_summary(config);
                if (confirm_action("Start custom build?")) {
                    return perform_quick_setup(config);
                }
                break;
        }
    }
    
    return ERROR_SUCCESS;
}

// Main entry point
int main(int argc, char *argv[]) {
    int result = ERROR_SUCCESS;
    build_config_t config;
    
    // Setup signal handlers
    setup_signal_handlers();
    
#if DEBUG_ENABLED
    // Initialize debug system
    debug_init();
#endif
    
    // Create .env template if it doesn't exist
    create_env_template_builder();
    
    // Log GitHub token status
    char* token = get_github_token();
    if (token != NULL && strlen(token) > 0) {
        fprintf(stdout, "[INFO] GitHub authentication token found\n");
    } else {
        fprintf(stdout, "[WARNING] No GitHub authentication token found. Some operations may fail.\n");
        fprintf(stdout, "[WARNING] Please add a token to the .env file or set the GITHUB_TOKEN environment variable.\n");
    }
    
    // Initialize configuration
    init_build_config(&config);
    global_config = &config;
    
	char* test_token = get_github_token();
if (test_token != NULL && strlen(test_token) > 0) {
    printf("[DEBUG] GitHub token loaded: %c***%c (length: %zu)\n", 
           test_token[0], test_token[strlen(test_token)-1], strlen(test_token));
} else {
    printf("[DEBUG] No GitHub token found!\n");
}
    // Process command line arguments
    process_args(argc, argv, &config);
    
    // Validate configuration
    result = validate_config(&config);
    if (result != ERROR_SUCCESS) {
        fprintf(stderr, "Invalid configuration. Exiting.\n");
        return result;
    }
    
    // Check root permissions
    result = check_root_permissions();
    if (result != ERROR_SUCCESS) {
        fprintf(stderr, "Root permissions required. Run with sudo.\n");
        return result;
    }
    
    // Non-interactive mode
    if (argc > 1) {
        // Ensure directories exist
        result = ensure_directories_exist(&config);
        if (result != ERROR_SUCCESS && !config.continue_on_error) {
            return result;
        }
        
        // Setup build environment
        result = setup_build_environment();
        if (result != ERROR_SUCCESS && !config.continue_on_error) {
            return result;
        }
        
        // Perform build
        result = perform_quick_setup(&config);
    } else {
        // Interactive mode
        result = start_interactive_build(&config);
    }
    
     // Cleanup
    if (log_fp) {
        fclose(log_fp);
    }
    if (error_log_fp) {
        fclose(error_log_fp);
    }
    
#if DEBUG_ENABLED
    // Cleanup debug system
    debug_cleanup();
#endif

}
