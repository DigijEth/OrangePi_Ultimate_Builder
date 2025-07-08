# Makefile for Orange Pi 5 Plus Ultimate Interactive Builder
# Version: 0.1.0a

CC = gcc
CFLAGS = -Wall -Wextra -O2 -D_GNU_SOURCE
LDFLAGS = -lpthread

# Output binary name
TARGET = builder

# Source directories
SRC_DIR = src
MODULE_DIR = modules

# Source files
MAIN_SRCS = builder.c
SRC_SRCS = $(SRC_DIR)/system.c $(SRC_DIR)/kernel.c $(SRC_DIR)/gpu.c $(SRC_DIR)/ui.c
MODULE_SRCS = $(MODULE_DIR)/debug.c $(MODULE_DIR)/example_module.c

# All source files
SRCS = $(MAIN_SRCS) $(SRC_SRCS)

# Object files
MAIN_OBJS = $(MAIN_SRCS:.c=.o)
SRC_OBJS = $(SRC_SRCS:.c=.o)
MODULE_OBJS = $(MODULE_SRCS:.c=.o)

# All object files
OBJS = $(MAIN_OBJS) $(SRC_OBJS)

# Debug build flag
DEBUG ?= 0

ifeq ($(DEBUG),1)
    CFLAGS += -g -DDEBUG_ENABLED=1
    OBJS += $(MODULE_OBJS)
else
    CFLAGS += -DDEBUG_ENABLED=0
endif

# Include paths
INCLUDES = -I. -I$(MODULE_DIR)

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Pattern rule for object files in root directory
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Pattern rule for object files in src directory
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Pattern rule for object files in modules directory
$(MODULE_DIR)/%.o: $(MODULE_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Dependencies
builder.o: builder.c builder.h
$(SRC_DIR)/system.o: $(SRC_DIR)/system.c builder.h
$(SRC_DIR)/kernel.o: $(SRC_DIR)/kernel.c builder.h
$(SRC_DIR)/gpu.o: $(SRC_DIR)/gpu.c builder.h
$(SRC_DIR)/ui.o: $(SRC_DIR)/ui.c builder.h

ifeq ($(DEBUG),1)
$(MODULE_DIR)/debug.o: $(MODULE_DIR)/debug.c builder.h $(MODULE_DIR)/debug.h
$(MODULE_DIR)/example_module.o: $(MODULE_DIR)/example_module.c builder.h $(MODULE_DIR)/debug.h
endif

# Clean target
clean:
	rm -f $(TARGET) $(MAIN_OBJS) $(SRC_OBJS) $(MODULE_OBJS)

# Install target
install: $(TARGET)
	@echo "Installing Orange Pi 5 Plus Builder..."
	@sudo cp $(TARGET) /usr/local/bin/orangepi-builder
	@sudo chmod +x /usr/local/bin/orangepi-builder
	@echo "Installation complete. Run with: sudo orangepi-builder"

# Uninstall target
uninstall:
	@echo "Uninstalling Orange Pi 5 Plus Builder..."
	@sudo rm -f /usr/local/bin/orangepi-builder
	@echo "Uninstallation complete."

# Debug build
debug:
	$(MAKE) clean
	$(MAKE) DEBUG=1

# Release build
release:
	$(MAKE) clean
	$(MAKE) DEBUG=0

# Help target
help:
	@echo "Orange Pi 5 Plus Ultimate Interactive Builder - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  make              - Build the builder (release mode)"
	@echo "  make debug        - Build with debug support enabled"
	@echo "  make release      - Build without debug support (default)"
	@echo "  make clean        - Remove all build artifacts"
	@echo "  make install      - Install the builder system-wide"
	@echo "  make uninstall    - Uninstall the builder"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Debug build includes:"
	@echo "  - Debug logging and tracing"
	@echo "  - Memory leak detection"
	@echo "  - Performance profiling"
	@echo "  - Custom module support"

.PHONY: all clean install uninstall debug release help