
#include <linux/types.h>
#include <linux/version.h>

// 函数声明
int read_process_memory_ioremap(pid_t pid, uintptr_t addr, void* buffer, size_t size, bool cache);
int write_process_memory_ioremap(pid_t pid, uintptr_t addr, void* buffer, size_t size, bool cache);
bool read_process_memory(pid_t pid, uintptr_t addr, void *dest, size_t size);
bool write_process_memory(pid_t pid, uintptr_t addr, void *src, size_t size);

// 辅助函数声明
phys_addr_t translate_linear_address(struct mm_struct* mm, uintptr_t va);
size_t read_physical_address_ioremap(phys_addr_t pa, void* buffer, size_t size, bool cache);
size_t write_physical_address_ioremap(phys_addr_t pa, void* buffer, size_t size, bool cache);
size_t read_physical_address_def(phys_addr_t pa, void* buffer, size_t size);
size_t write_physical_address_def(phys_addr_t pa, void* buffer, size_t size);

// 版本相关宏定义
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0))
#include <linux/mmap_lock.h>
#define MM_READ_LOCK(mm) mmap_read_lock(mm)
#define MM_READ_UNLOCK(mm) mmap_read_unlock(mm)
#else
#include <linux/rwsem.h>
#define MM_READ_LOCK(mm) down_read(&(mm)->mmap_sem)
#define MM_READ_UNLOCK(mm) up_read(&(mm)->mmap_sem)
#endif


// valid_phys_addr_range 检查有效phys地址逻辑定义
#ifdef ARCH_HAS_VALID_PHYS_ADDR_RANGE
static size_t get_high_memory(void) {
    struct sysinfo meminfo;
    si_meminfo(&meminfo);
    return (meminfo.totalram * (meminfo.mem_unit / 1024)) << PAGE_SHIFT;
}
    #define valid_phys_addr_range(addr, count) (addr + count <= get_high_memory())
#else
    #define valid_phys_addr_range(addr, count) true
#endif
