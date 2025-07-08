#!/bin/bash

# Complete Orange Pi 5 Plus Ubuntu System Builder
# Integrates Joshua-Riek Ubuntu Rockchip project and JeffyCN Mali drivers
# 
# Repository Information:
# - Ubuntu Rockchip: https://github.com/Joshua-Riek/ubuntu-rockchip  
# - Mali Drivers: https://github.com/JeffyCN/mirrors/raw/libmali/lib/aarch64-linux-gnu/
#
# Usage: sudo ./build_complete_system.sh [options]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Configuration
KERNEL_VERSION="6.8.0"
BUILD_DIR="/tmp/orangepi_build"
OUTPUT_DIR="/tmp/orangepi_output"
JOBS=$(nproc)
ENABLE_GPU=1
ENABLE_OPENCL=1
ENABLE_VULKAN=1
BUILD_ROOTFS=0
BUILD_UBOOT=0
CREATE_IMAGE=0
CLEAN_BUILD=0
VERBOSE=0

print_header() {
    echo -e "${BOLD}${CYAN}"
    echo "══════════════════════════════════════════════════════════════════════════════════"
    echo "                    ORANGE PI 5 PLUS UBUNTU SYSTEM BUILDER"
    echo "══════════════════════════════════════════════════════════════════════════════════"
    echo -e "${NC}"
    echo -e "${BLUE}Integrating:${NC}"
    echo -e "• ${YELLOW}Joshua-Riek Ubuntu Rockchip${NC}: Ubuntu for Rockchip hardware"
    echo -e "• ${YELLOW}JeffyCN Mali Drivers${NC}: Comprehensive Mali G610 GPU support"
    echo -e "• ${YELLOW}Ubuntu 25.04 LTS${NC}: Latest Ubuntu with all release channels"
    echo ""
}

print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --kernel-version VERSION   Kernel version to build (default: $KERNEL_VERSION)"
    echo "  --build-dir DIR           Build directory (default: $BUILD_DIR)"
    echo "  --output-dir DIR          Output directory (default: $OUTPUT_DIR)"
    echo "  --jobs N                  Number of parallel jobs (default: $JOBS)"
    echo "  --disable-gpu             Disable Mali GPU support"
    echo "  --disable-opencl          Disable OpenCL support"
    echo "  --disable-vulkan          Disable Vulkan support"
    echo "  --build-rootfs            Build Ubuntu root filesystem"
    echo "  --build-uboot             Build U-Boot bootloader"
    echo "  --create-image            Create complete system image"
    echo "  --clean                   Clean previous build"
    echo "  --verbose                 Verbose output"
    echo "  --help                    Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 --build-rootfs --build-uboot --create-image  # Complete system"
    echo "  $0 --clean --jobs 8                            # Clean kernel build"
    echo "  $0 --disable-gpu --kernel-version 6.9.0        # Custom kernel without GPU"
}

log_message() {
    local level=$1
    local message=$2
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    
    case $level in
        "INFO")
            echo -e "${CYAN}[$timestamp]${NC} $message"
            ;;
        "SUCCESS")
            echo -e "${GREEN}[$timestamp] ✓${NC} $message"
            ;;
        "WARNING")
            echo -e "${YELLOW}[$timestamp] ⚠${NC} $message"
            ;;
        "ERROR")
            echo -e "${RED}[$timestamp] ✗${NC} $message"
            ;;
    esac
}

check_dependencies() {
    log_message "INFO" "Checking build dependencies..."
    
    local missing_deps=()
    local required_deps=(
        "gcc-aarch64-linux-gnu"
        "git"
        "wget"
        "build-essential"
        "bc"
        "bison"
        "flex"
        "libssl-dev"
        "libelf-dev"
        "device-tree-compiler"
    )
    
    for dep in "${required_deps[@]}"; do
        if ! dpkg -l | grep -q "^ii.*$dep"; then
            missing_deps+=("$dep")
        fi
    done
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        log_message "WARNING" "Missing dependencies: ${missing_deps[*]}"
        log_message "INFO" "Installing missing dependencies..."
        apt update
        apt install -y "${missing_deps[@]}"
    fi
    
    log_message "SUCCESS" "All dependencies satisfied"
}

prepare_build_environment() {
    log_message "INFO" "Preparing build environment..."
    
    if [ "$CLEAN_BUILD" -eq 1 ] && [ -d "$BUILD_DIR" ]; then
        log_message "INFO" "Cleaning previous build directory..."
        rm -rf "$BUILD_DIR"
    fi
    
    mkdir -p "$BUILD_DIR"
    mkdir -p "$OUTPUT_DIR"
    
    cd "$BUILD_DIR"
    log_message "SUCCESS" "Build environment ready"
}

download_ubuntu_rockchip() {
    log_message "INFO" "Downloading Ubuntu Rockchip project..."
    
    if [ ! -d "ubuntu-rockchip" ]; then
        git clone --depth 1 https://github.com/Joshua-Riek/ubuntu-rockchip.git
        log_message "SUCCESS" "Ubuntu Rockchip project downloaded"
    else
        log_message "INFO" "Ubuntu Rockchip already exists, updating..."
        cd ubuntu-rockchip
        git pull origin main || true
        cd ..
    fi
    
    # Display project information
    log_message "INFO" "Ubuntu Rockchip Project Features:"
    log_message "INFO" "• Ubuntu 22.04 LTS and 24.04 LTS support"
    log_message "INFO" "• 3D hardware acceleration via panfrost"
    log_message "INFO" "• GNOME desktop with Wayland support"
    log_message "INFO" "• Hardware video acceleration"
    log_message "INFO" "• Official Ubuntu package management"
}

download_mali_drivers() {
    if [ "$ENABLE_GPU" -eq 0 ]; then
        log_message "INFO" "GPU support disabled, skipping Mali drivers"
        return 0
    fi
    
    log_message "INFO" "Downloading Mali G610 drivers from JeffyCN mirrors..."
    
    mkdir -p mali_drivers
    cd mali_drivers
    
    local mali_drivers=(
        "libmali-valhall-g610-g6p0-x11-wayland-gbm.so"
        "libmali-valhall-g610-g6p0-wayland-gbm.so"
        "libmali-valhall-g610-g6p0-gbm.so"
        "libmali-valhall-g610-g6p0-x11-gbm.so"
        "libmali-valhall-g610-g13p0-x11-wayland-gbm.so"
    )
    
    local downloaded=0
    for driver in "${mali_drivers[@]}"; do
        local url="https://github.com/JeffyCN/mirrors/raw/libmali/lib/aarch64-linux-gnu/$driver"
        if wget -q "$url"; then
            log_message "SUCCESS" "Downloaded: $driver"
            ((downloaded++))
        else
            log_message "WARNING" "Failed to download: $driver"
        fi
    done
    
    # Download firmware
    local firmware_url="https://github.com/JeffyCN/mirrors/raw/libmali/firmware/g610/mali_csffw.bin"
    if wget -q -O mali_csffw.bin "$firmware_url"; then
        log_message "SUCCESS" "Downloaded Mali CSF firmware"
        ((downloaded++))
    else
        log_message "WARNING" "Failed to download Mali firmware"
    fi
    
    cd ..
    
    if [ $downloaded -eq 0 ]; then
        log_message "ERROR" "Failed to download any Mali components"
        return 1
    fi
    
    log_message "SUCCESS" "Downloaded $downloaded Mali components"
}

build_with_builder() {
    log_message "INFO" "Building with Orange Pi builder..."
    
    # Compile the builder if needed
    if [ ! -f "../builder" ]; then
        log_message "INFO" "Compiling builder..."
        gcc -o ../builder ../builder.c -std=c99 -D_GNU_SOURCE
    fi
    
    # Prepare builder arguments
    local builder_args=""
    
    if [ "$VERBOSE" -eq 1 ]; then
        builder_args="$builder_args --verbose"
    fi
    
    if [ "$CLEAN_BUILD" -eq 1 ]; then
        builder_args="$builder_args --clean"
    fi
    
    if [ "$ENABLE_GPU" -eq 0 ]; then
        builder_args="$builder_args --disable-gpu"
    fi
    
    if [ "$ENABLE_OPENCL" -eq 0 ]; then
        builder_args="$builder_args --disable-opencl"
    fi
    
    if [ "$ENABLE_VULKAN" -eq 0 ]; then
        builder_args="$builder_args --disable-vulkan"
    fi
    
    if [ "$BUILD_ROOTFS" -eq 1 ]; then
        builder_args="$builder_args --build-rootfs"
    fi
    
    if [ "$BUILD_UBOOT" -eq 1 ]; then
        builder_args="$builder_args --build-uboot"
    fi
    
    if [ "$CREATE_IMAGE" -eq 1 ]; then
        builder_args="$builder_args --create-image"
    fi
    
    builder_args="$builder_args --kernel-version $KERNEL_VERSION"
    builder_args="$builder_args --build-dir $BUILD_DIR"
    builder_args="$builder_args --output-dir $OUTPUT_DIR"
    builder_args="$builder_args --jobs $JOBS"
    
    log_message "INFO" "Running: ../builder $builder_args"
    ../builder $builder_args
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --kernel-version)
            KERNEL_VERSION="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --jobs)
            JOBS="$2"
            shift 2
            ;;
        --disable-gpu)
            ENABLE_GPU=0
            ENABLE_OPENCL=0
            ENABLE_VULKAN=0
            shift
            ;;
        --disable-opencl)
            ENABLE_OPENCL=0
            shift
            ;;
        --disable-vulkan)
            ENABLE_VULKAN=0
            shift
            ;;
        --build-rootfs)
            BUILD_ROOTFS=1
            shift
            ;;
        --build-uboot)
            BUILD_UBOOT=1
            shift
            ;;
        --create-image)
            CREATE_IMAGE=1
            shift
            ;;
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        --verbose)
            VERBOSE=1
            shift
            ;;
        --help)
            print_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Main execution
main() {
    print_header
    
    # Check if running as root
    if [ "$EUID" -ne 0 ]; then
        log_message "ERROR" "This script must be run with sudo"
        exit 1
    fi
    
    # Display configuration
    echo -e "${BOLD}${YELLOW}Build Configuration:${NC}"
    echo "• Kernel Version: $KERNEL_VERSION"
    echo "• Build Directory: $BUILD_DIR"
    echo "• Output Directory: $OUTPUT_DIR"
    echo "• Parallel Jobs: $JOBS"
    echo "• Mali GPU Support: $([ $ENABLE_GPU -eq 1 ] && echo "Enabled" || echo "Disabled")"
    echo "• OpenCL Support: $([ $ENABLE_OPENCL -eq 1 ] && echo "Enabled" || echo "Disabled")"
    echo "• Vulkan Support: $([ $ENABLE_VULKAN -eq 1 ] && echo "Enabled" || echo "Disabled")"
    echo "• Build Rootfs: $([ $BUILD_ROOTFS -eq 1 ] && echo "Yes" || echo "No")"
    echo "• Build U-Boot: $([ $BUILD_UBOOT -eq 1 ] && echo "Yes" || echo "No")"
    echo "• Create Image: $([ $CREATE_IMAGE -eq 1 ] && echo "Yes" || echo "No")"
    echo ""
    
    check_dependencies
    prepare_build_environment
    download_ubuntu_rockchip
    download_mali_drivers
    build_with_builder
    
    log_message "SUCCESS" "Build process completed!"
    
    echo ""
    echo -e "${BOLD}${GREEN}Next Steps:${NC}"
    if [ "$CREATE_IMAGE" -eq 1 ]; then
        echo "1. Flash the image to SD card: $OUTPUT_DIR/orangepi5plus-ubuntu25.04.img"
        echo "2. Boot your Orange Pi 5 Plus"
        echo "3. Login with user: orangepi, password: orangepi"
    else
        echo "1. Install the kernel on your Orange Pi 5 Plus"
        echo "2. Reboot and verify with: uname -r"
    fi
    
    if [ "$ENABLE_GPU" -eq 1 ]; then
        echo ""
        echo -e "${BOLD}${CYAN}GPU Testing Commands:${NC}"
        echo "• OpenCL: clinfo | grep -i mali"
        echo "• Vulkan: vulkaninfo | grep -i mali"
        echo "• EGL: eglinfo | grep -i mali"
        echo "• GPU load: cat /sys/class/devfreq/fb000000.gpu/load"
    fi
    
    echo ""
    echo -e "${BOLD}${MAGENTA}Output Files:${NC}"
    echo "• Build log: /tmp/kernel_build.log"
    echo "• Output directory: $OUTPUT_DIR"
    ls -la "$OUTPUT_DIR" 2>/dev/null || echo "No output files generated"
}

# Execute main function
main "$@"
