#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include "memory.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,83))
#include <linux/sched/mm.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <linux/sched/task.h>
#endif

// 根据内核版本选择正确的 access_ok 调用方式
static __maybe_unused int my_access_ok(const void __user *buffer, size_t size) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
    return access_ok(buffer, size);
#else
    return access_ok(VERIFY_READ, buffer, size) && access_ok(VERIFY_READ, buffer, size);
#endif
}

// 内核版本 >= 5.4.61 的地址转换实现
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 61))
phys_addr_t translate_linear_address(struct mm_struct* mm, uintptr_t va) {
    pgd_t *pgd;
    p4d_t *p4d;
    pmd_t *pmd;
    pte_t *pte;
    pud_t *pud;
    phys_addr_t page_addr;
    uintptr_t page_offset;

    pgd = pgd_offset(mm, va);
    if(pgd_none(*pgd) || pgd_bad(*pgd)) return 0;

    p4d = p4d_offset(pgd, va);
    if (p4d_none(*p4d) || p4d_bad(*p4d)) return 0;

    pud = pud_offset(p4d,va);
    if(pud_none(*pud) || pud_bad(*pud)) return 0;

    pmd = pmd_offset(pud,va);
    if(pmd_none(*pmd)) return 0;

    pte = pte_offset_kernel(pmd,va);
    if(pte_none(*pte)) return 0;
    if(!pte_present(*pte)) return 0;

    // 页物理地址
#if defined(__pte_to_phys)
    page_addr = (phys_addr_t) __pte_to_phys(*pte);
#elif defined(pte_pfn)
    page_addr = (phys_addr_t)(pte_pfn(*pte) << PAGE_SHIFT);
#else
#error unsupported kernel version：__pte_to_phys or pte_pfn
#endif

    // 页内偏移
    page_offset = va & (PAGE_SIZE-1);
    return page_addr + page_offset;
}
#else
// 内核版本 < 5.4.16 的地址转换实现
phys_addr_t translate_linear_address(struct mm_struct* mm, uintptr_t va) {
    pgd_t *pgd;
    pmd_t *pmd;
    pte_t *pte;
    pud_t *pud;
    phys_addr_t page_addr;
    uintptr_t page_offset;

    pgd = pgd_offset(mm, va);
    if(pgd_none(*pgd) || pgd_bad(*pgd)) return 0;

    pud = pud_offset(pgd,va);
    if(pud_none(*pud) || pud_bad(*pud)) return 0;

    pmd = pmd_offset(pud,va);
    if(pmd_none(*pmd)) return 0;

    pte = pte_offset_kernel(pmd,va);
    if(pte_none(*pte)) return 0;
    if(!pte_present(*pte)) return 0;

    // 页物理地址
#if defined(__pte_to_phys)
    page_addr = (phys_addr_t) __pte_to_phys(*pte);
#elif defined(pte_pfn)
    page_addr = (phys_addr_t)(pte_pfn(*pte) << PAGE_SHIFT);
#else
#error unsupported kernel version：__pte_to_phys or pte_pfn
#endif

    // 页内偏移
    page_offset = va & (PAGE_SIZE-1);
    return page_addr + page_offset;
}
#endif



//============== ioremap ================//
size_t read_physical_address_ioremap(phys_addr_t pa, void* buffer, size_t size, bool cache) {
    void* mapped;

    if (!pfn_valid(__phys_to_pfn(pa))) return 0;
    if (!valid_phys_addr_range(pa, size)) return 0;

    if(cache){
        mapped = ioremap_cache(pa, size);
    }else{
        mapped = ioremap(pa, size);
        //pgprot_t prot = cache ? PAGE_KERNEL : PAGE_KERNEL_NOCACHE;

        // 绕过pfn_valid检查
        //mapped = ioremap_prot(pa, size, prot);
    }


    if (!mapped) return 0;

    if(copy_to_user(buffer, mapped, size)) {
        iounmap(mapped);
        return 0;
    }

    iounmap(mapped);
    return size;
}

size_t write_physical_address_ioremap(phys_addr_t pa, void* buffer, size_t size, bool cache) {
    void* mapped;

    if (!pfn_valid(__phys_to_pfn(pa))) return 0;
    if (!valid_phys_addr_range(pa, size)) return 0;

    mapped = cache ? ioremap_cache(pa, size) : ioremap(pa, size);
    if (!mapped) return 0;

    if(copy_from_user(mapped, buffer, size)) {
        iounmap(mapped);
        return 0;
    }

    iounmap(mapped);
    return size;
}

int read_process_memory_ioremap(pid_t pid, uintptr_t addr, void* buffer, size_t size, bool cache) {
    struct task_struct* task;
    struct mm_struct* mm;
    phys_addr_t pa;
    size_t max;
    size_t count = 0;

    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) return false;

    mm = get_task_mm(task);
    if (!mm) return false;

    while (size > 0) {
        pa = translate_linear_address(mm, addr);
        max = min(PAGE_SIZE - (addr & (PAGE_SIZE - 1)), min(size, PAGE_SIZE));
        if (!pa) goto none_phy_addr;

        count = read_physical_address_ioremap(pa, buffer, max, cache);
    none_phy_addr:
        size -= max;
        buffer += max;
        addr += max;
    }

    mmput(mm);
    return count;
}

int write_process_memory_ioremap(pid_t pid, uintptr_t addr, void* buffer, size_t size, bool cache) {
    struct task_struct* task;
    struct mm_struct* mm;
    phys_addr_t pa;
    size_t max;
    size_t count = 0;

    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) return false;

    mm = get_task_mm(task);
    if (!mm) return false;

    while (size > 0) {
        pa = translate_linear_address(mm, addr);
        max = min(PAGE_SIZE - (addr & (PAGE_SIZE - 1)), min(size, PAGE_SIZE));
        if (!pa) goto none_phy_addr;

        count = write_physical_address_ioremap(pa, buffer, max, cache);
    none_phy_addr:
        size -= max;
        buffer += max;
        addr += max;
    }

    mmput(mm);
    return count;
}




//==============phys_to_virt=============//
size_t read_physical_address_def(phys_addr_t pa, void* buffer, size_t size) {
    void* mapped;

    if (!pfn_valid(__phys_to_pfn(pa))) {
        return 0;
    }
    if (!valid_phys_addr_range(pa, size)) {
        return 0;
    }

    // 直接通过线性映射获取虚拟地址
    mapped = phys_to_virt(pa);

    if (!mapped) {
        return 0;
    }

    if (copy_to_user(buffer, mapped, size)) {
        return 0;
    }

    //flush_dcache_page(virt_to_page(mapped));

    return size;
}
size_t write_physical_address_def(phys_addr_t pa, void* buffer, size_t size) {
    void* mapped;

    if (!pfn_valid(__phys_to_pfn(pa))) {
        return 0;
    }
    if (!valid_phys_addr_range(pa, size)) {
        return 0;
    }

    // 直接通过线性映射获取虚拟地址
    mapped = phys_to_virt(pa);

    if (!mapped) {
        return 0;
    }

    if (copy_from_user(mapped, buffer, size)) {
        return 0;
    }

    return size;
}
bool read_process_memory(pid_t pid, uintptr_t addr, void* buffer, size_t size)
{
    struct task_struct* task;
    struct mm_struct* mm;
    phys_addr_t pa;
    size_t max;
    size_t count = 0;

    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        return false;
    }
    mm = get_task_mm(task);
    if (!mm) {
        return false;
    }
    while (size > 0) {
        pa = translate_linear_address(mm, addr);
        max = min(PAGE_SIZE - (addr & (PAGE_SIZE - 1)), min(size, PAGE_SIZE));
        if (!pa) {
            goto none_phy_addr;
        }

        count = read_physical_address_def(pa, buffer, max);
    none_phy_addr:
        size -= max;
        buffer += max;
        addr += max;
    }
    mmput(mm);
    return count;
}
bool write_process_memory(pid_t pid, uintptr_t addr, void* buffer, size_t size){
    struct task_struct* task;
    struct mm_struct* mm;
    phys_addr_t pa;
    size_t max;
    size_t count = 0;

    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
    return false;
    }
    mm = get_task_mm(task);
    if (!mm) {
        return false;
    }
    while (size > 0) {
        pa = translate_linear_address(mm, addr);
        max = min(PAGE_SIZE - (addr & (PAGE_SIZE - 1)), min(size, PAGE_SIZE));
        if (!pa) {
            goto none_phy_addr;
        }
        count = write_physical_address_def(pa,buffer,max);
    none_phy_addr:
        size -= max;
        buffer += max;
        addr += max;
    }
    mmput(mm);
    return count;
}
