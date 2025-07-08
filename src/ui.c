/*
 * ui.c - User interface functions for Orange Pi 5 Plus Ultimate Interactive Builder
 * Version: 0.1.0a
 * 
 * This file contains all menu display and user interaction functions.
 */

#include "builder.h"

// Print header
void print_header(void) {
    clear_screen();
    printf("%s%s", COLOR_BOLD, COLOR_CYAN);
    printf("╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║           ORANGE PI 5 PLUS ULTIMATE INTERACTIVE BUILDER v%s              ║\n", VERSION);
    printf("║                         Setec Labs Edition                                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");
    printf("%s", COLOR_RESET);
}

// Print legal notice
void print_legal_notice(void) {
    clear_screen();
    print_header();
    printf("\n%s%sIMPORTANT LEGAL NOTICE:%s\n", COLOR_BOLD, COLOR_RED, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("%s• This software is provided by Setec Labs for legitimate purposes only%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s• NO games, BIOS files, or copyrighted software will be installed%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s• Setec Labs does not support piracy in any form%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s• Users are responsible for complying with all applicable laws%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s• Emulation platforms are installed WITHOUT any copyrighted content%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s• You must legally own any games/software you intend to use%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("\n");
    printf("By continuing, you acknowledge that:\n");
    printf("1. You will only use legally obtained software\n");
    printf("2. You understand the legal requirements in your jurisdiction\n");
    printf("3. You will not use this tool for piracy or copyright infringement\n");
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Press ENTER to accept and continue, or Ctrl+C to exit...");
    getchar();
}

// Clear screen
void clear_screen(void) {
    printf(CLEAR_SCREEN);
}

// Pause screen
void pause_screen(void) {
    printf("\nPress ENTER to continue...");
    getchar();
}

// Get user input
char* get_user_input(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    fflush(stdout);
    
    if (fgets(buffer, size, stdin) == NULL) {
        return NULL;
    }
    
    // Remove newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
    
    return buffer;
}

// Get user choice
int get_user_choice(const char *prompt, int min, int max) {
    char buffer[32];
    int choice;
    
    while (1) {
        printf("%s (%d-%d): ", prompt, min, max);
        fflush(stdout);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            return -1;
        }
        
        if (sscanf(buffer, "%d", &choice) == 1) {
            if (choice >= min && choice <= max) {
                return choice;
            }
        }
        
        printf("%sInvalid choice. Please try again.%s\n", COLOR_RED, COLOR_RESET);
    }
}

// Confirm action
int confirm_action(const char *message) {
    char buffer[32];
    
    printf("\n%s%s%s\n", COLOR_YELLOW, message, COLOR_RESET);
    printf("Are you sure? (y/N): ");
    fflush(stdout);
    
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0;
    }
    
    return (buffer[0] == 'y' || buffer[0] == 'Y');
}

// Show main menu
void show_main_menu(void) {
    clear_screen();
    print_header();
    
    printf("\n%s%sMAIN MENU%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("  %s1.%s Quick Setup          - Guided setup with recommended settings\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s2.%s Custom Build         - Advanced configuration options\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s3.%s Emulation Focus      - Build optimized for retro gaming\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s4.%s Documentation        - View build documentation\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s5.%s System Requirements  - Check prerequisites\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s6.%s About               - About this builder\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s0.%s Exit                - Exit the builder\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
}

// Show quick setup menu
void show_quick_setup_menu(void) {
    clear_screen();
    print_header();
    
    printf("\n%s%sQUICK SETUP%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("This will build a complete Orange Pi 5 Plus system with:\n");
    printf("\n");
    printf("  • %sUbuntu 25.04 (Plucky Puffin)%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  • %sLatest stable kernel (6.8+)%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  • %sFull Mali G610 GPU support%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  • %sOpenCL 2.2 and Vulkan 1.2%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  • %sGNOME desktop environment%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  • %sHardware video acceleration%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("\n");
    printf("Estimated build time: 30-60 minutes\n");
    printf("Required disk space: 15GB\n");
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
}

// Show custom build menu
void show_custom_build_menu(void) {
    clear_screen();
    print_header();
    
    printf("\n%s%sCUSTOM BUILD OPTIONS%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("  %s1.%s Distribution Type    - Desktop/Server/Minimal/Emulation\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s2.%s Ubuntu Version       - Select Ubuntu release\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s3.%s Kernel Options       - Configure kernel version\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s4.%s GPU Configuration    - Mali driver options\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s5.%s Build Components     - Select what to build\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s6.%s Image Settings       - Configure output image\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s7.%s Start Build          - Begin building\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s0.%s Back                 - Return to main menu\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
}

// Show distribution selection menu
void show_distro_selection_menu(void) {
    clear_screen();
    print_header();
    
    printf("\n%s%sDISTRIBUTION TYPE%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("  %s1.%s Desktop Edition\n", COLOR_CYAN, COLOR_RESET);
    printf("     • Full GNOME desktop environment\n");
    printf("     • Office and productivity software\n");
    printf("     • Web browsers and multimedia apps\n");
    printf("     • Development tools\n");
    printf("\n");
    printf("  %s2.%s Server Edition\n", COLOR_CYAN, COLOR_RESET);
    printf("     • Minimal installation\n");
    printf("     • Server utilities and tools\n");
    printf("     • Container runtime support\n");
    printf("     • Network services\n");
    printf("\n");
    printf("  %s3.%s Emulation Station\n", COLOR_CYAN, COLOR_RESET);
    printf("     • Optimized for retro gaming\n");
    printf("     • Multiple emulation platforms\n");
    printf("     • Media center capabilities\n");
    printf("     • %sNO GAMES OR BIOS INCLUDED%s\n", COLOR_RED, COLOR_RESET);
    printf("\n");
    printf("  %s4.%s Minimal System\n", COLOR_CYAN, COLOR_RESET);
    printf("     • Base system only\n");
    printf("     • Essential packages\n");
    printf("     • Smallest footprint\n");
    printf("\n");
    printf("  %s0.%s Back\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
}

// Show emulation menu
void show_emulation_menu(void) {
    clear_screen();
    print_header();
    
    printf("\n%s%sEMULATION PLATFORM SELECTION%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("%s%sLEGAL NOTICE: NO copyrighted games, BIOS files, or ROMs will be installed!%s\n", COLOR_BOLD, COLOR_RED, COLOR_RESET);
    printf("%sYou must provide your own legally obtained content.%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("\n");
    printf("Select emulation platform:\n");
    printf("\n");
    printf("  %s1.%s LibreELEC\n", COLOR_CYAN, COLOR_RESET);
    printf("     • Lightweight media center OS\n");
    printf("     • Kodi-based interface\n");
    printf("     • Minimal resource usage\n");
    printf("     • Supports RetroArch cores\n");
    printf("\n");
    printf("  %s2.%s EmulationStation\n", COLOR_CYAN, COLOR_RESET);
    printf("     • Frontend for multiple emulators\n");
    printf("     • Customizable themes\n");
    printf("     • Scraper for game metadata\n");
    printf("     • Controller configuration\n");
    printf("\n");
    printf("  %s3.%s RetroPie\n", COLOR_CYAN, COLOR_RESET);
    printf("     • Complete emulation solution\n");
    printf("     • Pre-configured emulators\n");
    printf("     • User-friendly setup\n");
    printf("     • Active community support\n");
    printf("\n");
    printf("  %s4.%s Lakka\n", COLOR_CYAN, COLOR_RESET);
    printf("     • RetroArch-based OS\n");
    printf("     • Plug-and-play design\n");
    printf("     • Network play support\n");
    printf("     • Minimal configuration\n");
    printf("\n");
    printf("  %s5.%s All Platforms\n", COLOR_CYAN, COLOR_RESET);
    printf("     • Install all emulation platforms\n");
    printf("     • Choose at boot time\n");
    printf("     • Maximum compatibility\n");
    printf("\n");
    printf("  %s0.%s Back\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
}

// Show Ubuntu selection menu
void show_ubuntu_selection_menu(void) {
    clear_screen();
    print_header();
    
    printf("\n%s%sUBUNTU VERSION SELECTION%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Available Ubuntu releases:\n");
    printf("\n");
    
    int i;
    for (i = 0; strlen(ubuntu_releases[i].version) > 0; i++) {
        ubuntu_release_t *rel = &ubuntu_releases[i];
        const char *type = rel->is_lts ? "LTS" : "Regular";
        const char *status = rel->is_supported ? "Supported" : "Preview";
        const char *color = rel->is_lts ? COLOR_GREEN : COLOR_YELLOW;
        
        printf("  %s%d.%s %s (%s) - %s %s\n", 
               COLOR_CYAN, i+1, COLOR_RESET,
               rel->version, rel->codename, type, status);
        printf("     • %s%s%s\n", color, rel->full_name, COLOR_RESET);
        printf("     • Kernel: %s\n", rel->kernel_version);
        printf("\n");
    }
    
    printf("  %s0.%s Back\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
}

// Show GPU options menu
void show_gpu_options_menu(build_config_t *config) {
    clear_screen();
    print_header();
    
    printf("\n%s%sGPU CONFIGURATION%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Current settings:\n");
    printf("  • Mali GPU drivers: %s%s%s\n", 
           config->install_gpu_blobs ? COLOR_GREEN : COLOR_RED,
           config->install_gpu_blobs ? "Enabled" : "Disabled",
           COLOR_RESET);
    printf("  • OpenCL support: %s%s%s\n",
           config->enable_opencl ? COLOR_GREEN : COLOR_RED,
           config->enable_opencl ? "Enabled" : "Disabled",
           COLOR_RESET);
    printf("  • Vulkan support: %s%s%s\n",
           config->enable_vulkan ? COLOR_GREEN : COLOR_RED,
           config->enable_vulkan ? "Enabled" : "Disabled",
           COLOR_RESET);
    printf("\n");
    printf("Options:\n");
    printf("  %s1.%s Toggle Mali GPU drivers\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s2.%s Toggle OpenCL support\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s3.%s Toggle Vulkan support\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s4.%s Enable all GPU features\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s5.%s Disable all GPU features\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s0.%s Back\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
}

// Show build options menu
void show_build_options_menu(void) {
    clear_screen();
    print_header();
    
    printf("\n%s%sBUILD OPTIONS%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Configure build settings:\n");
    printf("\n");
    printf("  %s1.%s Parallel Jobs        - Current: %d\n", COLOR_CYAN, COLOR_RESET, 
           global_config ? global_config->jobs : 4);
    printf("  %s2.%s Verbose Output       - Current: %s\n", COLOR_CYAN, COLOR_RESET,
           global_config && global_config->verbose ? "Enabled" : "Disabled");
    printf("  %s3.%s Clean Build          - Current: %s\n", COLOR_CYAN, COLOR_RESET,
           global_config && global_config->clean_build ? "Yes" : "No");
    printf("  %s4.%s Continue on Error    - Current: %s\n", COLOR_CYAN, COLOR_RESET,
           global_config && global_config->continue_on_error ? "Yes" : "No");
    printf("  %s5.%s Log Level            - Current: ", COLOR_CYAN, COLOR_RESET);
    
    if (global_config) {
        switch (global_config->log_level) {
            case LOG_LEVEL_DEBUG: printf("Debug\n"); break;
            case LOG_LEVEL_INFO: printf("Info\n"); break;
            case LOG_LEVEL_WARNING: printf("Warning\n"); break;
            case LOG_LEVEL_ERROR: printf("Error\n"); break;
            case LOG_LEVEL_CRITICAL: printf("Critical\n"); break;
        }
    } else {
        printf("Info\n");
    }
    
    printf("  %s6.%s Build Directory      - Current: %s\n", COLOR_CYAN, COLOR_RESET,
           global_config ? global_config->build_dir : BUILD_DIR);
    printf("  %s7.%s Output Directory     - Current: %s\n", COLOR_CYAN, COLOR_RESET,
           global_config ? global_config->output_dir : "./output");
    printf("\n");
    printf("  %s0.%s Back\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
}

// Show image settings menu
void show_image_settings_menu(build_config_t *config) {
    int choice;
    
    while (1) {
        clear_screen();
        print_header();
        printf("\n%s%sIMAGE SETTINGS%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
        printf("════════════════════════════════════════════════════════════════════════\n");
        printf("\n");
        printf("Current settings:\n");
        printf("• Output directory: %s\n", config->output_dir);
        printf("• Image size: %s MB\n", config->image_size);
        printf("• Hostname: %s\n", config->hostname);
        printf("• Username: %s\n", config->username);
        printf("• Password: %s\n", config->password);
        printf("\n");
        printf("1. Change output directory\n");
        printf("2. Change image size\n");
        printf("3. Change hostname\n");
        printf("4. Change username\n");
        printf("5. Change password\n");
        printf("0. Back\n");
        printf("\n");
        
        choice = get_user_choice("Select option", 0, 5);
        
        char buffer[MAX_PATH_LEN];
        switch (choice) {
            case 1:
                printf("Current output directory: %s\n", config->output_dir);
                get_user_input("Enter new output directory path: ", buffer, sizeof(buffer));
                if (strlen(buffer) > 0) {
                    // Expand ~ to home directory
                    if (buffer[0] == '~') {
                        char *home = getenv("HOME");
                        if (home) {
                            snprintf(config->output_dir, sizeof(config->output_dir), 
                                    "%s%s", home, buffer + 1);
                        } else {
                            strncpy(config->output_dir, buffer, sizeof(config->output_dir) - 1);
                        }
                    } else {
                        strncpy(config->output_dir, buffer, sizeof(config->output_dir) - 1);
                    }
                    config->output_dir[sizeof(config->output_dir) - 1] = '\0';
                    
                    // Create directory if it doesn't exist
                    error_context_t error_ctx = {0};
                    if (create_directory_safe(config->output_dir, &error_ctx) != 0) {
                        LOG_WARNING("Failed to create output directory");
                    }
                }
                break;
            case 2:
                get_user_input("Enter image size in MB (min 4096): ", buffer, sizeof(buffer));
                if (strlen(buffer) > 0 && atoi(buffer) >= 4096) {
                    strncpy(config->image_size, buffer, sizeof(config->image_size) - 1);
                    config->image_size[sizeof(config->image_size) - 1] = '\0';
                }
                break;
            case 3:
                get_user_input("Enter hostname: ", buffer, sizeof(buffer));
                if (strlen(buffer) > 0) {
                    strncpy(config->hostname, buffer, sizeof(config->hostname) - 1);
                    config->hostname[sizeof(config->hostname) - 1] = '\0';
                }
                break;
            case 4:
                get_user_input("Enter username: ", buffer, sizeof(buffer));
                if (strlen(buffer) > 0) {
                    strncpy(config->username, buffer, sizeof(config->username) - 1);
                    config->username[sizeof(config->username) - 1] = '\0';
                }
                break;
            case 5:
                get_user_input("Enter password: ", buffer, sizeof(buffer));
                if (strlen(buffer) > 0) {
                    strncpy(config->password, buffer, sizeof(config->password) - 1);
                    config->password[sizeof(config->password) - 1] = '\0';
                }
                break;
            case 0:
                return;
            default:
                break;
        }
        
        if (choice != 0) {
            pause_screen();
        }
    }
}

// Show advanced menu
void show_advanced_menu(void) {
    clear_screen();
    print_header();
    
    printf("\n%s%sADVANCED OPTIONS%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Advanced configuration options:\n");
    printf("\n");
    printf("  %s1.%s Kernel Configuration  - Manually edit kernel config\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s2.%s Boot Parameters       - Configure kernel boot args\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s3.%s Device Tree           - Custom device tree options\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s4.%s Overclocking         - CPU/GPU frequency settings\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s5.%s Network Config        - Pre-configure networking\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s6.%s Package Selection     - Custom package lists\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s7.%s Partition Layout     - Custom disk partitioning\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s8.%s Post-Install Script  - Add custom scripts\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    printf("  %s0.%s Back\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
}

// Show help menu
void show_help_menu(void) {
    clear_screen();
    print_header();
    
    printf("\n%s%sHELP & DOCUMENTATION%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("%sQuick Start Guide:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("1. Choose 'Quick Setup' for a standard desktop build\n");
    printf("2. Or select 'Custom Build' for advanced options\n");
    printf("3. Follow the prompts to configure your build\n");
    printf("4. The builder will download and compile everything\n");
    printf("\n");
    printf("%sDistribution Types:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("• Desktop: Full GUI with GNOME desktop\n");
    printf("• Server: Minimal installation for servers\n");
    printf("• Emulation: Optimized for retro gaming (NO ROMs included)\n");
    printf("• Minimal: Base system only\n");
    printf("\n");
    printf("%sGPU Support:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("• Mali G610 drivers are included\n");
    printf("• OpenCL 2.2 for compute workloads\n");
    printf("• Vulkan 1.2 for modern graphics\n");
    printf("\n");
    printf("%sTroubleshooting:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("• Check logs in: %s\n", LOG_FILE);
    printf("• Error logs in: %s\n", ERROR_LOG_FILE);
    printf("• Ensure at least 15GB free disk space\n");
    printf("• Run with sudo for root permissions\n");
    printf("\n");
    pause_screen();
}

// Show build progress
void show_build_progress(const char *stage, int percent) {
    const int bar_width = 50;
    int filled = (bar_width * percent) / 100;
    
    printf("\r%s: [", stage);
    
    int i;
    for (i = 0; i < bar_width; i++) {
        if (i < filled) {
            printf("%s█%s", COLOR_GREEN, COLOR_RESET);
        } else {
            printf("-");
        }
    }
    
    printf("] %d%%", percent);
    fflush(stdout);
    
    if (percent >= 100) {
        printf("\n");
    }
}

// Show build summary
void show_build_summary(build_config_t *config) {
    clear_screen();
    print_header();
    
    printf("\n%s%sBUILD SUMMARY%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Distribution Type: ");
    switch (config->distro_type) {
        case DISTRO_DESKTOP:
            printf("Desktop Edition\n");
            break;
        case DISTRO_SERVER:
            printf("Server Edition\n");
            break;
        case DISTRO_EMULATION:
            printf("Emulation Station\n");
            if (config->emu_platform != EMU_NONE) {
                printf("Platform: ");
                switch (config->emu_platform) {
                    case EMU_LIBREELEC:
                        printf("LibreELEC\n");
                        break;
                    case EMU_EMULATIONSTATION:
                        printf("EmulationStation\n");
                        break;
                    case EMU_RETROPIE:
                        printf("RetroPie\n");
                        break;
                    case EMU_LAKKA:
                        printf("Lakka\n");
                        break;
                    case EMU_ALL:
                        printf("All Platforms\n");
                        break;
                    default:
                        break;
                }
            }
            break;
        case DISTRO_MINIMAL:
            printf("Minimal System\n");
            break;
        default:
            printf("Custom\n");
            break;
    }
    
    printf("Ubuntu Version: %s (%s)\n", config->ubuntu_release, config->ubuntu_codename);
    printf("Kernel Version: %s\n", config->kernel_version);
    printf("GPU Support: %s\n", config->install_gpu_blobs ? "Enabled" : "Disabled");
    if (config->install_gpu_blobs) {
        printf("  - OpenCL: %s\n", config->enable_opencl ? "Yes" : "No");
        printf("  - Vulkan: %s\n", config->enable_vulkan ? "Yes" : "No");
    }
    printf("Image Size: %s MB\n", config->image_size);
    printf("Build Directory: %s\n", config->build_dir);
    printf("Output Directory: %s\n", config->output_dir);
    printf("\n");
    printf("Components to build:\n");
    printf("  - Kernel: %s\n", config->build_kernel ? "Yes" : "No");
    printf("  - Root filesystem: %s\n", config->build_rootfs ? "Yes" : "No");
    printf("  - U-Boot: %s\n", config->build_uboot ? "Yes" : "No");
    printf("  - System image: %s\n", config->create_image ? "Yes" : "No");
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
}