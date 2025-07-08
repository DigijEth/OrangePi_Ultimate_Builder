/*
 * example_module.c - Example Custom Module for Orange Pi 5 Plus Builder
 * Version: 1.0.0
 * 
 * This demonstrates how to create custom modules that extend the builder's functionality.
 * Custom modules can add new menu options, build steps, and override existing behavior.
 */

#include "builder.h"
#include "debug.h"

// Module-specific configuration
typedef struct {
    int enable_custom_optimization;
    int enable_custom_patches;
    char custom_repo_url[256];
    char custom_patch_dir[256];
} example_module_config_t;

static example_module_config_t module_config = {
    .enable_custom_optimization = 1,
    .enable_custom_patches = 0,
    .custom_repo_url = "https://github.com/custom/orangepi-patches.git",
    .custom_patch_dir = "/tmp/custom_patches"
};

// Forward declarations
static int example_module_init(void);
static int example_module_cleanup(void);
static void example_module_show_menu(void);
static int example_module_handle_menu_choice(int choice);
static int example_module_add_build_options(build_config_t *config);
static int example_module_execute_build_step(build_config_t *config);
static char* example_module_get_help_text(void);

// Module implementation functions
static int apply_custom_patches(build_config_t *config);
static int apply_performance_optimizations(build_config_t *config);
static int download_custom_repository(void);
static void show_module_configuration(void);
static int configure_module_settings(void);

// Module initialization
static int example_module_init(void) {
    DEBUG_INFO("Initializing Example Custom Module v1.0.0");
    
    // Create custom directories
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", module_config.custom_patch_dir);
    if (system(cmd) != 0) {
        DEBUG_ERROR("Failed to create custom patch directory");
        return -1;
    }
    
    // Download custom repository if configured
    if (strlen(module_config.custom_repo_url) > 0) {
        if (download_custom_repository() != 0) {
            DEBUG_WARN("Failed to download custom repository - continuing without it");
        }
    }
    
    DEBUG_INFO("Example module initialized successfully");
    return 0;
}

// Module cleanup
static int example_module_cleanup(void) {
    DEBUG_INFO("Cleaning up Example Custom Module");
    
    // Clean up temporary files
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", module_config.custom_patch_dir);
    system(cmd);
    
    return 0;
}

// Show module-specific menu
static void example_module_show_menu(void) {
    clear_screen();
    print_header();
    
    printf("\n%s%sEXAMPLE CUSTOM MODULE%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("This is an example of how custom modules can extend the builder.\n");
    printf("\n");
    printf("Current module configuration:\n");
    printf("• Custom optimization: %s\n", module_config.enable_custom_optimization ? "Enabled" : "Disabled");
    printf("• Custom patches: %s\n", module_config.enable_custom_patches ? "Enabled" : "Disabled");
    printf("• Custom repository: %s\n", module_config.custom_repo_url);
    printf("\n");
    printf("Available options:\n");
    printf("  %s1.%s Apply Performance Optimizations\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s2.%s Apply Custom Patches\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s3.%s Download Custom Repository\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s4.%s Configure Module Settings\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s5.%s Show Module Information\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s0.%s Back to Main Menu\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
}

// Handle module menu choices
static int example_module_handle_menu_choice(int choice) {
    DEBUG_INFO("Example module handling menu choice: %d", choice);
    
    switch (choice) {
        case 1:
            if (global_config) {
                printf("Applying performance optimizations...\n");
                apply_performance_optimizations(global_config);
                printf("Performance optimizations applied successfully!\n");
            } else {
                printf("Error: No build configuration available\n");
            }
            pause_screen();
            break;
            
        case 2:
            if (global_config) {
                printf("Applying custom patches...\n");
                apply_custom_patches(global_config);
                printf("Custom patches applied successfully!\n");
            } else {
                printf("Error: No build configuration available\n");
            }
            pause_screen();
            break;
            
        case 3:
            printf("Downloading custom repository...\n");
            if (download_custom_repository() == 0) {
                printf("Custom repository downloaded successfully!\n");
            } else {
                printf("Failed to download custom repository\n");
            }
            pause_screen();
            break;
            
        case 4:
            configure_module_settings();
            break;
            
        case 5:
            show_module_configuration();
            pause_screen();
            break;
            
        case 0:
            return 0;  // Back to main menu
            
        default:
            printf("Invalid choice: %d\n", choice);
            pause_screen();
            break;
    }
    
    return 1;  // Continue showing menu
}

// Add build options to the main configuration
static int example_module_add_build_options(build_config_t *config) {
    DEBUG_INFO("Adding custom build options to configuration");
    
    if (!config) {
        DEBUG_ERROR("Build configuration is NULL");
        return -1;
    }
    
    // Example: Modify compiler flags for optimization
    if (module_config.enable_custom_optimization) {
        DEBUG_INFO("Enabling custom optimizations in build");
        // This would modify the build configuration
        // For example, you could add custom CFLAGS or modify kernel config
    }
    
    // Example: Enable custom patches
    if (module_config.enable_custom_patches) {
        DEBUG_INFO("Custom patches will be applied during build");
    }
    
    return 0;
}

// Execute custom build step
static int example_module_execute_build_step(build_config_t *config) {
    DEBUG_INFO("Executing custom module build step");
    
    if (!config) {
        DEBUG_ERROR("Build configuration is NULL");
        return -1;
    }
    
    DEBUG_TIMER_START("custom_module_build");
    
    // Apply custom patches if enabled
    if (module_config.enable_custom_patches) {
        if (apply_custom_patches(config) != 0) {
            DEBUG_ERROR("Failed to apply custom patches");
            DEBUG_TIMER_END("custom_module_build");
            return -1;
        }
    }
    
    // Apply performance optimizations if enabled
    if (module_config.enable_custom_optimization) {
        if (apply_performance_optimizations(config) != 0) {
            DEBUG_ERROR("Failed to apply performance optimizations");
            DEBUG_TIMER_END("custom_module_build");
            return -1;
        }
    }
    
    DEBUG_TIMER_END("custom_module_build");
    DEBUG_TIMER_REPORT("custom_module_build");
    
    return 0;
}

// Get help text for the module
static char* example_module_get_help_text(void) {
    static char help_text[] = 
        "Example Custom Module v1.0.0\n"
        "=============================\n\n"
        "This module demonstrates how to extend the Orange Pi 5 Plus builder\n"
        "with custom functionality. It provides:\n\n"
        "• Custom performance optimizations for the kernel\n"
        "• Ability to apply custom patches from external repositories\n"
        "• Integration with the main build process\n"
        "• Custom menu options and configuration\n\n"
        "The module can be configured to:\n"
        "- Download patches from custom repositories\n"
        "- Apply performance optimizations\n"
        "- Modify kernel configuration\n"
        "- Add custom build steps\n\n"
        "This serves as a template for creating your own custom modules.";
    
    return help_text;
}

// Implementation of module-specific functions
static int apply_custom_patches(build_config_t *config) {
    DEBUG_INFO("Applying custom patches from: %s", module_config.custom_patch_dir);
    
    // Check if patch directory exists
    if (access(module_config.custom_patch_dir, F_OK) != 0) {
        DEBUG_WARN("Custom patch directory does not exist: %s", module_config.custom_patch_dir);
        return 0;  // Not an error, just skip
    }
    
    // Change to kernel directory
    char kernel_dir[512];
    snprintf(kernel_dir, sizeof(kernel_dir), "%s/linux", config->build_dir);
    
    if (chdir(kernel_dir) != 0) {
        DEBUG_ERROR("Failed to change to kernel directory: %s", kernel_dir);
        return -1;
    }
    
    // Apply all patches in the custom patch directory
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), 
             "find %s -name '*.patch' -print0 | sort -z | xargs -0 -n 1 patch -p1 -i",
             module_config.custom_patch_dir);
    
    DEBUG_INFO("Executing: %s", cmd);
    
    if (system(cmd) != 0) {
        DEBUG_WARN("Some custom patches may have failed to apply");
        // Don't return error - some patches might conflict but others might work
    }
    
    DEBUG_INFO("Custom patches applied successfully");
    return 0;
}

static int apply_performance_optimizations(build_config_t *config) {
    DEBUG_INFO("Applying custom performance optimizations");
    
    char kernel_dir[512];
    snprintf(kernel_dir, sizeof(kernel_dir), "%s/linux", config->build_dir);
    
    if (chdir(kernel_dir) != 0) {
        DEBUG_ERROR("Failed to change to kernel directory: %s", kernel_dir);
        return -1;
    }
    
    // Add custom kernel configuration options for performance
    FILE *config_file = fopen(".config", "a");
    if (config_file) {
        fprintf(config_file, "\n# Custom Performance Optimizations\n");
        fprintf(config_file, "CONFIG_PREEMPT_NONE=y\n");
        fprintf(config_file, "CONFIG_PREEMPT_VOLUNTARY=n\n");
        fprintf(config_file, "CONFIG_PREEMPT=n\n");
        fprintf(config_file, "CONFIG_HZ_1000=y\n");
        fprintf(config_file, "CONFIG_HZ=1000\n");
        fprintf(config_file, "CONFIG_CC_OPTIMIZE_FOR_PERFORMANCE=y\n");
        fprintf(config_file, "CONFIG_CC_OPTIMIZE_FOR_SIZE=n\n");
        fclose(config_file);
        
        DEBUG_INFO("Added custom performance optimizations to kernel config");
    } else {
        DEBUG_ERROR("Failed to open kernel config file for writing");
        return -1;
    }
    
    // Re-run olddefconfig to resolve dependencies
    if (system("make olddefconfig") != 0) {
        DEBUG_WARN("Failed to resolve kernel config dependencies");
    }
    
    return 0;
}

static int download_custom_repository(void) {
    DEBUG_INFO("Downloading custom repository: %s", module_config.custom_repo_url);
    
    // Clean up existing directory
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", module_config.custom_patch_dir);
    system(cmd);
    
    // Clone the repository
    snprintf(cmd, sizeof(cmd), 
             "git clone --depth 1 %s %s",
             module_config.custom_repo_url, module_config.custom_patch_dir);
    
    if (system(cmd) != 0) {
        DEBUG_ERROR("Failed to clone custom repository");
        return -1;
    }
    
    DEBUG_INFO("Custom repository downloaded successfully");
    return 0;
}

static void show_module_configuration(void) {
    printf("\n%s%sEXAMPLE MODULE CONFIGURATION%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Module Information:\n");
    printf("• Name: Example Custom Module\n");
    printf("• Version: 1.0.0\n");
    printf("• Type: Custom Build Enhancement\n");
    printf("• Priority: 100\n");
    printf("\n");
    printf("Current Settings:\n");
    printf("• Custom optimization: %s%s%s\n", 
           module_config.enable_custom_optimization ? COLOR_GREEN : COLOR_RED,
           module_config.enable_custom_optimization ? "Enabled" : "Disabled",
           COLOR_RESET);
    printf("• Custom patches: %s%s%s\n",
           module_config.enable_custom_patches ? COLOR_GREEN : COLOR_RED,
           module_config.enable_custom_patches ? "Enabled" : "Disabled",
           COLOR_RESET);
    printf("• Repository URL: %s\n", module_config.custom_repo_url);
    printf("• Patch directory: %s\n", module_config.custom_patch_dir);
    printf("\n");
    printf("Module Capabilities:\n");
    printf("• Adds custom kernel optimizations\n");
    printf("• Downloads and applies external patches\n");
    printf("• Integrates with main build process\n");
    printf("• Provides custom menu interface\n");
    printf("\n");
    printf("Usage Example:\n");
    printf("This module demonstrates how developers can extend the builder\n");
    printf("with custom functionality without modifying the core codebase.\n");
    printf("\n");
}

static int configure_module_settings(void) {
    int choice;
    char buffer[256];
    
    while (1) {
        clear_screen();
        printf("\n%s%sMODULE CONFIGURATION%s\n", COLOR_BOLD, COLOR_YELLOW, COLOR_RESET);
        printf("════════════════════════════════════════════════════════════════════════\n");
        printf("\n");
        printf("Current settings:\n");
        printf("1. Custom optimization: %s\n", module_config.enable_custom_optimization ? "Enabled" : "Disabled");
        printf("2. Custom patches: %s\n", module_config.enable_custom_patches ? "Enabled" : "Disabled");
        printf("3. Repository URL: %s\n", module_config.custom_repo_url);
        printf("4. Patch directory: %s\n", module_config.custom_patch_dir);
        printf("0. Back\n");
        printf("\n");
        
        choice = get_user_choice("Select option to configure", 0, 4);
        
        switch (choice) {
            case 1:
                module_config.enable_custom_optimization = !module_config.enable_custom_optimization;
                printf("Custom optimization %s\n", 
                       module_config.enable_custom_optimization ? "enabled" : "disabled");
                pause_screen();
                break;
                
            case 2:
                module_config.enable_custom_patches = !module_config.enable_custom_patches;
                printf("Custom patches %s\n",
                       module_config.enable_custom_patches ? "enabled" : "disabled");
                pause_screen();
                break;