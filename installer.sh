#!/bin/bash

# Orange Pi 5 Plus Ultimate Interactive Builder Installer
# Setec Labs Edition v0.1.0a

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BOLD}${CYAN}"
    echo "══════════════════════════════════════════════════════════════════════════════════"
    echo "          ORANGE PI 5 PLUS ULTIMATE INTERACTIVE BUILDER - INSTALLER"
    echo "══════════════════════════════════════════════════════════════════════════════════"
    echo -e "${NC}"
}

check_root() {
    if [ "$EUID" -ne 0 ]; then
        echo -e "${RED}Error: This script must be run with sudo privileges${NC}"
        exit 1
    fi
}

organize_files() {
    echo -e "${YELLOW}Organizing project files...${NC}"
    
    # Check if organize_files.sh exists and is executable
    if [ -f "./organize_files.sh" ]; then
        chmod +x ./organize_files.sh
        ./organize_files.sh
    else
        # Manual organization
        mkdir -p src
        
        # Move source files to src directory if they're in the root
        for file in system.c kernel.c gpu.c ui.c; do
            if [ -f "$file" ]; then
                echo "Moving $file to src/"
                mv "$file" src/
            fi
        done
    fi
    
    echo -e "${GREEN}Files organized successfully${NC}"
}

install_dependencies() {
    echo -e "${YELLOW}Installing required dependencies...${NC}"
    
    apt-get update
    apt-get install -y build-essential git wget make bc bison flex \
    libssl-dev libncurses-dev gcc-aarch64-linux-gnu device-tree-compiler \
    debootstrap qemu-user-static rsync parted dosfstools e2fsprogs cpio
    
    echo -e "${GREEN}Dependencies installed successfully${NC}"
}

setup_debootstrap() {
    echo -e "${YELLOW}Setting up debootstrap for newer Ubuntu releases...${NC}"
    
    # Create debootstrap_fix.sh
    cat > /tmp/debootstrap_fix.sh << 'EOF'
#!/bin/bash
# Script to create symlinks for new Ubuntu versions in debootstrap

# Get the path to debootstrap scripts directory
SCRIPTS_DIR="/usr/share/debootstrap/scripts"

# Create symlink for plucky (25.04) if it doesn't exist
if [ ! -f "${SCRIPTS_DIR}/plucky" ]; then
    if [ -f "${SCRIPTS_DIR}/noble" ]; then
        echo "Creating symlink for plucky (25.04) using noble as base"
        ln -sf "${SCRIPTS_DIR}/noble" "${SCRIPTS_DIR}/plucky"
    else
        echo "noble script not found, trying to create symlink using jammy"
        if [ -f "${SCRIPTS_DIR}/jammy" ]; then
            ln -sf "${SCRIPTS_DIR}/jammy" "${SCRIPTS_DIR}/plucky"
        else
            echo "ERROR: No base script found for plucky"
            exit 1
        fi
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
        echo "ERROR: No base script found for vivid"
        exit 1
    fi
fi

echo "Ubuntu release scripts setup completed successfully"
EOF
    
    chmod +x /tmp/debootstrap_fix.sh
    /tmp/debootstrap_fix.sh
    
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

build_installer() {
    echo -e "${YELLOW}Building the Orange Pi builder...${NC}"
    
    make clean || true
    make
    
    if [ ! -f "builder" ]; then
        echo -e "${RED}Build failed! Check for errors above.${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}Build successful${NC}"
}

install_builder() {
    echo -e "${YELLOW}Installing the builder...${NC}"
    
    # Create directory if it doesn't exist
    mkdir -p /usr/local/bin
    
    # Install the builder
    cp builder /usr/local/bin/orangepi-builder
    chmod +x /usr/local/bin/orangepi-builder
    
    # Copy .env file to /etc for system-wide access
    if [ -f ".env" ]; then
        cp .env /etc/orangepi-builder.env
        chmod 644 /etc/orangepi-builder.env
    fi
    
    echo -e "${GREEN}Installation complete!${NC}"
    echo -e "${BLUE}You can now run the builder with: sudo orangepi-builder${NC}"
}

# Main execution
main() {
    print_header
    check_root
    
    echo -e "${BOLD}This installer will:${NC}"
    echo "1. Organize project files"
    echo "2. Install required dependencies"
    echo "3. Setup debootstrap for newer Ubuntu releases"
    echo "4. Configure GitHub authentication"
    echo "5. Build the Orange Pi builder"
    echo "6. Install it system-wide"
    echo ""
    echo -e "${YELLOW}Do you want to continue? (y/n)${NC}"
    read -r choice
    
    if [[ ! "$choice" =~ ^[Yy]$ ]]; then
        echo "Installation cancelled."
        exit 0
    fi
    
    organize_files
    install_dependencies
    setup_debootstrap
    setup_github_auth
    build_installer
    install_builder
    
    echo -e "${BOLD}${GREEN}Installation completed successfully!${NC}"
    
    # Create build and output directories with proper permissions
    mkdir -p /tmp/opi5plus_build /tmp/opi5plus_output
    chmod 777 /tmp/opi5plus_build /tmp/opi5plus_output
    
    echo -e "${YELLOW}IMPORTANT: ${NC}"
    echo "1. Edit the .env file to add your GitHub token"
    echo "2. Run the builder with: sudo orangepi-builder"
}

# Execute main function
main "$@"