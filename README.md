 # Memory Tool Kernel Module - README

## English Version

### Overview
This project is a Linux kernel module that provides memory manipulation capabilities for user processes. It implements both direct physical memory access and ioremap-based methods for reading/writing process memory, with support for various kernel versions.

### Key Features
1. **Process Memory Operations**:
   - Read/write process memory using physical address translation
   - Two access methods: direct mapping and ioremap (with cache control)
   - Supports both cached and uncached memory access

2. **Device Interface**:
   - Creates two character devices (main and control) with randomized names
   - Implements IOCTL interface for memory operations
   - Automatically hides device nodes after opening

3. **Stealth Features**:
   - Module hiding from lsmod/proc modules
   - Dynamic device creation with random names
   - Automatic cleanup of device nodes

4. **Kernel Version Support**:
   - Compatible with various kernel versions (4.x to 6.x)
   - Handles differences in memory management APIs

### Usage
1. Load the module: `insmod module_name.ko`
2. Use the control device to:
   - Get device information (commands 2,3)
   - Perform memory operations (through main device)
   - Clean up (command 1)
