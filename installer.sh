#!/bin/bash

# Orange Pi 5 Plus Ultimate Interactive Builder Installer
# Setec Labs Edition v0.1.1

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Installation paths
INSTALL_DIR="/usr/local/bin"
BUILDER_NAME="builder"
WRAPPER_NAME="run-builder"
CONFIG_FILE="/etc/orangepi-builder.env"
BACKUP_DIR="/tmp/orangepi-builder-backup-$"

# Track if we've made a backup
BACKUP_MADE=false

# Error handler
error_exit() {
    echo -e "${RED}Error: $1${NC}" >&2
    
    # If installation failed and we have a backup, restore it
    if [ "$BACKUP_MADE" = true ] && [ -d "$BACKUP_DIR" ]; then
        echo -e "${YELLOW}Installation failed. Restoring previous version...${NC}"
        restore_backup
    fi
    
    exit 1
}

# Trap errors and signals
trap 'error_exit "Script failed at line $LINENO"' ERR
trap 'cleanup_on_exit' EXIT INT TERM

cleanup_on_exit() {
    # Remove backup directory if everything succeeded
    if [ $? -eq 0 ] && [ -d "$BACKUP_DIR" ]; then
        echo -e "${CYAN}Cleaning up backup files...${NC}"
        rm -rf "$BACKUP_DIR"
    fi
}

print_header() {
    echo -e "${BOLD}${CYAN}"
    echo "══════════════════════════════════════════════════════════════════════════════════"
    echo "          ORANGE PI 5 PLUS ULTIMATE INTERACTIVE BUILDER - INSTALLER"
    echo "                              Version 0.1.1"
    echo "══════════════════════════════════════════════════════════════════════════════════"
    echo -e "${NC}"
}

check_root() {
    if [ "$EUID" -ne 0 ]; then
        error_exit "This script must be run with sudo privileges"
    fi
}

check_prerequisites() {
    echo -e "${YELLOW}Checking prerequisites...${NC}"
    
    # Check if builder binary exists
    if [ ! -f "./builder" ]; then
        # Check if we can build it
        if [ ! -f "Makefile" ] && [ ! -f "CMakeLists.txt" ]; then
            error_exit "builder binary not found and no build system detected"
        fi
    fi
    
    # Check if we're on a supported system
    if [ ! -f "/etc/debian_version" ]; then
        error_exit "This installer only supports Debian-based systems"
    fi
    
    echo -e "${GREEN}Prerequisites check passed${NC}"
}

backup_existing_installation() {
    echo -e "${YELLOW}Checking for existing installation...${NC}"
    
    local found_existing=false
    
    # Create backup directory
    mkdir -p "$BACKUP_DIR"
    
    # Check and backup existing files
    if [ -f "$INSTALL_DIR/$BUILDER_NAME" ]; then
        echo "Found existing $BUILDER_NAME, backing up..."
        cp -p "$INSTALL_DIR/$BUILDER_NAME" "$BACKUP_DIR/" || error_exit "Failed to backup $BUILDER_NAME"
        found_existing=true
    fi
    
    if [ -f "$INSTALL_DIR/$WRAPPER_NAME" ]; then
        echo "Found existing $WRAPPER_NAME, backing up..."
        cp -p "$INSTALL_DIR/$WRAPPER_NAME" "$BACKUP_DIR/" || error_exit "Failed to backup $WRAPPER_NAME"
        found_existing=true
    fi
    
    if [ -f "$CONFIG_FILE" ]; then
        echo "Found existing config file, backing up..."
        cp -p "$CONFIG_FILE" "$BACKUP_DIR/" || error_exit "Failed to backup config file"
        found_existing=true
    fi
    
    if [ "$found_existing" = true ]; then
        BACKUP_MADE=true
        echo -e "${GREEN}Backup completed to $BACKUP_DIR${NC}"
        
        # Store version info if possible
        if command -v "$INSTALL_DIR/$BUILDER_NAME" &> /dev/null; then
            "$INSTALL_DIR/$BUILDER_NAME" --version > "$BACKUP_DIR/version.txt" 2>&1 || true
        fi
    else
        echo -e "${GREEN}No existing installation found${NC}"
        # Remove empty backup directory
        rmdir "$BACKUP_DIR" 2>/dev/null || true
    fi
}

uninstall_existing() {
    echo -e "${YELLOW}Removing existing installation...${NC}"
    
    # Remove current installation
    if [ -f "$INSTALL_DIR/$BUILDER_NAME" ]; then
        rm -f "$INSTALL_DIR/$BUILDER_NAME" || error_exit "Failed to remove $BUILDER_NAME"
        echo "Removed $BUILDER_NAME"
    fi
    
    if [ -f "$INSTALL_DIR/$WRAPPER_NAME" ]; then
        rm -f "$INSTALL_DIR/$WRAPPER_NAME" || error_exit "Failed to remove $WRAPPER_NAME"
        echo "Removed $WRAPPER_NAME"
    fi
    
    # Note: We don't remove the config file here as it might contain user settings
    # It will be overwritten if needed during installation
    
    echo -e "${GREEN}Existing installation removed${NC}"
}

restore_backup() {
    echo -e "${YELLOW}Restoring previous installation from backup...${NC}"
    
    if [ ! -d "$BACKUP_DIR" ]; then
        echo -e "${RED}No backup found to restore${NC}"
        return 1
    fi
    
    # Restore all backed up files
    for file in "$BACKUP_DIR"/*; do
        if [ -f "$file" ]; then
            basename=$(basename "$file")
            
            # Skip version.txt
            if [ "$basename" = "version.txt" ]; then
                continue
            fi
            
            # Determine destination
            if [ "$basename" = "$(basename $CONFIG_FILE)" ]; then
                dest="$CONFIG_FILE"
            else
                dest="$INSTALL_DIR/$basename"
            fi
            
            echo "Restoring $basename..."
            cp -p "$file" "$dest" || echo -e "${RED}Failed to restore $basename${NC}"
        fi
    done
    
    echo -e "${GREEN}Previous installation restored${NC}"
}

organize_files() {
    echo -e "${YELLOW}Organizing project files...${NC}"
    
    # Check if organize_files.sh exists and is executable
    if [ -f "./organize_files.sh" ]; then
        chmod +x ./organize_files.sh
        if ! ./organize_files.sh; then
            echo -e "${YELLOW}Warning: organize_files.sh failed, continuing with manual organization${NC}"
        fi
    fi
    
    # Manual organization
    if [ ! -d "src" ]; then
        mkdir -p src
    fi
    
    # Move source files to src directory if they're in the root
    for file in system.c kernel.c gpu.c ui.c; do
        if [ -f "$file" ]; then
            echo "Moving $file to src/"
            mv "$file" src/ || echo -e "${YELLOW}Warning: Could not move $file${NC}"
        fi
    done
    
    echo -e "${GREEN}Files organized successfully${NC}"
}

install_dependencies() {
    echo -e "${YELLOW}Installing required dependencies...${NC}"
    
    # Update package list
    if ! apt-get update; then
        error_exit "Failed to update package list"
    fi
    
    # Install packages
    PACKAGES=(
        build-essential
        git
        wget
        make
        bc
        bison
        flex
        libssl-dev
        libncurses-dev
        gcc-aarch64-linux-gnu
        device-tree-compiler
        debootstrap
        qemu-user-static
        rsync
        parted
        dosfstools
        e2fsprogs
        cpio
    )
    
    for pkg in "${PACKAGES[@]}"; do
        echo -n "Installing $pkg... "
        if apt-get install -y "$pkg" > /dev/null 2>&1; then
            echo -e "${GREEN}OK${NC}"
        else
            echo -e "${YELLOW}Failed (may already be installed)${NC}"
        fi
    done
    
    echo -e "${GREEN}Dependencies installation completed${NC}"
}

setup_debootstrap() {
    echo -e "${YELLOW}Setting up debootstrap for newer Ubuntu releases...${NC}"
    
    # Create debootstrap_fix.sh
    cat > /tmp/debootstrap_fix.sh << 'EOF'
#!/bin/bash
# Script to create symlinks for new Ubuntu versions in debootstrap

# Get the path to debootstrap scripts directory
SCRIPTS_DIR="/usr/share/debootstrap/scripts"

if [ ! -d "$SCRIPTS_DIR" ]; then
    echo "ERROR: debootstrap scripts directory not found"
    exit 1
fi

# Create symlink for plucky (25.04) if it doesn't exist
if [ ! -f "${SCRIPTS_DIR}/plucky" ]; then
    if [ -f "${SCRIPTS_DIR}/noble" ]; then
        echo "Creating symlink for plucky (25.04) using noble as base"
        ln -sf "${SCRIPTS_DIR}/noble" "${SCRIPTS_DIR}/plucky"
    elif [ -f "${SCRIPTS_DIR}/jammy" ]; then
        echo "Creating symlink for plucky (25.04) using jammy as base"
        ln -sf "${SCRIPTS_DIR}/jammy" "${SCRIPTS_DIR}/plucky"
    else
        echo "WARNING: No base script found for plucky"
    fi
fi

# Create symlink for vivid (25.10) if it doesn't exist
if [ ! -f "${SCRIPTS_DIR}/vivid" ]; then
    if [ -f "${SCRIPTS_DIR}/noble" ]; then
        echo "Creating symlink for vivid (25.10) using noble as base"
        ln -sf "${SCRIPTS_DIR}/noble" "${SCRIPTS_DIR}/vivid"
    elif [ -f "${SCRIPTS_DIR}/jammy" ]; then
        echo "Creating symlink for vivid (25.10) using jammy as base"
        ln -sf "${SCRIPTS_DIR}/jammy" "${SCRIPTS_DIR}/vivid"
    else
        echo "WARNING: No base script found for vivid"
    fi
fi

echo "Ubuntu release scripts setup completed"
EOF
    
    chmod +x /tmp/debootstrap_fix.sh
    if ! /tmp/debootstrap_fix.sh; then
        echo -e "${YELLOW}Warning: debootstrap setup had issues, but continuing${NC}"
    fi
    rm -f /tmp/debootstrap_fix.sh
    
    echo -e "${GREEN}debootstrap setup completed${NC}"
}

setup_github_auth() {
    echo -e "${YELLOW}Setting up GitHub authentication...${NC}"
    
    # Create .env file if it doesn't exist
    if [ ! -f ".env" ]; then
        cat > .env << 'EOF'
# Environment variables for Orange Pi 5 Plus Ultimate Interactive Builder

# GitHub personal access token for authentication
# Create one at: https://github.com/settings/tokens
# Required scopes: repo, read:packages
# GITHUB_TOKEN=your_token_here

# Optional: Custom Mali driver URLs
# MALI_DRIVER_URL=https://example.com/path/to/mali/driver.so
# MALI_FIRMWARE_URL=https://example.com/path/to/mali/firmware.bin

# Optional: Override default build settings
# BUILD_JOBS=8
# BUILD_DIR=/custom/build/path
# OUTPUT_DIR=/custom/output/path
EOF
        chmod 600 .env
        echo -e "${GREEN}Created .env template file${NC}"
        echo -e "${YELLOW}Please edit the .env file to add your GitHub token${NC}"
    else
        echo -e "${GREEN}.env file already exists${NC}"
    fi
}

build_builder() {
    echo -e "${YELLOW}Building the Orange Pi builder...${NC}"
    
    # Check if Makefile exists
    if [ -f "Makefile" ]; then
        echo "Running make clean..."
        make clean || echo -e "${YELLOW}Warning: make clean failed${NC}"
        
        echo "Building the project..."
        if ! make; then
            error_exit "Build failed. Please check the compilation errors above"
        fi
        echo -e "${GREEN}Build completed successfully${NC}"
    else
        # If no Makefile, check if builder already exists
        if [ ! -f "./builder" ]; then
            error_exit "No Makefile found and builder binary doesn't exist"
        fi
        echo -e "${YELLOW}Makefile not found, but builder binary exists. Skipping build.${NC}"
    fi
}

install_builder() {
    echo -e "${YELLOW}Installing the builder...${NC}"
    
    # Create directory if it doesn't exist
    mkdir -p "$INSTALL_DIR"
    
    # Check if builder exists
    if [ ! -f "./builder" ]; then
        error_exit "builder binary not found"
    fi
    
    # Install the builder
    cp builder "$INSTALL_DIR/$BUILDER_NAME" || error_exit "Failed to copy builder"
    chmod +x "$INSTALL_DIR/$BUILDER_NAME" || error_exit "Failed to make builder executable"
    
    # Install the wrapper script if it exists
    if [ -f "run-builder" ]; then
        cp run-builder "$INSTALL_DIR/$WRAPPER_NAME" || echo -e "${YELLOW}Warning: Failed to copy wrapper script${NC}"
        chmod +x "$INSTALL_DIR/$WRAPPER_NAME" 2>/dev/null || true
        echo -e "${GREEN}Installed wrapper script for filesystem mounting${NC}"
    fi
    
    # Copy .env file to /etc for system-wide access
    if [ -f ".env" ]; then
        cp .env "$CONFIG_FILE" || echo -e "${YELLOW}Warning: Failed to copy .env file${NC}"
        chmod 644 "$CONFIG_FILE" 2>/dev/null || true
    fi
    
    echo -e "${GREEN}Installation complete!${NC}"
    echo -e "${BLUE}You can now run the builder with:${NC}"
    echo -e "${BLUE}  sudo $BUILDER_NAME${NC}"
    if [ -f "$INSTALL_DIR/$WRAPPER_NAME" ]; then
        echo -e "${BLUE}Or if you have filesystem mounting issues:${NC}"
        echo -e "${BLUE}  sudo $WRAPPER_NAME${NC}"
    fi
}

create_directories() {
    echo -e "${YELLOW}Creating build directories...${NC}"
    
    # Create build and output directories with proper permissions
    for dir in /tmp/opi5plus_build /tmp/opi5plus_output; do
        if mkdir -p "$dir" 2>/dev/null; then
            chmod 777 "$dir" 2>/dev/null || true
            echo -e "${GREEN}Created $dir${NC}"
        else
            echo -e "${YELLOW}Warning: Could not create $dir${NC}"
        fi
    done
}

verify_installation() {
    echo -e "${YELLOW}Verifying installation...${NC}"
    
    local all_good=true
    
    # Check if main binary exists and is executable
    if [ -x "$INSTALL_DIR/$BUILDER_NAME" ]; then
        echo -e "${GREEN}✓ $BUILDER_NAME installed correctly${NC}"
    else
        echo -e "${RED}✗ $BUILDER_NAME not found or not executable${NC}"
        all_good=false
    fi
    
    # Check wrapper script if it should exist
    if [ -f "run-builder" ]; then
        if [ -x "$INSTALL_DIR/$WRAPPER_NAME" ]; then
            echo -e "${GREEN}✓ $WRAPPER_NAME installed correctly${NC}"
        else
            echo -e "${YELLOW}⚠ $WRAPPER_NAME not found or not executable${NC}"
        fi
    fi
    
    # Check config file
    if [ -f "$CONFIG_FILE" ]; then
        echo -e "${GREEN}✓ Configuration file installed${NC}"
    else
        echo -e "${YELLOW}⚠ Configuration file not found${NC}"
    fi
    
    if [ "$all_good" = false ]; then
        error_exit "Installation verification failed"
    fi
    
    echo -e "${GREEN}Installation verified successfully${NC}"
}

# Main execution
main() {
    print_header
    check_root
    
    echo -e "${BOLD}This installer will:${NC}"
    echo "1. Check prerequisites"
    echo "2. Backup any existing installation"
    echo "3. Remove previous installations"
    echo "4. Organize project files"
    echo "5. Install required dependencies"
    echo "6. Setup debootstrap for newer Ubuntu releases"
    echo "7. Configure GitHub authentication"
    echo "8. Build the Orange Pi builder"
    echo "9. Install it system-wide"
    echo ""
    echo -e "${YELLOW}If installation fails, the previous version will be restored automatically.${NC}"
    echo ""
    echo -e "${YELLOW}Do you want to continue? (y/n)${NC}"
    read -r choice
    
    if [[ ! "$choice" =~ ^[Yy]$ ]]; then
        echo "Installation cancelled."
        exit 0
    fi
    
    # Run installation steps
    check_prerequisites
    backup_existing_installation
    uninstall_existing
    organize_files
    install_dependencies
    setup_debootstrap
    setup_github_auth
    build_builder
    install_builder
    create_directories
    verify_installation
    
    # If we get here, installation was successful
    echo ""
    echo -e "${BOLD}${GREEN}════════════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BOLD}${GREEN}              Installation completed successfully!${NC}"
    echo -e "${BOLD}${GREEN}════════════════════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    # Show what was done
    if [ "$BACKUP_MADE" = true ]; then
        echo -e "${CYAN}Previous installation was backed up and can be found at:${NC}"
        echo -e "${CYAN}  $BACKUP_DIR${NC}"
        echo ""
    fi
    
    echo -e "${YELLOW}IMPORTANT NEXT STEPS:${NC}"
    echo -e "${BOLD}1.${NC} Edit the .env file to add your GitHub token:"
    echo -e "   ${BLUE}nano .env${NC}"
    echo ""
    echo -e "${BOLD}2.${NC} Run the builder:"
    echo -e "   ${BLUE}sudo $BUILDER_NAME${NC}"
    echo ""
    echo -e "${BOLD}3.${NC} If you encounter issues, check the log file:"
    echo -e "   ${BLUE}/tmp/orangepi-builder.log${NC}"
    echo ""
    
    # Cleanup will happen automatically via trap
}

# Execute main function
main "$@"
