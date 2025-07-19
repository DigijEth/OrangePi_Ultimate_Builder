Github Desktop is the worst application ever. It has deleted over 30 hours of work multiple times and refuses to push after the inital commit. I am currently working on my own git manager for solo projects called GitSolo that lets you do decide what you want to do instead of forcing you to pull before you can push, overwritting your local work, without telling you. 

This project has been scrapped becuse all my hard work was auto over written multiple times by days old code. Everytime I tried to push it said my repo was out of date and I had to pull before I could push (due to manual updates to the readme.md). 

A new project is being released to my repo today.



# Orange Pi 5 Plus Ultimate Interactive Builder

**Version: 0.1.0a**  
**License: GPLv3**  
**Author: Setec Labs**

A comprehensive, interactive tool for building custom Ubuntu distributions for the Orange Pi 5 Plus single-board computer with full Mali G610 GPU support. Support for other boards coming soon.

## Table of Contents

- [Features](#features)
- [System Requirements](#system-requirements)
- [Quick Start](#quick-start)
- [Installation](#installation)
- [Configuration](#configuration)
- [Building Your First Image](#building-your-first-image)
- [Ubuntu Version Compatibility](#ubuntu-version-compatibility)
- [Debootstrap Configuration](#debootstrap-configuration)
- [GPU Support](#gpu-support)
- [Debug Mode](#debug-mode)
- [Troubleshooting](#troubleshooting)
- [Project Structure](#project-structure)
- [Contributing](#contributing)
- [Legal Notice](#legal-notice)

## Features

- **Interactive Menu System**: User-friendly interface for configuring and building custom images
- **Multiple Ubuntu Versions**: Support for Ubuntu 20.04 LTS through 25.10
- **Full GPU Support**: Mali G610 drivers with OpenCL 2.2 and Vulkan 1.2
- **Distribution Types**:
  - Desktop (GNOME)
  - Server (headless)
  - Minimal (base system)
  - Emulation-focused (RetroArch, EmulationStation, etc.)
- **Automated Build Process**: Handles kernel compilation, rootfs creation, and image generation
- **Debug Mode**: Comprehensive debugging with memory leak detection and performance profiling
- **Custom Module System**: Extend functionality without modifying core code

## System Requirements

### Build System Requirements

- **OS**: Ubuntu 20.04 LTS or newer (Ubuntu 22.04 LTS recommended)
- **Architecture**: x86_64
- **RAM**: Minimum 8GB (16GB recommended)
- **Storage**: At least 25GB free space
- **Privileges**: Root access (sudo)
- **Internet**: Stable connection for downloading sources

### Target Device

- **Board**: Orange Pi 5 Plus
- **Storage**: microSD card (minimum 8GB, 16GB+ recommended)

## Quick Start

```bash
# Clone the repository
git clone https://github.com/digijeth/orangepi5plus-builder.git
cd orangepi5plus-builder

# Run the installer
sudo ./installer.sh

# Build your first image
sudo builder
```

## Installation

### 1. Prerequisites

The installer will automatically install required packages, but you can install them manually:

```bash
sudo apt update
sudo apt install -y \
    build-essential git wget curl bc \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    debootstrap qemu-user-static \
    device-tree-compiler u-boot-tools \
    parted dosfstools e2fsprogs
```

### 2. GitHub Authentication

This builder requires a GitHub personal access token to avoid rate limits:

1. Go to [GitHub Settings > Developer settings > Personal access tokens](https://github.com/settings/tokens)
2. Create a new token with `repo` and `read:packages` scopes
3. Create a `.env` file in the project root:

```bash
echo "GITHUB_TOKEN=your_token_here" > .env
chmod 600 .env
```

### 3. Build and Install

```bash
# Make installer executable
chmod +x installer.sh

# Run installer (handles everything automatically)
sudo ./installer.sh

# Or build manually
make clean
make debug  # Includes debug features (recommended)
# make release  # For production build without debug
sudo make install
```

## Configuration

### Environment Variables

The builder uses a `.env` file for configuration:

```bash
# GitHub authentication (REQUIRED)
GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxx

# Optional: Custom Mali driver URLs
MALI_DRIVER_URL=https://example.com/mali-driver.so
MALI_FIRMWARE_URL=https://example.com/mali-firmware.bin

# Optional: Build settings
BUILD_JOBS=8
OUTPUT_DIR=/custom/output/path
```

### Build Configuration

When running the builder, you can configure:

- **Distribution Type**: Desktop, Server, Minimal, or Emulation
- **Ubuntu Version**: 20.04 LTS through 25.10
- **Kernel Version**: Default or custom
- **GPU Support**: Enable/disable Mali, OpenCL, Vulkan
- **Image Settings**: Size, hostname, username, password

## Building Your First Image

### Interactive Mode (Recommended)

```bash
sudo builder
```

Follow the menu options:
1. Choose "Quick Setup" for recommended settings
2. Or "Custom Build" for advanced configuration
3. Select Ubuntu version (22.04 LTS recommended)
4. Configure options as needed
5. Start the build process

### Command Line Mode

```bash
# Build with specific options
sudo builder --ubuntu 22.04 --kernel-version 6.1.0 --jobs 8

# Available options
sudo builder --help
```

## Ubuntu Version Compatibility

### Recommended: Ubuntu 22.04 LTS (Jammy)

Ubuntu 22.04 LTS is the **most stable and tested** version for this builder:
- Best hardware compatibility
- Stable package repositories
- Long-term support until 2027
- All features fully tested

### Version Compatibility Table

| Version | Codename | Status | Stability | Notes |
|---------|----------|---------|-----------|--------|
| 20.04 LTS | Focal | ✅ Supported | Stable | Older kernel, limited features |
| **22.04 LTS** | **Jammy** | **✅ Supported** | **Most Stable** | **RECOMMENDED** |
| 24.04 LTS | Noble | ✅ Supported | Stable | Newer, well-tested |
| 25.04 | Plucky | ⚠️ Preview | Experimental | Development version |
| 25.10 | Vivid | ⚠️ Preview | Experimental | Not yet released |

## Debootstrap Configuration

Debootstrap is used to create the Ubuntu root filesystem. For newer Ubuntu versions not yet supported by your system's debootstrap, you need to create symlinks.

### Understanding Debootstrap Scripts

Debootstrap scripts are located in `/usr/share/debootstrap/scripts/`. Each Ubuntu release needs a corresponding script.

### Manual Symlink Creation

For Ubuntu versions without debootstrap scripts:

```bash
# For Ubuntu 25.04 (Plucky)
sudo ln -sf /usr/share/debootstrap/scripts/jammy /usr/share/debootstrap/scripts/plucky

# For Ubuntu 25.10 (Vivid)
sudo ln -sf /usr/share/debootstrap/scripts/jammy /usr/share/debootstrap/scripts/vivid
```

### Automatic Setup

The installer includes `debootstrap.sh` which automatically creates necessary symlinks:

```bash
sudo ./debootstrap.sh
```

### Why Symlinks Work

Newer Ubuntu versions are generally backward-compatible with previous LTS releases for the base system installation. The symlink tells debootstrap to use the jammy (22.04) installation process, then the system updates to the newer version through package updates.

## GPU Support

### Mali G610 Features

- **OpenGL ES**: 3.2
- **OpenCL**: 2.2 (compute acceleration)
- **Vulkan**: 1.2 (modern graphics API)
- **Video Decode**: 8K@60fps H.265/H.264
- **Video Encode**: 8K@30fps H.265/H.264

### Testing GPU Installation

After building and booting your image:

```bash
# Check GPU status
gpu-info

# Test OpenGL ES
es2_info
glmark2-es2

# Test OpenCL
clinfo
test-opencl

# Test Vulkan
vulkaninfo
test-vulkan
```

## Debug Mode

Debug mode is enabled by default and provides:

### Features

- Detailed logging with timestamps
- Memory leak detection
- Performance profiling
- Custom module support
- Debug menu (options 900+)

### Accessing Debug Features

In the main menu, options 900+ provide debug functionality:
- 901: Memory Report
- 902: Timer Report
- 903: Configuration Dump
- 904: System Information
- 907: Interactive Debug Shell

### Building Without Debug

```bash
make release
sudo make install
```

## Troubleshooting

### Common Issues

#### 1. "/proc is not mounted" Error

**Problem**: Error message about /proc not being mounted when running the builder

**Causes**:
- Running in a chroot environment without proper filesystem mounts
- Running in a Docker/LXC container without privileged mode
- Running on a minimal system where /proc isn't auto-mounted

**Solutions**:

**Quick Fix**:
```bash
# Mount /proc manually
sudo mount -t proc /proc /proc

# Then run the builder
sudo builder
```

**For Docker Users**:
```bash
# Run container with proper mounts
docker run --privileged -v /proc:/proc -v /sys:/sys -v /dev:/dev ...
```

**For chroot Users**:
```bash
# Before entering chroot
sudo mount -t proc /proc /path/to/chroot/proc
sudo mount -t sysfs /sys /path/to/chroot/sys
sudo mount -o bind /dev /path/to/chroot/dev
sudo mount -o bind /dev/pts /path/to/chroot/dev/pts
```

**Create a Wrapper Script** (`run-builder.sh`):
```bash
#!/bin/bash
# Ensure we're running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root (use sudo)"
    exit 1
fi

# Check and mount essential filesystems
if [ ! -e /proc/self ]; then
    echo "Mounting /proc..."
    mount -t proc /proc /proc
fi

if [ ! -e /sys/class ]; then
    echo "Mounting /sys..."
    mount -t sysfs /sys /sys
fi

if [ ! -e /dev/null ]; then
    echo "Mounting /dev..."
    mount -t devtmpfs /dev /dev
fi

# Run the builder
/usr/local/bin/builder "$@"
```

Then:
```bash
chmod +x run-builder.sh
sudo ./run-builder.sh
```

#### 2. Locale Errors During Package Installation

**Problem**: Errors like "Cannot set LC_CTYPE to default locale" when installing packages

**Causes**:
- Locale not properly configured in the chroot environment
- Missing language packs
- Environment variables not set during package installation

**Solution**:
The builder now automatically:
- Installs and configures locales immediately after debootstrap
- Sets all required locale environment variables (LANG, LC_ALL, LC_CTYPE, LC_MESSAGES)
- Installs language-pack-en for complete locale support
- Creates an apt-wrapper script to ensure locale is always set during package operations

If you still encounter locale issues:
```bash
# Manually fix locale in the chroot
sudo chroot /path/to/rootfs locale-gen en_US.UTF-8
sudo chroot /path/to/rootfs update-locale LANG=en_US.UTF-8
```

#### 3. GitHub Authentication Failures

**Problem**: "Authentication failed" when cloning repositories

**Solution**:
```bash
# Check token is set
cat .env | grep GITHUB_TOKEN

# Test token
curl -H "Authorization: token YOUR_TOKEN" https://api.github.com/user
```

#### 3. Debootstrap Failures

**Problem**: "No such script: /usr/share/debootstrap/scripts/plucky"

**Solution**:
```bash
# Run debootstrap setup
sudo ./debootstrap.sh

# Or manually create symlink
sudo ln -sf /usr/share/debootstrap/scripts/jammy /usr/share/debootstrap/scripts/plucky
```

#### 4. Build Failures

**Problem**: Compilation errors

**Solution**:
```bash
# Clean and retry
make clean
make debug

# Check logs
cat /tmp/opi5plus_build.log
cat /tmp/opi5plus_build_errors.log
```

#### 5. Insufficient Space

**Problem**: "No space left on device"

**Solution**:
- Ensure at least 25GB free in /tmp
- Change build directory in .env:
  ```bash
  BUILD_DIR=/path/with/more/space
  ```

### Log Files

- Main log: `/tmp/opi5plus_build.log`
- Error log: `/tmp/opi5plus_build_errors.log`
- Debug log: `/tmp/opi5plus_debug.log` (when debug enabled)

## Project Structure

```
orangepi5plus-builder/
├── Makefile              # Build configuration
├── README.md            # This file
├── installer.sh         # Automated installer
├── debootstrap.sh       # Debootstrap setup
├── builder.c            # Main program
├── builder.h            # Common headers
├── .env                 # Configuration (create this)
├── src/                 # Core source files
│   ├── system.c         # System utilities
│   ├── kernel.c         # Kernel building
│   ├── gpu.c            # GPU driver installation
│   └── ui.c             # User interface
└── modules/             # Optional modules
    ├── debug.h          # Debug system header
    ├── debug.c          # Debug implementation
    └── example_module.c # Module template
```

## Contributing

### Development Setup

```bash
# Enable debug build
make debug

# Test changes
sudo ./builder

# Check for memory leaks (debug mode)
# Access debug menu option 901
```

### Adding Custom Modules

See `modules/example_module.c` for a template. Modules can:
- Add menu options
- Hook into build process
- Override default behavior
- Add custom build steps

### Coding Standards

- C99 compatible
- 4-space indentation
- Maximum line length: 100 characters
- Comment all functions
- Check return values

## Legal Notice

### Software License

This software is licensed under GPLv3. See LICENSE file for details.

### Important Legal Information

- **NO copyrighted games, BIOS files, or ROMs are included**
- **Users must legally own any content they use**
- **Emulation platforms are installed WITHOUT copyrighted content**
- **This tool is for legitimate purposes only**
- **Users are responsible for complying with all applicable laws**

### Mali GPU Drivers

Mali drivers are property of ARM Limited and are redistributed under ARM's license terms. Users must comply with ARM's licensing requirements.

## Credits and Acknowledgments

This project incorporates and builds upon the excellent work of many open-source projects and their maintainers:

### Core Projects

- **[Orange Pi Linux Kernel](https://github.com/orangepi-xunlong/linux-orangepi)** - Official Orange Pi kernel sources
- **[Rockchip Linux](https://github.com/rockchip-linux)** - Rockchip's official Linux repositories
- **[Joshua Riek's Ubuntu Rockchip](https://github.com/Joshua-Riek/ubuntu-rockchip)** - Ubuntu for Rockchip RK3588 devices
  - Special thanks to Joshua Riek for the comprehensive RK3588 Ubuntu implementation
- **[Armbian](https://github.com/armbian/build)** - Linux for ARM development boards
  - Thanks to the Armbian team for their extensive collection of patches and configurations

### GPU Support

- **[JeffyCN's Rockchip Mirrors](https://github.com/JeffyCN/mirrors)** - Mali GPU drivers and firmware
  - Thanks to JeffyCN for maintaining Mali driver mirrors
- **[Panfrost Project](https://gitlab.freedesktop.org/panfrost)** - Open-source Mali GPU drivers
- **ARM Mali Team** - For the proprietary Mali G610 drivers

### Emulation Platforms

- **[LibreELEC](https://github.com/LibreELEC/LibreELEC.tv)** - Just enough OS for Kodi
- **[RetroPie](https://github.com/RetroPie)** - Retro-gaming on the Raspberry Pi
- **[EmulationStation](https://github.com/RetroPie/EmulationStation)** - Emulator front-end

### Build System Components

- **[U-Boot](https://github.com/u-boot/u-boot)** - Universal Boot Loader
- **[ARM Trusted Firmware](https://github.com/ARM-software/arm-trusted-firmware)** - Secure world software
- **[Linux Kernel](https://github.com/torvalds/linux)** - The Linux kernel

### Special Thanks

- **Linus Torvalds** and all Linux kernel contributors
- **Canonical/Ubuntu** team for the ARM64 port
- **Debian** team for debootstrap and the package system
- **Orange Pi/Xunlong** for the hardware and BSP support
- **Rockchip** for the RK3588 SoC documentation and support
- **ARM** for the Mali GPU drivers and documentation
- All the unnamed contributors who make open-source possible

### License Acknowledgments

Each incorporated project maintains its own license. Users must comply with all applicable licenses:
- Linux Kernel: GPLv2
- U-Boot: GPLv2+
- ARM Trusted Firmware: BSD-3-Clause
- Mali Drivers: Proprietary (ARM Limited)
- Ubuntu: Various (mainly GPL)

This builder (Orange Pi 5 Plus Ultimate Interactive Builder) is licensed under GPLv3.

## Frequently Asked Questions (FAQ)

### Q: Do I need to run Ubuntu 25.04 to build a Ubuntu 25.04 image?

**A: No!** You can build any Ubuntu version from any other version. The builder uses `debootstrap` which downloads packages directly from Ubuntu repositories for your target version.

**What you need:**
- A reasonably recent Ubuntu host (20.04 or newer recommended)
- The debootstrap symlink fix (handled automatically by the installer)

**Why it works:**
- Debootstrap downloads packages for the target version, not your host
- The chroot environment isolates the target system
- Cross-version building is a core feature of debootstrap

**Example:** You can run the builder on Ubuntu 20.04 and create images for Ubuntu 22.04, 24.04, or even experimental 25.04.

### Q: Which Ubuntu version should I build?

**A:** For production use, **Ubuntu 22.04 LTS (Jammy) is strongly recommended** as it's the most stable and tested. For testing or development, you can try newer versions, but expect potential issues with unreleased versions like 25.04.

### Q: Why do I get "No such script" errors for newer Ubuntu versions?

**A:** Newer Ubuntu versions (like 25.04 Plucky) don't have debootstrap scripts on older systems. The installer automatically creates symlinks to fix this. You can also manually run:
```bash
sudo ./debootstrap.sh
```

### Q: Can I build ARM64 images on an x86_64 host?

**A:** Yes! The builder uses `qemu-user-static` for cross-architecture support. It automatically handles the emulation needed to run ARM64 binaries during the build process.

### Q: How much disk space do I really need?

**A:** 
- Minimum: 15GB free space
- Recommended: 25GB+ free space
- The build process creates temporary files, downloads sources, and builds packages
- Final image size depends on your configuration (typically 4-8GB)

### Q: Can I resume a failed build?

**A:** Currently, the builder doesn't support resuming. If a build fails, you need to start over. However, downloaded files are cached, so subsequent builds are faster.

### Q: Why does the build take so long?

**A:** The builder:
- Downloads and compiles the kernel (20-40 minutes)
- Creates a complete root filesystem
- Downloads and installs hundreds of packages
- Builds U-Boot and creates the final image

Total time: 30-120 minutes depending on your system and internet speed.

### Q: Can I use this builder in a Docker container?

**A:** Yes, but you need to run the container with special privileges:
```bash
docker run --privileged -v /proc:/proc -v /sys:/sys -v /dev:/dev ...
```

### Q: Is the Mali GPU driver included automatically?

**A:** Yes, if you enable GPU support in the configuration. The builder downloads and installs:
- Mali G610 firmware
- OpenGL ES libraries
- OpenCL 2.2 runtime (if enabled)
- Vulkan 1.2 support (if enabled)

## TODO / Roadmap

### High Priority Features

- [ ] **Custom Package Management**
  - Add ability to specify custom package lists in configuration files
  - Support for adding PPAs and third-party repositories
  - Package version pinning support
  - Create package profiles (e.g., "development", "multimedia", "gaming")
  - GUI for package selection

- [ ] **Expanded Emulation Support**
  - Complete RetroArch integration with optimal settings
  - Add more emulation platforms (Recalbox, VICE, MAME standalone)
  - Automatic controller configuration
  - Performance profiles for different emulation cores
  - Legal ROM management system
  - Shader preset configurations

- [ ] **Build System Improvements**
  - Resume failed builds from last successful step
  - Incremental builds (reuse unchanged components)
  - Build caching system
  - Parallel download support
  - Progress saving and restoration
  - Build time estimation

### Medium Priority Features

- [ ] **Enhanced GPU Support**
  - Automatic GPU driver updates
  - Performance tuning profiles
  - GPU compute workload optimization
  - Hardware video encoding presets
  - Wayland compositor optimization

- [ ] **Network Boot Support**
  - PXE boot image generation
  - NFS root filesystem support
  - iSCSI boot configuration
  - Network installer creation

- [ ] **Advanced Customization**
  - Theme and branding customization
  - Boot splash screen editor
  - First-boot setup wizard
  - OEM installation mode
  - Unattended installation support

- [ ] **Development Features**
  - Kernel module development framework
  - Cross-compilation toolchain setup
  - Remote debugging support
  - Performance profiling tools
  - Automated testing framework

### Low Priority / Future Features

- [ ] **Cloud Integration**
  - Build images in the cloud
  - Distributed compilation support
  - Image hosting and sharing
  - Build farm support
  - CI/CD pipeline integration

- [ ] **Additional Platforms**
  - Support for other RK3588 boards
  - Generic ARM64 board support
  - RISC-V experimental support
  - x86_64 UEFI images

- [ ] **User Interface**
  - Web-based builder interface
  - Build configuration wizard
  - Real-time build monitoring
  - Build history and statistics
  - Multi-language support

- [ ] **Container Support**
  - Generate Docker/Podman images
  - Kubernetes node images
  - LXC/LXD container images
  - Snap package creation
  - Flatpak runtime generation

### Community Requested Features

- [ ] **Custom kernel patches interface**
- [ ] **Automated security updates**
- [ ] **Build reproducibility**
- [ ] **Signed image support**
- [ ] **Secure Boot integration**
- [ ] **Full disk encryption options**
- [ ] **Custom partition layouts GUI**
- [ ] **Post-install script framework**
- [ ] **Hardware testing suite**
- [ ] **Benchmark integration**

### Contributing

Want to help implement these features? Check out the [Contributing](#contributing) section or open an issue to discuss your ideas!

## Known Issues and Fixes

### Build Environment Issues

1. **Filesystem Mounting**: The builder automatically mounts `/proc`, `/sys`, `/dev`, and `/dev/pts` in the chroot environment during rootfs creation. This prevents errors during package installation.

2. **Locale Configuration**: The builder now properly configures locales to prevent "Cannot set LC_CTYPE" errors. It installs the `locales` and `language-pack-en` packages and sets all required environment variables.

3. **Python Warnings**: Python 3.12 regex warnings are suppressed by setting `PYTHONWARNINGS=ignore` during the build process.

### Recent Improvements

- **v0.1.0a**: 
  - Fixed `/proc` not mounted errors in chroot environments
  - Resolved locale configuration issues during package installation
  - Added proper filesystem mounting and unmounting
  - Improved error handling with cleanup on failure
  - Suppressed Python 3.12 syntax warnings

## Support

### Resources

- [Orange Pi 5 Plus Wiki](http://www.orangepi.org/html/hardWare/computerAndMicrocontrollers/details/Orange-Pi-5-Plus.html)
- [Rockchip RK3588 Documentation](https://opensource.rock-chips.com/wiki_RK3588)
- [Ubuntu ARM Documentation](https://wiki.ubuntu.com/ARM)
- [Joshua Riek's Ubuntu Rockchip Wiki](https://github.com/Joshua-Riek/ubuntu-rockchip/wiki)
- [Armbian Documentation](https://docs.armbian.com/)

### Reporting Issues

1. Check existing issues on GitHub
2. Provide detailed information:
   - Ubuntu version (host and target)
   - Error messages
   - Log files
   - Steps to reproduce

### Community

- [Orange Pi Forums](http://www.orangepi.org/orangepibbsen/)
- [Armbian Forums](https://forum.armbian.com/)
- [Ubuntu ARM Community](https://discourse.ubuntu.com/c/support/arm/116)
- [CNX Software](https://www.cnx-software.com/) - Embedded systems news

---

**Happy Building!** 🚀

*Created with ❤️ by Setec Labs in Seattle, Wa*

*Standing on the shoulders of giants - thank you to all the open-source contributors who made this possible!*
