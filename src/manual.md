# Technical Documentation - Orange Pi 5 Plus Ultimate Interactive Builder

**Version**: 0.1.0a  
**Document Version**: 1.0  
**Last Updated**: January 2024

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Core Components](#core-components)
3. [Build Process Deep Dive](#build-process-deep-dive)
4. [Module System Architecture](#module-system-architecture)
5. [Creating Custom Modules](#creating-custom-modules)
6. [Debug System Implementation](#debug-system-implementation)
7. [GPU Driver Integration](#gpu-driver-integration)
8. [Kernel Building Process](#kernel-building-process)
9. [Root Filesystem Creation](#root-filesystem-creation)
10. [Image Generation Pipeline](#image-generation-pipeline)
11. [Error Handling and Recovery](#error-handling-and-recovery)
12. [Performance Optimization](#performance-optimization)
13. [Security Considerations](#security-considerations)
14. [API Reference](#api-reference)
15. [Advanced Customization](#advanced-customization)

---

## Architecture Overview

### Design Philosophy

The Orange Pi 5 Plus Ultimate Interactive Builder is designed as a modular, extensible system that automates the complex process of creating custom Linux distributions for ARM64 single-board computers. The architecture follows several key principles:

1. **Modularity**: Each component is self-contained with well-defined interfaces
2. **Extensibility**: Custom modules can extend or override any functionality
3. **Robustness**: Comprehensive error handling and recovery mechanisms
4. **Transparency**: Detailed logging and debug capabilities
5. **User-Friendliness**: Interactive menus hide complexity while allowing full control

### System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        User Interface Layer                      │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────────┐ │
│  │   Main Menu │  │ Config Menus │  │   Progress Display    │ │
│  └─────────────┘  └──────────────┘  └───────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                                │
┌─────────────────────────────────────────────────────────────────┐
│                         Core Builder Layer                       │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────────┐ │
│  │Build Manager│  │Config Manager│  │   Module Loader       │ │
│  └─────────────┘  └──────────────┘  └───────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                                │
┌─────────────────────────────────────────────────────────────────┐
│                      Component Builder Layer                     │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────────┐ │
│  │   Kernel    │  │    RootFS    │  │    GPU Drivers        │ │
│  │   Builder   │  │   Builder    │  │    Installer          │ │
│  └─────────────┘  └──────────────┘  └───────────────────────┘ │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────────┐ │
│  │   U-Boot    │  │    Image     │  │   System Config       │ │
│  │   Builder   │  │   Creator    │  │    Manager            │ │
│  └─────────────┘  └──────────────┘  └───────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                                │
┌─────────────────────────────────────────────────────────────────┐
│                        System Services Layer                     │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────────┐ │
│  │   Logging   │  │Error Handling│  │   GitHub Auth         │ │
│  │   System    │  │  & Recovery  │  │   Manager             │ │
│  └─────────────┘  └──────────────┘  └───────────────────────┘ │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────────┐ │
│  │   Command   │  │  File System │  │   Debug & Profile     │ │
│  │  Execution  │  │  Operations  │  │   System              │ │
│  └─────────────┘  └──────────────┘  └───────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Data Flow

The build process follows a carefully orchestrated data flow:

1. **Configuration Phase**: User input → Configuration validation → Build plan generation
2. **Preparation Phase**: Dependency checking → Environment setup → Source downloading
3. **Build Phase**: Kernel compilation → RootFS creation → Driver installation
4. **Assembly Phase**: Component integration → Image creation → Verification
5. **Finalization Phase**: Cleanup → Report generation → Output delivery

---

## Core Components

### 1. Main Controller (`builder.c`)

The main controller serves as the central orchestrator, managing:

#### State Management
```c
typedef struct {
    build_config_t config;      // Current build configuration
    build_state_t state;        // Current build state
    error_stack_t errors;       // Error tracking
    module_list_t modules;      // Loaded modules
} application_state_t;
```

#### Event Loop
The main event loop processes user input and coordinates component actions:

```c
while (running) {
    event = get_next_event();
    
    switch (event.type) {
        case EVENT_MENU_SELECTION:
            handle_menu_selection(event.data);
            break;
        case EVENT_BUILD_START:
            initiate_build_sequence();
            break;
        case EVENT_MODULE_HOOK:
            execute_module_hooks(event.hook_point);
            break;
        case EVENT_ERROR:
            handle_error_condition(event.error);
            break;
    }
}
```

### 2. System Services (`system.c`)

Provides low-level system operations with safety wrappers:

#### Command Execution Engine
```c
typedef struct {
    char command[MAX_CMD_LEN];
    int timeout;
    int retry_count;
    int capture_output;
    char *output_buffer;
    size_t output_size;
} command_context_t;

int execute_command_advanced(command_context_t *ctx) {
    // Pre-execution hooks
    if (debug_enabled) {
        log_command_execution(ctx->command);
    }
    
    // Setup execution environment
    setup_signal_handlers();
    configure_process_limits();
    
    // Execute with monitoring
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        setup_command_environment();
        exec_with_timeout(ctx);
    } else {
        // Parent process
        monitor_child_process(pid, ctx);
    }
    
    // Post-execution processing
    process_command_output(ctx);
    check_error_conditions(ctx);
    
    return ctx->exit_code;
}
```

#### GitHub Authentication System
```c
typedef struct {
    char token[GITHUB_TOKEN_MAX_LEN];
    time_t expiry;
    int rate_limit_remaining;
    time_t rate_limit_reset;
} github_auth_state_t;

char* authenticate_github_url(const char *url) {
    github_auth_state_t *auth = get_auth_state();
    
    // Check token validity
    if (!validate_token(auth)) {
        refresh_token(auth);
    }
    
    // Apply rate limiting
    if (auth->rate_limit_remaining <= 0) {
        wait_for_rate_limit_reset(auth);
    }
    
    // Construct authenticated URL
    return construct_authenticated_url(url, auth->token);
}
```

### 3. User Interface (`ui.c`)

Implements a sophisticated menu system with state tracking:

#### Menu State Machine
```c
typedef struct menu_node {
    char *title;
    menu_item_t *items;
    int item_count;
    struct menu_node *parent;
    void (*on_enter)(void);
    void (*on_exit)(void);
    int (*handle_input)(int choice);
} menu_node_t;

typedef struct {
    menu_node_t *current;
    menu_stack_t history;
    int refresh_needed;
    render_context_t render_ctx;
} menu_system_t;
```

#### Dynamic Menu Generation
```c
void generate_dynamic_menu(menu_node_t *menu) {
    // Clear existing items
    clear_menu_items(menu);
    
    // Add static items
    add_static_menu_items(menu);
    
    // Query modules for menu contributions
    module_list_t *modules = get_loaded_modules();
    for (module_t *mod = modules->head; mod; mod = mod->next) {
        if (mod->contribute_menu_items) {
            menu_contribution_t *contrib = mod->contribute_menu_items();
            integrate_menu_contribution(menu, contrib);
        }
    }
    
    // Sort and organize items
    organize_menu_items(menu);
}
```

### 4. Kernel Builder (`kernel.c`)

Manages the complex kernel build process:

#### Source Management
```c
typedef struct {
    char *repo_url;
    char *branch;
    char *commit;
    patch_list_t *patches;
    config_overlay_t *config_changes;
} kernel_source_config_t;

int prepare_kernel_source(kernel_source_config_t *config) {
    // Clone or update repository
    if (!repository_exists(config->repo_url)) {
        clone_repository(config->repo_url, config->branch);
    } else {
        update_repository(config->repo_url, config->branch);
    }
    
    // Checkout specific commit if specified
    if (config->commit) {
        checkout_commit(config->commit);
    }
    
    // Apply patch series
    for (patch_t *patch = config->patches->head; patch; patch = patch->next) {
        apply_patch(patch);
    }
    
    return SUCCESS;
}
```

#### Configuration Management
```c
typedef struct {
    hashtable_t *base_config;
    hashtable_t *board_config;
    hashtable_t *user_config;
    hashtable_t *module_config;
} kernel_config_layers_t;

void build_kernel_configuration(kernel_config_layers_t *layers) {
    // Start with defconfig
    load_defconfig(layers->base_config);
    
    // Apply board-specific settings
    merge_config_layer(layers->base_config, layers->board_config);
    
    // Apply user customizations
    merge_config_layer(layers->base_config, layers->user_config);
    
    // Allow modules to modify
    for (module_t *mod = modules->head; mod; mod = mod->next) {
        if (mod->modify_kernel_config) {
            mod->modify_kernel_config(layers->base_config);
        }
    }
    
    // Validate final configuration
    validate_kernel_config(layers->base_config);
}
```

### 5. GPU Driver Manager (`gpu.c`)

Handles Mali GPU driver integration:

#### Driver Package Management
```c
typedef struct {
    char *name;
    char *version;
    char *url;
    char *sha256;
    driver_type_t type;
    dependency_list_t *deps;
} driver_package_t;

typedef struct {
    driver_package_t *drivers;
    int driver_count;
    char *firmware_path;
    char *library_path;
} mali_driver_config_t;
```

#### Installation Process
```c
int install_mali_stack(mali_driver_config_t *config) {
    // Phase 1: Download and verify
    for (int i = 0; i < config->driver_count; i++) {
        driver_package_t *pkg = &config->drivers[i];
        
        // Download with retry logic
        download_file_with_retry(pkg->url, pkg->name);
        
        // Verify integrity
        if (!verify_sha256(pkg->name, pkg->sha256)) {
            return ERROR_INTEGRITY_FAILED;
        }
    }
    
    // Phase 2: Install firmware
    install_firmware_files(config->firmware_path);
    
    // Phase 3: Install libraries
    install_driver_libraries(config->library_path);
    
    // Phase 4: Configure system
    configure_mali_environment();
    create_system_links();
    update_loader_cache();
    
    // Phase 5: Verify installation
    return verify_mali_installation();
}
```

---

## Build Process Deep Dive

### Phase 1: Initialization and Validation

```c
typedef struct {
    checklist_t *prereq_checks;
    validation_result_t *config_validation;
    resource_estimate_t *resource_needs;
} preflight_check_t;

int perform_preflight_checks(build_config_t *config, preflight_check_t *checks) {
    // System requirements
    check_root_permissions(checks->prereq_checks);
    check_disk_space(checks->prereq_checks, config);
    check_memory_available(checks->prereq_checks);
    check_network_connectivity(checks->prereq_checks);
    
    // Tool availability
    check_required_tools(checks->prereq_checks);
    check_cross_compiler(checks->prereq_checks, config);
    
    // Configuration validation
    validate_ubuntu_version(config, checks->config_validation);
    validate_kernel_version(config, checks->config_validation);
    validate_output_paths(config, checks->config_validation);
    
    // Resource estimation
    estimate_build_time(config, checks->resource_needs);
    estimate_disk_usage(config, checks->resource_needs);
    estimate_download_size(config, checks->resource_needs);
    
    return aggregate_check_results(checks);
}
```

### Phase 2: Source Acquisition

```c
typedef struct {
    source_list_t *sources;
    download_queue_t *queue;
    progress_tracker_t *progress;
} source_manager_t;

int acquire_all_sources(build_config_t *config, source_manager_t *mgr) {
    // Build source list
    add_kernel_sources(mgr->sources, config);
    add_uboot_sources(mgr->sources, config);
    add_firmware_sources(mgr->sources, config);
    add_rootfs_packages(mgr->sources, config);
    
    // Optimize download order
    optimize_download_queue(mgr->queue, mgr->sources);
    
    // Parallel download with progress tracking
    parallel_download_manager_t *dl_mgr = create_download_manager();
    
    for (source_t *src = mgr->sources->head; src; src = src->next) {
        download_task_t *task = create_download_task(src);
        submit_download_task(dl_mgr, task);
    }
    
    // Wait for completion with progress updates
    while (!all_downloads_complete(dl_mgr)) {
        update_progress_display(mgr->progress, dl_mgr);
        handle_download_errors(dl_mgr);
        sleep_ms(100);
    }
    
    return verify_all_sources(mgr->sources);
}
```

### Phase 3: Kernel Building

```c
typedef struct {
    build_environment_t *env;
    compilation_flags_t *flags;
    parallel_job_manager_t *job_mgr;
} kernel_build_context_t;

int build_kernel_complete(build_config_t *config, kernel_build_context_t *ctx) {
    // Setup build environment
    setup_cross_compile_env(ctx->env, config);
    configure_build_flags(ctx->flags, config);
    
    // Configure kernel
    int result = configure_kernel_interactive(config);
    if (result != SUCCESS) return result;
    
    // Build kernel image
    result = compile_kernel_image(ctx);
    if (result != SUCCESS) return result;
    
    // Build modules
    result = compile_kernel_modules(ctx);
    if (result != SUCCESS) return result;
    
    // Build device tree
    result = compile_device_trees(ctx);
    if (result != SUCCESS) return result;
    
    // Package results
    return package_kernel_artifacts(config, ctx);
}
```

### Phase 4: Root Filesystem Creation

```c
typedef struct {
    debootstrap_config_t *debootstrap;
    package_manager_t *pkg_mgr;
    customization_list_t *customizations;
} rootfs_builder_t;

int create_ubuntu_rootfs(build_config_t *config, rootfs_builder_t *builder) {
    // Stage 1: Bootstrap base system
    debootstrap_stage1(builder->debootstrap, config);
    
    // Stage 2: Configure package sources
    configure_apt_sources(builder->pkg_mgr, config);
    
    // Stage 3: Install packages
    install_base_packages(builder->pkg_mgr);
    install_kernel_packages(builder->pkg_mgr, config);
    install_firmware_packages(builder->pkg_mgr);
    install_user_packages(builder->pkg_mgr, config);
    
    // Stage 4: System configuration
    configure_users(config);
    configure_networking(config);
    configure_services(config);
    configure_boot(config);
    
    // Stage 5: Custom modifications
    apply_customizations(builder->customizations, config);
    
    // Stage 6: Cleanup
    cleanup_rootfs(config);
    
    return SUCCESS;
}
```

### Phase 5: Image Assembly

```c
typedef struct {
    partition_layout_t *layout;
    filesystem_list_t *filesystems;
    bootloader_config_t *bootloader;
} image_builder_t;

int assemble_system_image(build_config_t *config, image_builder_t *builder) {
    // Create image file
    create_sparse_image(config->output_image, config->image_size);
    
    // Setup loop device
    loop_device_t *loop = setup_loop_device(config->output_image);
    
    // Create partitions
    create_partition_table(loop, builder->layout);
    
    // Format filesystems
    for (filesystem_t *fs = builder->filesystems->head; fs; fs = fs->next) {
        format_partition(loop, fs);
    }
    
    // Mount filesystems
    mount_hierarchy_t *mounts = mount_partitions(loop, builder->layout);
    
    // Copy rootfs
    sync_rootfs_to_image(config->rootfs_path, mounts->root);
    
    // Install bootloader
    install_bootloader(loop, builder->bootloader);
    
    // Cleanup
    unmount_hierarchy(mounts);
    detach_loop_device(loop);
    
    return verify_image(config->output_image);
}
```

---

## Module System Architecture

### Module Structure

The module system allows extending the builder without modifying core code:

```c
typedef struct custom_module {
    // Module metadata
    char name[64];
    char version[16];
    char description[256];
    char author[64];
    char license[32];
    
    // Module type and priority
    module_type_t type;
    int priority;  // Higher = loaded later, can override
    
    // Dependencies
    char **required_modules;
    int required_module_count;
    char **conflicting_modules;
    int conflicting_module_count;
    
    // Lifecycle callbacks
    int (*init_module)(module_context_t *ctx);
    int (*cleanup_module)(void);
    
    // Feature callbacks
    void (*contribute_menu_items)(menu_contribution_t *contrib);
    int (*handle_menu_selection)(int menu_id);
    int (*modify_build_config)(build_config_t *config);
    int (*pre_build_hook)(build_context_t *ctx);
    int (*post_build_hook)(build_context_t *ctx);
    int (*build_step_hook)(const char *step, build_context_t *ctx);
    
    // Configuration
    void* (*get_config_schema)(void);
    int (*validate_config)(void *config);
    int (*apply_config)(void *config);
    
    // State
    void *private_data;
    module_state_t state;
    
    // List linkage
    struct custom_module *next;
} custom_module_t;
```

### Module Loading Process

```c
int load_module_from_file(const char *module_path) {
    // Step 1: Load shared library
    void *handle = dlopen(module_path, RTLD_LAZY);
    if (!handle) {
        log_error("Failed to load module: %s", dlerror());
        return ERROR_MODULE_LOAD_FAILED;
    }
    
    // Step 2: Get module descriptor
    custom_module_t* (*get_module)(void) = dlsym(handle, "get_module_descriptor");
    if (!get_module) {
        dlclose(handle);
        return ERROR_INVALID_MODULE;
    }
    
    custom_module_t *module = get_module();
    
    // Step 3: Validate module
    if (!validate_module_descriptor(module)) {
        dlclose(handle);
        return ERROR_INVALID_MODULE;
    }
    
    // Step 4: Check dependencies
    if (!check_module_dependencies(module)) {
        dlclose(handle);
        return ERROR_DEPENDENCY_FAILED;
    }
    
    // Step 5: Check for conflicts
    if (check_module_conflicts(module)) {
        dlclose(handle);
        return ERROR_MODULE_CONFLICT;
    }
    
    // Step 6: Initialize module
    module_context_t ctx = create_module_context(module);
    if (module->init_module && module->init_module(&ctx) != SUCCESS) {
        dlclose(handle);
        return ERROR_MODULE_INIT_FAILED;
    }
    
    // Step 7: Register module
    register_loaded_module(module, handle);
    
    return SUCCESS;
}
```

### Module Hook System

```c
typedef struct {
    char *hook_name;
    hook_priority_t priority;
    module_hook_fn callback;
    void *user_data;
} hook_registration_t;

typedef struct {
    hashtable_t *hooks;  // hook_name -> sorted list of registrations
} hook_manager_t;

int execute_hooks(const char *hook_name, void *data) {
    hook_manager_t *mgr = get_hook_manager();
    
    // Get registered hooks
    hook_list_t *hooks = hashtable_get(mgr->hooks, hook_name);
    if (!hooks) return SUCCESS;
    
    // Sort by priority
    sort_hooks_by_priority(hooks);
    
    // Execute each hook
    for (hook_registration_t *reg = hooks->head; reg; reg = reg->next) {
        hook_result_t result = reg->callback(data, reg->user_data);
        
        switch (result) {
            case HOOK_CONTINUE:
                continue;
            case HOOK_STOP:
                return SUCCESS;
            case HOOK_ERROR:
                return ERROR_HOOK_FAILED;
        }
    }
    
    return SUCCESS;
}
```

---

## Creating Custom Modules

### Basic Module Template

```c
// my_custom_module.c
#include "builder.h"
#include "modules/module_api.h"

// Module configuration structure
typedef struct {
    bool enable_feature_x;
    char custom_repo[256];
    int optimization_level;
} my_module_config_t;

// Module state
static struct {
    my_module_config_t config;
    bool initialized;
    void *private_data;
} module_state = {
    .config = {
        .enable_feature_x = true,
        .custom_repo = "https://github.com/myrepo/patches.git",
        .optimization_level = 2
    },
    .initialized = false,
    .private_data = NULL
};

// Module descriptor
static custom_module_t module_descriptor = {
    .name = "My Custom Module",
    .version = "1.0.0",
    .description = "Adds custom features to Orange Pi builder",
    .author = "Your Name",
    .license = "GPL-3.0",
    .type = MODULE_TYPE_BUILD,
    .priority = 100,
    
    // Define callbacks
    .init_module = my_module_init,
    .cleanup_module = my_module_cleanup,
    .contribute_menu_items = my_module_menu_items,
    .handle_menu_selection = my_module_handle_menu,
    .modify_build_config = my_module_modify_config,
    .pre_build_hook = my_module_pre_build,
    .post_build_hook = my_module_post_build,
    .build_step_hook = my_module_build_step
};

// Module initialization
static int my_module_init(module_context_t *ctx) {
    LOG_INFO("Initializing %s v%s", module_descriptor.name, module_descriptor.version);
    
    // Load configuration
    if (load_module_config(&module_state.config) != SUCCESS) {
        LOG_WARN("Using default configuration");
    }
    
    // Initialize resources
    module_state.private_data = allocate_resources();
    if (!module_state.private_data) {
        return ERROR_INIT_FAILED;
    }
    
    // Register hooks
    register_hook("kernel.pre_config", my_kernel_config_hook, PRIORITY_NORMAL);
    register_hook("rootfs.post_install", my_rootfs_hook, PRIORITY_HIGH);
    
    module_state.initialized = true;
    return SUCCESS;
}

// Menu contribution
static void my_module_menu_items(menu_contribution_t *contrib) {
    // Add main menu item
    add_menu_item(contrib, &(menu_item_t){
        .id = 1001,
        .text = "My Module Options",
        .type = MENU_SUBMENU,
        .enabled = true
    });
    
    // Add submenu items
    add_submenu_item(contrib, 1001, &(menu_item_t){
        .id = 1002,
        .text = "Configure Feature X",
        .type = MENU_ACTION,
        .enabled = module_state.config.enable_feature_x
    });
}

// Build configuration modification
static int my_module_modify_config(build_config_t *config) {
    if (!module_state.initialized) return ERROR_NOT_INITIALIZED;
    
    // Add custom kernel config options
    if (module_state.config.enable_feature_x) {
        add_kernel_config(config, "CONFIG_MY_FEATURE_X=y");
        add_kernel_config(config, "CONFIG_MY_OPTIMIZATION=y");
    }
    
    // Modify build flags
    if (module_state.config.optimization_level > 0) {
        char flag[32];
        snprintf(flag, sizeof(flag), "-O%d", module_state.config.optimization_level);
        add_build_flag(config, flag);
    }
    
    return SUCCESS;
}

// Pre-build hook
static int my_module_pre_build(build_context_t *ctx) {
    LOG_INFO("Executing pre-build tasks for %s", module_descriptor.name);
    
    // Download custom patches
    if (strlen(module_state.config.custom_repo) > 0) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "git clone --depth 1 %s /tmp/my_patches",
                 module_state.config.custom_repo);
        
        if (execute_command(cmd) != SUCCESS) {
            LOG_WARN("Failed to download custom patches");
        }
    }
    
    return SUCCESS;
}

// Module entry point
custom_module_t* get_module_descriptor(void) {
    return &module_descriptor;
}
```

### Advanced Module Features

#### 1. Configuration Schema

```c
static config_schema_t* get_config_schema(void) {
    static config_schema_t schema = {
        .version = 1,
        .fields = {
            {
                .name = "enable_feature_x",
                .type = CONFIG_TYPE_BOOL,
                .default_value = "true",
                .description = "Enable experimental feature X"
            },
            {
                .name = "custom_repo",
                .type = CONFIG_TYPE_STRING,
                .max_length = 256,
                .validator = validate_url,
                .description = "Custom patch repository URL"
            },
            {
                .name = "optimization_level",
                .type = CONFIG_TYPE_INT,
                .min_value = 0,
                .max_value = 3,
                .default_value = "2",
                .description = "Compiler optimization level"
            },
            { NULL } // Terminator
        }
    };
    
    return &schema;
}
```

#### 2. Dynamic Menu Generation

```c
static void contribute_dynamic_menus(menu_contribution_t *contrib) {
    // Query available features
    feature_list_t *features = discover_features();
    
    int menu_id = 2000;
    for (feature_t *feat = features->head; feat; feat = feat->next) {
        menu_item_t item = {
            .id = menu_id++,
            .text = feat->display_name,
            .type = MENU_TOGGLE,
            .enabled = feat->available,
            .data = feat
        };
        
        add_menu_item(contrib, &item);
    }
}
```

#### 3. Build Step Injection

```c
static int inject_custom_build_steps(build_pipeline_t *pipeline) {
    // Add step after kernel build
    build_step_t *custom_step = create_build_step(
        "Apply Custom Patches",
        apply_custom_patches,
        rollback_custom_patches
    );
    
    insert_build_step_after(pipeline, "kernel.build", custom_step);
    
    // Add parallel step
    build_step_t *parallel_step = create_build_step(
        "Build Custom Components",
        build_custom_components,
        NULL
    );
    
    add_parallel_build_step(pipeline, "rootfs.packages", parallel_step);
    
    return SUCCESS;
}
```

#### 4. Event Handling

```c
static int handle_build_events(build_event_t *event) {
    switch (event->type) {
        case EVENT_KERNEL_CONFIGURED:
            return on_kernel_configured(event->data);
            
        case EVENT_PACKAGE_INSTALLED:
            return on_package_installed(event->data);
            
        case EVENT_BUILD_FAILED:
            return on_build_failed(event->data);
            
        default:
            return SUCCESS;
    }
}
```

### Module Best Practices

#### 1. Error Handling

```c
static int robust_operation(void) {
    int result;
    error_context_t *err_ctx = NULL;
    
    // Create error context
    err_ctx = create_error_context();
    
    // Perform operation with proper cleanup
    result = risky_operation();
    if (result != SUCCESS) {
        set_error_context(err_ctx, result, "Operation failed: %s", 
                         get_error_string(result));
        goto cleanup;
    }
    
    // More operations...
    
cleanup:
    if (err_ctx && has_error(err_ctx)) {
        log_error_context(err_ctx);
        cleanup_resources();
    }
    
    free_error_context(err_ctx);
    return result;
}
```

#### 2. Resource Management

```c
typedef struct {
    void *resource;
    void (*cleanup)(void *);
} managed_resource_t;

static resource_manager_t* create_resource_manager(void) {
    resource_manager_t *mgr = calloc(1, sizeof(resource_manager_t));
    mgr->resources = create_list();
    return mgr;
}

static void* allocate_managed_resource(resource_manager_t *mgr, 
                                      size_t size, 
                                      void (*cleanup)(void*)) {
    void *resource = malloc(size);
    if (!resource) return NULL;
    
    managed_resource_t *managed = malloc(sizeof(managed_resource_t));
    managed->resource = resource;
    managed->cleanup = cleanup;
    
    list_append(mgr->resources, managed);
    return resource;
}

static void cleanup_all_resources(resource_manager_t *mgr) {
    for (node_t *node = mgr->resources->head; node; node = node->next) {
        managed_resource_t *res = node->data;
        if (res->cleanup) {
            res->cleanup(res->resource);
        }
        free(res->resource);
        free(res);
    }
    
    free_list(mgr->resources);
    free(mgr);
}
```

#### 3. Thread Safety

```c
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    void *shared_data;
    int ref_count;
} thread_safe_resource_t;

static thread_safe_resource_t* get_shared_resource(void) {
    static thread_safe_resource_t *resource = NULL;
    static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
    
    pthread_mutex_lock(&init_lock);
    if (!resource) {
        resource = create_thread_safe_resource();
    }
    resource->ref_count++;
    pthread_mutex_unlock(&init_lock);
    
    return resource;
}

static void use_shared_resource(thread_safe_resource_t *res) {
    pthread_mutex_lock(&res->lock);
    
    // Critical section
    modify_shared_data(res->shared_data);
    
    pthread_mutex_unlock(&res->lock);
}
```

---

## Debug System Implementation

### Debug Architecture

The debug system provides comprehensive development and troubleshooting capabilities:

```c
typedef struct {
    debug_level_t level;
    output_config_t output;
    filter_list_t *filters;
    handler_list_t *handlers;
} debug_config_t;

typedef struct {
    uint64_t id;
    timestamp_t time;
    debug_level_t level;
    const char *file;
    int line;
    const char *function;
    thread_id_t thread;
    char message[DEBUG_MSG_MAX];
    void *context;
} debug_event_t;
```

### Performance Profiling

```c
typedef struct {
    char name[64];
    uint64_t call_count;
    uint64_t total_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    uint64_t last_time_ns;
    call_stack_t *stack;
} profile_entry_t;

static void profile_function_enter(const char *name) {
    profile_entry_t *entry = get_or_create_profile_entry(name);
    
    // Record entry time
    entry->stack = push_call_stack(entry->stack, get_timestamp_ns());
    entry->call_count++;
}

static void profile_function_exit(const char *name) {
    profile_entry_t *entry = get_profile_entry(name);
    if (!entry || !entry->stack) return;
    
    // Calculate duration
    uint64_t enter_time = pop_call_stack(&entry->stack);
    uint64_t duration = get_timestamp_ns() - enter_time;
    
    // Update statistics
    entry->total_time_ns += duration;
    entry->last_time_ns = duration;
    
    if (duration < entry->min_time_ns || entry->min_time_ns == 0) {
        entry->min_time_ns = duration;
    }
    
    if (duration > entry->max_time_ns) {
        entry->max_time_ns = duration;
    }
}
```

### Memory Tracking

```c
typedef struct allocation {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    const char *function;
    uint64_t timestamp;
    call_stack_t *stack_trace;
    struct allocation *next;
    struct allocation *prev;
} allocation_t;

static void* debug_malloc(size_t size, const char *file, int line, const char *func) {
    void *ptr = malloc(size + sizeof(allocation_header_t));
    if (!ptr) return NULL;
    
    // Add header
    allocation_header_t *header = ptr;
    header->magic = ALLOC_MAGIC;
    header->size = size;
    
    // Track allocation
    allocation_t *alloc = create_allocation_record(
        ptr + sizeof(allocation_header_t), 
        size, file, line, func
    );
    
    add_allocation(alloc);
    
    return ptr + sizeof(allocation_header_t);
}

static void check_memory_integrity(void) {
    for (allocation_t *alloc = allocations_head; alloc; alloc = alloc->next) {
        allocation_header_t *header = alloc->ptr - sizeof(allocation_header_t);
        
        if (header->magic != ALLOC_MAGIC) {
            report_corruption(alloc, "Header corruption detected");
        }
        
        // Check for buffer overrun
        unsigned char *end_marker = alloc->ptr + alloc->size;
        if (*end_marker != END_MARKER) {
            report_corruption(alloc, "Buffer overrun detected");
        }
    }
}
```

---

## GPU Driver Integration

### Mali Driver Architecture

```c
typedef struct {
    driver_variant_t variant;      // CSF, Midgard, Bifrost
    version_info_t version;        // Driver version details
    capability_flags_t caps;       // Supported features
    api_support_t apis;           // OpenGL ES, Vulkan, OpenCL
} mali_driver_info_t;

typedef struct {
    char *library_path;
    char *firmware_path;
    char *config_path;
    symlink_map_t *symlinks;
    environment_vars_t *env_vars;
} mali_installation_paths_t;
```

### Installation Process

```c
static int install_mali_complete(mali_config_t *config) {
    installation_context_t *ctx = create_installation_context(config);
    
    // Phase 1: System preparation
    prepare_system_for_mali(ctx);
    
    // Phase 2: Remove conflicting drivers
    remove_conflicting_drivers(ctx);
    
    // Phase 3: Install firmware
    install_mali_firmware(ctx);
    
    // Phase 4: Install userspace libraries
    install_mali_libraries(ctx);
    
    // Phase 5: Configure APIs
    if (config->enable_gles) configure_gles(ctx);
    if (config->enable_vulkan) configure_vulkan(ctx);
    if (config->enable_opencl) configure_opencl(ctx);
    
    // Phase 6: System integration
    integrate_with_system(ctx);
    
    // Phase 7: Verification
    verify_installation(ctx);
    
    cleanup_installation_context(ctx);
    return ctx->result;
}
```

### API Configuration

#### OpenCL Setup
```c
static int configure_opencl(installation_context_t *ctx) {
    // Create ICD file
    icd_config_t *icd = create_icd_config();
    icd->vendor = "ARM";
    icd->library = ctx->paths->library_path;
    icd->version = "2.2";
    
    write_icd_file("/etc/OpenCL/vendors/mali.icd", icd);
    
    // Set environment
    setenv("OCL_ICD_VENDORS", "/etc/OpenCL/vendors", 1);
    setenv("MALI_OPENCL_VERSION", "220", 1);
    
    // Create test program
    create_opencl_test_program("/usr/local/bin/test-opencl");
    
    return SUCCESS;
}
```

#### Vulkan Setup
```c
static int configure_vulkan(installation_context_t *ctx) {
    // Create Vulkan ICD JSON
    vulkan_icd_t *icd = create_vulkan_icd();
    icd->file_format_version = "1.0.0";
    icd->library_path = ctx->paths->library_path;
    icd->api_version = "1.2.0";
    icd->implementation_version = "1";
    icd->description = "ARM Mali Vulkan Driver";
    
    write_vulkan_icd("/usr/share/vulkan/icd.d/mali_icd.aarch64.json", icd);
    
    // Configure layers
    configure_vulkan_layers(ctx);
    
    return SUCCESS;
}
```

---

## Kernel Building Process

### Multi-Source Support

The kernel builder supports multiple source strategies:

```c
typedef enum {
    KERNEL_SOURCE_MAINLINE,
    KERNEL_SOURCE_STABLE,
    KERNEL_SOURCE_ROCKCHIP,
    KERNEL_SOURCE_ORANGEPI,
    KERNEL_SOURCE_CUSTOM
} kernel_source_type_t;

static kernel_source_strategy_t* select_kernel_strategy(build_config_t *config) {
    switch (determine_kernel_source_type(config)) {
        case KERNEL_SOURCE_MAINLINE:
            return &mainline_kernel_strategy;
        case KERNEL_SOURCE_ROCKCHIP:
            return &rockchip_kernel_strategy;
        case KERNEL_SOURCE_ORANGEPI:
            return &orangepi_kernel_strategy;
        default:
            return &generic_kernel_strategy;
    }
}
```

### Patch Management

```c
typedef struct {
    char *name;
    char *description;
    patch_type_t type;
    dependency_list_t *deps;
    conflict_list_t *conflicts;
    int (*test_applicable)(kernel_version_t *);
    int (*apply)(const char *kernel_path);
    int (*revert)(const char *kernel_path);
} kernel_patch_t;

static int apply_patch_series(patch_series_t *series, const char *kernel_path) {
    // Create checkpoint
    checkpoint_t *checkpoint = create_kernel_checkpoint(kernel_path);
    
    // Apply patches in order
    for (patch_t *patch = series->head; patch; patch = patch->next) {
        if (!test_patch_applicable(patch, kernel_path)) {
            LOG_INFO("Skipping non-applicable patch: %s", patch->name);
            continue;
        }
        
        LOG_INFO("Applying patch: %s", patch->name);
        int result = apply_patch(patch, kernel_path);
        
        if (result != SUCCESS) {
            LOG_ERROR("Patch failed: %s", patch->name);
            
            if (series->fail_strategy == PATCH_FAIL_ABORT) {
                restore_checkpoint(checkpoint);
                return ERROR_PATCH_FAILED;
            }
        }
    }
    
    free_checkpoint(checkpoint);
    return SUCCESS;
}
```

### Configuration Layer System

```c
typedef struct {
    char *name;
    int priority;
    config_map_t *configs;
    int (*validator)(config_map_t *);
} config_layer_t;

static void build_final_kernel_config(kernel_config_builder_t *builder) {
    // Base layer: defconfig
    apply_config_layer(builder, get_defconfig_layer());
    
    // Hardware layer: SoC-specific
    apply_config_layer(builder, get_soc_config_layer());
    
    // Board layer: Board-specific
    apply_config_layer(builder, get_board_config_layer());
    
    // Feature layers
    if (builder->config->enable_debug) {
        apply_config_layer(builder, get_debug_config_layer());
    }
    
    if (builder->config->enable_performance) {
        apply_config_layer(builder, get_performance_config_layer());
    }
    
    // User layer: User customizations
    apply_config_layer(builder, get_user_config_layer());
    
    // Module layers: From loaded modules
    apply_module_config_layers(builder);
    
    // Validate final configuration
    validate_final_config(builder);
}
```

---

## Root Filesystem Creation

### Package Management Integration

```c
typedef struct {
    package_source_t *sources;
    package_cache_t *cache;
    dependency_resolver_t *resolver;
    conflict_detector_t *conflicts;
} package_manager_t;

static int install_package_set(package_manager_t *pm, package_set_t *set) {
    // Resolve dependencies
    package_list_t *to_install = resolve_dependencies(pm->resolver, set);
    
    // Check for conflicts
    conflict_list_t *conflicts = detect_conflicts(pm->conflicts, to_install);
    if (conflicts->count > 0) {
        return handle_conflicts(conflicts);
    }
    
    // Download packages
    download_packages(pm->cache, to_install);
    
    // Install in correct order
    package_list_t *ordered = topological_sort(to_install);
    for (package_t *pkg = ordered->head; pkg; pkg = pkg->next) {
        install_package(pkg);
    }
    
    return SUCCESS;
}
```

### Service Configuration

```c
typedef struct {
    char *name;
    service_type_t type;
    char *exec_start;
    char *exec_stop;
    dependency_list_t *deps;
    environment_t *env;
    restart_policy_t restart;
} service_config_t;

static int configure_system_services(rootfs_context_t *ctx) {
    service_manager_t *svc_mgr = create_service_manager(ctx);
    
    // Core services
    configure_service(svc_mgr, &network_service_config);
    configure_service(svc_mgr, &ssh_service_config);
    configure_service(svc_mgr, &time_sync_service_config);
    
    // Distribution-specific services
    switch (ctx->config->distro_type) {
        case DISTRO_DESKTOP:
            configure_desktop_services(svc_mgr);
            break;
        case DISTRO_SERVER:
            configure_server_services(svc_mgr);
            break;
        case DISTRO_EMULATION:
            configure_emulation_services(svc_mgr);
            break;
    }
    
    // Enable services
    enable_configured_services(svc_mgr);
    
    return SUCCESS;
}
```

---

## Image Generation Pipeline

### Partition Layout Management

```c
typedef struct {
    partition_type_t type;
    uint64_t start_sector;
    uint64_t size_sectors;
    filesystem_type_t fs_type;
    mount_point_t mount;
    partition_flags_t flags;
} partition_def_t;

static partition_layout_t* create_partition_layout(image_config_t *config) {
    partition_layout_t *layout = allocate_partition_layout();
    
    // Bootloader partition
    add_partition(layout, &(partition_def_t){
        .type = PART_TYPE_BOOTLOADER,
        .start_sector = 64,
        .size_sectors = 16384,  // 8MB
        .fs_type = FS_TYPE_NONE,
        .flags = PART_FLAG_BOOTABLE
    });
    
    // Boot partition
    add_partition(layout, &(partition_def_t){
        .type = PART_TYPE_BOOT,
        .start_sector = 16448,
        .size_sectors = 524288,  // 256MB
        .fs_type = FS_TYPE_FAT32,
        .mount = "/boot",
        .flags = PART_FLAG_NONE
    });
    
    // Root partition
    add_partition(layout, &(partition_def_t){
        .type = PART_TYPE_ROOT,
        .start_sector = 540736,
        .size_sectors = config->image_size_mb * 2048 - 540736,
        .fs_type = FS_TYPE_EXT4,
        .mount = "/",
        .flags = PART_FLAG_NONE
    });
    
    return layout;
}
```

### Bootloader Integration

```c
static int install_bootloader_chain(image_context_t *ctx) {
    // Stage 1: TPL (Tertiary Program Loader)
    install_tpl(ctx, "/usr/share/u-boot/tpl.bin");
    
    // Stage 2: SPL (Secondary Program Loader)
    install_spl(ctx, "/usr/share/u-boot/spl.bin");
    
    // Stage 3: U-Boot proper
    install_uboot(ctx, "/usr/share/u-boot/u-boot.itb");
    
    // Stage 4: Boot configuration
    create_boot_config(ctx, &(boot_config_t){
        .kernel_path = "/boot/vmlinuz",
        .initrd_path = "/boot/initrd.img",
        .dtb_path = "/boot/dtbs/rockchip/rk3588-orangepi-5-plus.dtb",
        .cmdline = "console=ttyS2,1500000 root=/dev/mmcblk0p3 rw rootwait"
    });
    
    return SUCCESS;
}
```

---

## Error Handling and Recovery

### Error Context System

```c
typedef struct error_frame {
    error_code_t code;
    char message[256];
    char file[64];
    int line;
    char function[64];
    timestamp_t time;
    struct error_frame *next;
} error_frame_t;

typedef struct {
    error_frame_t *stack;
    int depth;
    error_handler_fn *handlers;
    int handler_count;
} error_context_t;

static void push_error(error_context_t *ctx, error_code_t code, 
                      const char *fmt, ...) {
    error_frame_t *frame = allocate_error_frame();
    
    frame->code = code;
    frame->time = get_timestamp();
    
    va_list args;
    va_start(args, fmt);
    vsnprintf(frame->message, sizeof(frame->message), fmt, args);
    va_end(args);
    
    // Get caller information
    get_caller_info(&frame->file, &frame->line, &frame->function);
    
    // Push to stack
    frame->next = ctx->stack;
    ctx->stack = frame;
    ctx->depth++;
    
    // Trigger handlers
    trigger_error_handlers(ctx, frame);
}
```

### Recovery Strategies

```c
typedef struct {
    error_code_t error;
    recovery_strategy_t strategy;
    int (*recover)(error_context_t *, void *);
    void *context;
} recovery_handler_t;

static int attempt_recovery(error_context_t *err_ctx, build_context_t *build_ctx) {
    recovery_handler_t *handler = find_recovery_handler(err_ctx->stack->code);
    
    if (!handler) {
        LOG_ERROR("No recovery handler for error: %s", 
                  get_error_name(err_ctx->stack->code));
        return ERROR_NO_RECOVERY;
    }
    
    LOG_INFO("Attempting recovery strategy: %s", 
             get_strategy_name(handler->strategy));
    
    switch (handler->strategy) {
        case RECOVERY_RETRY:
            return retry_failed_operation(err_ctx, build_ctx);
            
        case RECOVERY_ROLLBACK:
            return rollback_to_checkpoint(err_ctx, build_ctx);
            
        case RECOVERY_SKIP:
            return skip_failed_component(err_ctx, build_ctx);
            
        case RECOVERY_CUSTOM:
            return handler->recover(err_ctx, handler->context);
            
        default:
            return ERROR_RECOVERY_FAILED;
    }
}
```

---

## Performance Optimization

### Parallel Build System

```c
typedef struct {
    thread_pool_t *pool;
    job_queue_t *queue;
    dependency_graph_t *deps;
    progress_tracker_t *progress;
} parallel_builder_t;

static int execute_parallel_build(parallel_builder_t *builder, 
                                 build_task_list_t *tasks) {
    // Build dependency graph
    for (task_t *task = tasks->head; task; task = task->next) {
        add_task_to_graph(builder->deps, task);
    }
    
    // Find tasks ready to execute
    task_list_t *ready = find_ready_tasks(builder->deps);
    
    // Submit to thread pool
    while (ready->count > 0) {
        for (task_t *task = ready->head; task; task = task->next) {
            job_t *job = create_job(task, job_complete_callback);
            submit_job(builder->pool, job);
        }
        
        // Wait for completions
        wait_for_job_completion(builder->pool);
        
        // Update dependency graph
        update_completed_tasks(builder->deps);
        
        // Find newly ready tasks
        ready = find_ready_tasks(builder->deps);
    }
    
    return builder->deps->failed_count == 0 ? SUCCESS : ERROR_BUILD_FAILED;
}
```

### Cache Management

```c
typedef struct {
    char *cache_dir;
    uint64_t max_size;
    uint64_t current_size;
    lru_list_t *lru;
    hash_table_t *entries;
} build_cache_t;

static cached_item_t* get_cached_item(build_cache_t *cache, const char *key) {
    cached_item_t *item = hash_table_get(cache->entries, key);
    
    if (!item) {
        return NULL;
    }
    
    // Check validity
    if (!validate_cached_item(item)) {
        remove_cached_item(cache, key);
        return NULL;
    }
    
    // Update LRU
    lru_touch(cache->lru, item);
    
    return item;
}

static int add_to_cache(build_cache_t *cache, const char *key, 
                       const void *data, size_t size) {
    // Check cache size
    while (cache->current_size + size > cache->max_size) {
        // Evict LRU item
        cached_item_t *victim = lru_get_victim(cache->lru);
        remove_cached_item(cache, victim->key);
    }
    
    // Add new item
    cached_item_t *item = create_cached_item(key, data, size);
    hash_table_put(cache->entries, key, item);
    lru_add(cache->lru, item);
    cache->current_size += size;
    
    return SUCCESS;
}
```

---

## Security Considerations

### Secure Download Verification

```c
typedef struct {
    char *url;
    char *sha256_expected;
    char *signature_url;
    char *public_key;
} secure_download_t;

static int download_and_verify(secure_download_t *dl, const char *output_path) {
    // Download file
    if (download_file(dl->url, output_path) != SUCCESS) {
        return ERROR_DOWNLOAD_FAILED;
    }
    
    // Verify checksum
    char sha256_actual[65];
    if (calculate_sha256(output_path, sha256_actual) != SUCCESS) {
        unlink(output_path);
        return ERROR_CHECKSUM_FAILED;
    }
    
    if (strcmp(sha256_actual, dl->sha256_expected) != 0) {
        unlink(output_path);
        return ERROR_CHECKSUM_MISMATCH;
    }
    
    // Verify signature if provided
    if (dl->signature_url && dl->public_key) {
        char sig_path[PATH_MAX];
        snprintf(sig_path, sizeof(sig_path), "%s.sig", output_path);
        
        if (download_file(dl->signature_url, sig_path) != SUCCESS) {
            unlink(output_path);
            return ERROR_SIGNATURE_DOWNLOAD_FAILED;
        }
        
        if (verify_signature(output_path, sig_path, dl->public_key) != SUCCESS) {
            unlink(output_path);
            unlink(sig_path);
            return ERROR_SIGNATURE_INVALID;
        }
        
        unlink(sig_path);
    }
    
    return SUCCESS;
}
```

### Privilege Management

```c
typedef struct {
    uid_t original_uid;
    gid_t original_gid;
    capability_set_t *required_caps;
    bool elevated;
} privilege_context_t;

static int elevate_privileges(privilege_context_t *ctx) {
    // Save current credentials
    ctx->original_uid = getuid();
    ctx->original_gid = getgid();
    
    // Check if already root
    if (ctx->original_uid == 0) {
        ctx->elevated = false;
        return SUCCESS;
    }
    
    // Try to elevate
    if (setuid(0) != 0) {
        // Try capabilities instead
        if (set_required_capabilities(ctx->required_caps) != SUCCESS) {
            return ERROR_PRIVILEGE_REQUIRED;
        }
    }
    
    ctx->elevated = true;
    return SUCCESS;
}

static void drop_privileges(privilege_context_t *ctx) {
    if (!ctx->elevated) return;
    
    // Restore original credentials
    setgid(ctx->original_gid);
    setuid(ctx->original_uid);
    
    // Drop capabilities
    drop_all_capabilities();
}
```

---

## API Reference

### Core APIs

#### Build Configuration API

```c
// Configuration management
build_config_t* create_build_config(void);
void free_build_config(build_config_t *config);
int validate_build_config(build_config_t *config);
int merge_build_configs(build_config_t *dst, const build_config_t *src);

// Configuration persistence
int save_build_config(const build_config_t *config, const char *path);
int load_build_config(build_config_t *config, const char *path);

// Configuration modification
int set_config_value(build_config_t *config, const char *key, const char *value);
const char* get_config_value(const build_config_t *config, const char *key);
```

#### Module API

```c
// Module management
int register_module(custom_module_t *module);
int unregister_module(const char *module_name);
custom_module_t* find_module(const char *module_name);
module_list_t* get_loaded_modules(void);

// Module communication
int send_module_message(const char *module_name, message_t *msg);
int broadcast_module_event(event_t *event);

// Module configuration
int set_module_config(const char *module_name, const char *key, const char *value);
const char* get_module_config(const char *module_name, const char *key);
```

#### Build System API

```c
// Build control
int start_build(build_config_t *config);
int pause_build(build_handle_t *handle);
int resume_build(build_handle_t *handle);
int cancel_build(build_handle_t *handle);

// Build status
build_status_t get_build_status(build_handle_t *handle);
int get_build_progress(build_handle_t *handle);
const char* get_current_build_step(build_handle_t *handle);

// Build hooks
int register_build_hook(const char *hook_point, hook_fn callback, void *data);
int unregister_build_hook(const char *hook_point, hook_fn callback);
```

### Extension Points

#### Hook Points

```c
// Pre-build hooks
"build.pre_start"           // Before build starts
"build.pre_download"        // Before downloading sources
"build.pre_kernel"          // Before kernel build
"build.pre_rootfs"          // Before rootfs creation
"build.pre_image"           // Before image creation

// Post-build hooks
"build.post_kernel"         // After kernel build
"build.post_rootfs"         // After rootfs creation
"build.post_image"          // After image creation
"build.post_complete"       // After build completes

// Error hooks
"build.error"               // On build error
"build.recovery"            // During error recovery
```

#### Custom Build Steps

```c
typedef struct {
    char *name;
    char *description;
    build_step_fn execute;
    build_step_fn rollback;
    dependency_list_t *depends_on;
    int priority;
} custom_build_step_t;

int register_build_step(custom_build_step_t *step);
int insert_build_step_after(const char *after_step, custom_build_step_t *step);
int replace_build_step(const char *step_name, custom_build_step_t *new_step);
```

---

## Advanced Customization

### Custom Distribution Creation

```c
typedef struct {
    char *name;
    char *version;
    base_distro_t base;
    package_list_t *packages;
    service_list_t *services;
    file_overlay_t *overlays;
    script_list_t *scripts;
} custom_distro_t;

static int create_custom_distribution(custom_distro_t *distro) {
    // Create base rootfs
    rootfs_t *rootfs = create_base_rootfs(distro->base);
    
    // Install custom packages
    install_package_list(rootfs, distro->packages);
    
    // Configure services
    configure_service_list(rootfs, distro->services);
    
    // Apply file overlays
    apply_overlay_list(rootfs, distro->overlays);
    
    // Run customization scripts
    execute_script_list(rootfs, distro->scripts);
    
    // Generate metadata
    generate_distro_metadata(rootfs, distro);
    
    return SUCCESS;
}
```

### Hardware Adaptation Layer

```c
typedef struct {
    char *board_name;
    char *soc_name;
    device_tree_t *device_tree;
    driver_list_t *drivers;
    peripheral_config_t *peripherals;
    power_management_t *power_config;
} hardware_adaptation_t;

static int apply_hardware_adaptation(hardware_adaptation_t *hal, build_context_t *ctx) {
    // Apply device tree modifications
    modify_device_tree(ctx->kernel_path, hal->device_tree);
    
    // Configure kernel for hardware
    configure_kernel_for_hardware(ctx->kernel_config, hal);
    
    // Install hardware-specific drivers
    for (driver_t *drv = hal->drivers->head; drv; drv = drv->next) {
        install_driver(ctx->rootfs, drv);
    }
    
    // Configure peripherals
    configure_peripherals(ctx->rootfs, hal->peripherals);
    
    // Setup power management
    configure_power_management(ctx->rootfs, hal->power_config);
    
    return SUCCESS;
}
```

### Build Pipeline Customization

```c
typedef struct {
    char *name;
    pipeline_stage_t *stages;
    int stage_count;
    error_handler_t error_handler;
    progress_reporter_t progress_reporter;
} custom_pipeline_t;

static int execute_custom_pipeline(custom_pipeline_t *pipeline, build_context_t *ctx) {
    pipeline_state_t *state = create_pipeline_state(pipeline);
    
    for (int i = 0; i < pipeline->stage_count; i++) {
        pipeline_stage_t *stage = &pipeline->stages[i];
        
        // Check prerequisites
        if (!check_stage_prerequisites(stage, state)) {
            return ERROR_PREREQUISITE_FAILED;
        }
        
        // Execute stage
        LOG_INFO("Executing pipeline stage: %s", stage->name);
        int result = execute_stage(stage, ctx, state);
        
        // Handle result
        if (result != SUCCESS) {
            result = pipeline->error_handler(stage, result, state);
            if (result != SUCCESS) {
                return result;
            }
        }
        
        // Report progress
        pipeline->progress_reporter(i + 1, pipeline->stage_count, stage->name);
    }
    
    return SUCCESS;
}
```

---

## Performance Metrics and Monitoring

### Build Performance Tracking

```c
typedef struct {
    char *metric_name;
    metric_type_t type;
    union {
        uint64_t counter;
        double gauge;
        histogram_t *histogram;
        summary_t *summary;
    } value;
    timestamp_t last_updated;
} performance_metric_t;

typedef struct {
    hash_table_t *metrics;
    metric_exporter_t *exporters;
    int exporter_count;
    pthread_mutex_t lock;
} metrics_registry_t;

static void record_build_metric(const char *name, double value) {
    metrics_registry_t *registry = get_metrics_registry();
    
    pthread_mutex_lock(&registry->lock);
    
    performance_metric_t *metric = hash_table_get(registry->metrics, name);
    if (!metric) {
        metric = create_metric(name, METRIC_TYPE_HISTOGRAM);
        hash_table_put(registry->metrics, name, metric);
    }
    
    histogram_observe(metric->value.histogram, value);
    metric->last_updated = get_timestamp();
    
    pthread_mutex_unlock(&registry->lock);
    
    // Export to configured exporters
    for (int i = 0; i < registry->exporter_count; i++) {
        registry->exporters[i]->export(metric);
    }
}
```

### Resource Usage Monitoring

```c
typedef struct {
    uint64_t cpu_time_user;
    uint64_t cpu_time_system;
    uint64_t memory_peak;
    uint64_t memory_average;
    uint64_t disk_read_bytes;
    uint64_t disk_write_bytes;
    uint64_t network_rx_bytes;
    uint64_t network_tx_bytes;
} resource_usage_t;

static void* resource_monitor_thread(void *arg) {
    build_context_t *ctx = (build_context_t*)arg;
    resource_usage_t usage = {0};
    
    while (!ctx->build_complete) {
        // Sample CPU usage
        update_cpu_usage(&usage);
        
        // Sample memory usage
        update_memory_usage(&usage);
        
        // Sample I/O usage
        update_io_usage(&usage);
        
        // Record metrics
        record_resource_metrics(&usage);
        
        // Sleep until next sample
        sleep_ms(RESOURCE_SAMPLE_INTERVAL_MS);
    }
    
    // Generate resource usage report
    generate_resource_report(&usage, ctx);
    
    return NULL;
}
```

---

## Testing and Validation

### Build Verification Framework

```c
typedef struct {
    char *name;
    char *description;
    test_fn test_function;
    test_type_t type;
    severity_t severity;
} build_test_t;

typedef struct {
    test_suite_t *suites;
    int suite_count;
    test_reporter_t *reporter;
    bool stop_on_failure;
} test_framework_t;

static int run_build_verification(build_output_t *output, test_framework_t *framework) {
    test_results_t *results = create_test_results();
    
    for (int i = 0; i < framework->suite_count; i++) {
        test_suite_t *suite = &framework->suites[i];
        
        LOG_INFO("Running test suite: %s", suite->name);
        
        for (test_t *test = suite->tests; test; test = test->next) {
            test_result_t *result = run_single_test(test, output);
            add_test_result(results, result);
            
            if (result->status == TEST_FAILED && framework->stop_on_failure) {
                break;
            }
        }
    }
    
    // Generate report
    framework->reporter->generate_report(results);
    
    return results->failed_count == 0 ? SUCCESS : ERROR_VALIDATION_FAILED;
}
```

### Integration Testing

```c
static int test_kernel_boot(test_context_t *ctx) {
    // Create QEMU instance
    qemu_instance_t *qemu = create_qemu_instance(&(qemu_config_t){
        .machine = "virt",
        .cpu = "cortex-a72",
        .memory = "2G",
        .kernel = ctx->output->kernel_path,
        .initrd = ctx->output->initrd_path,
        .dtb = ctx->output->dtb_path,
        .cmdline = "console=ttyAMA0"
    });
    
    // Start VM
    if (start_qemu_instance(qemu) != SUCCESS) {
        return TEST_FAILED;
    }
    
    // Wait for boot
    if (wait_for_boot_message(qemu, "Linux version", 30) != SUCCESS) {
        stop_qemu_instance(qemu);
        return TEST_FAILED;
    }
    
    // Run basic commands
    if (run_qemu_command(qemu, "uname -a") != SUCCESS) {
        stop_qemu_instance(qemu);
        return TEST_FAILED;
    }
    
    // Cleanup
    stop_qemu_instance(qemu);
    destroy_qemu_instance(qemu);
    
    return TEST_PASSED;
}
```

---

## Troubleshooting Guide

### Common Build Issues

#### Issue: Kernel Compilation Failures

**Symptoms:**
- Build stops during kernel compilation
- Error messages about missing symbols or configurations

**Diagnosis:**
```c
static void diagnose_kernel_build_failure(build_context_t *ctx) {
    // Check compiler version
    check_compiler_compatibility(ctx->cross_compile);
    
    // Verify kernel configuration
    verify_kernel_config_consistency(ctx->kernel_config);
    
    // Check for missing dependencies
    scan_for_missing_dependencies(ctx->kernel_path);
    
    // Analyze error log
    analyze_build_log(ctx->build_log);
    
    // Generate diagnostic report
    generate_diagnostic_report(ctx, "kernel_build_failure");
}
```

**Resolution Steps:**
1. Verify cross-compiler is properly installed
2. Check kernel configuration for conflicts
3. Ensure all required kernel patches are applied
4. Review module dependencies

#### Issue: Root Filesystem Creation Failures

**Symptoms:**
- Debootstrap fails to complete
- Package installation errors
- Chroot operations fail

**Diagnosis:**
```c
static void diagnose_rootfs_failure(rootfs_context_t *ctx) {
    // Check debootstrap requirements
    verify_debootstrap_setup();
    
    // Verify network connectivity
    test_package_repository_access(ctx->apt_sources);
    
    // Check filesystem permissions
    verify_filesystem_permissions(ctx->rootfs_path);
    
    // Analyze package conflicts
    analyze_package_conflicts(ctx->package_list);
}
```

### Debug Information Collection

```c
typedef struct {
    char *output_dir;
    log_level_t level;
    bool include_core_dumps;
    bool include_system_info;
    bool include_build_artifacts;
} debug_collection_config_t;

static int collect_debug_information(build_context_t *ctx, 
                                   debug_collection_config_t *config) {
    char debug_archive[PATH_MAX];
    snprintf(debug_archive, sizeof(debug_archive), 
             "%s/debug-%s.tar.gz", config->output_dir, get_timestamp_string());
    
    archive_t *archive = create_archive(debug_archive);
    
    // Collect logs
    add_to_archive(archive, ctx->build_log, "logs/build.log");
    add_to_archive(archive, ctx->error_log, "logs/errors.log");
    
    // Collect configuration
    save_build_config_to_file(ctx->config, "/tmp/build_config.json");
    add_to_archive(archive, "/tmp/build_config.json", "config/build.json");
    
    // Collect system information
    if (config->include_system_info) {
        collect_system_info("/tmp/system_info.txt");
        add_to_archive(archive, "/tmp/system_info.txt", "system/info.txt");
    }
    
    // Collect build artifacts
    if (config->include_build_artifacts) {
        add_directory_to_archive(archive, ctx->build_dir, "artifacts/");
    }
    
    // Close archive
    close_archive(archive);
    
    LOG_INFO("Debug information collected: %s", debug_archive);
    return SUCCESS;
}
```

---

## Best Practices and Guidelines

### Code Organization

1. **Module Structure**
   - Keep modules focused on a single responsibility
   - Use consistent naming conventions
   - Document all public interfaces
   - Provide example usage

2. **Error Handling**
   - Always check return values
   - Provide meaningful error messages
   - Clean up resources on failure
   - Use error contexts for debugging

3. **Resource Management**
   - Use RAII pattern where possible
   - Track all allocations
   - Implement proper cleanup handlers
   - Avoid resource leaks

### Performance Optimization

1. **Parallel Execution**
   - Identify independent tasks
   - Use thread pools efficiently
   - Minimize synchronization overhead
   - Balance load across cores

2. **Caching Strategy**
   - Cache expensive operations
   - Implement cache invalidation
   - Monitor cache hit rates
   - Limit cache size

3. **I/O Optimization**
   - Use buffered I/O
   - Minimize disk seeks
   - Batch operations
   - Use async I/O where appropriate

### Security Considerations

1. **Input Validation**
   - Validate all user input
   - Sanitize file paths
   - Check buffer boundaries
   - Validate configuration values

2. **Privilege Management**
   - Run with minimum privileges
   - Drop privileges when possible
   - Use capabilities instead of root
   - Audit privileged operations

3. **Secure Communication**
   - Use HTTPS for downloads
   - Verify signatures
   - Check checksums
   - Validate certificates

---

## Future Development Roadmap

### Planned Features

1. **Cloud Integration**
   - Remote build execution
   - Distributed compilation
   - Cloud storage integration
   - Build artifact sharing

2. **Container Support**
   - Docker image generation
   - Kubernetes deployment
   - Container orchestration
   - Microservice architecture

3. **Advanced GPU Features**
   - GPU compute optimization
   - Machine learning support
   - Video encoding acceleration
   - Graphics performance tuning

4. **Build Analytics**
   - Performance trending
   - Failure analysis
   - Resource optimization
   - Predictive modeling

### Architecture Evolution

```c
// Future module API v2.0
typedef struct {
    // Metadata
    module_metadata_t metadata;
    
    // Lifecycle
    module_lifecycle_t lifecycle;
    
    // Services
    module_service_t *services;
    int service_count;
    
    // Events
    event_handler_t *event_handlers;
    int handler_count;
    
    // Dependencies
    dependency_resolver_t *resolver;
    
    // Configuration
    config_schema_t *config_schema;
    config_validator_t *validator;
    
    // Metrics
    metric_collector_t *metrics;
    
    // API
    module_api_t *public_api;
} module_v2_t;
```

---

## Conclusion

The Orange Pi 5 Plus Ultimate Interactive Builder represents a comprehensive solution for creating custom Linux distributions for ARM64 single-board computers. Through its modular architecture, extensive customization options, and robust error handling, it provides both ease of use for beginners and powerful features for advanced users.

The technical implementation focuses on:
- **Modularity**: Clean separation of concerns with well-defined interfaces
- **Extensibility**: Comprehensive module system for customization
- **Reliability**: Robust error handling and recovery mechanisms
- **Performance**: Optimized build processes with parallelization
- **Security**: Secure download verification and privilege management

This documentation serves as both a reference guide and a development manual for extending and customizing the builder. The architecture is designed to evolve with changing requirements while maintaining backward compatibility and stability.

For questions, contributions, or support, please refer to the project repository and community resources.

---

**Document Version**: 1.0  
**Last Updated**: July 2025  
**Authors**: Setec Labs Technical Team  
**License**: GPLv3