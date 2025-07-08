#!/bin/bash
# Script to create symlinks for new Ubuntu versions in debootstrap
# This allows debootstrap to work with newer Ubuntu versions that don't have scripts yet

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "This script must be run with sudo privileges"
    exit 1
fi

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
echo "debootstrap should now work with newer Ubuntu releases"