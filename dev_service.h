#ifndef DEV_SERVICE_H
#define DEV_SERVICE_H

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>

/* 枚举定义 */
enum OPERATIONS {
    OP_INIT_KEY = 0x800,

    //读写
    OP_READ_MEM = 0x801,
    OP_WRITE_MEM = 0x802,
    OP_READ_MEM_IOREMAP = 0x811,
    OP_WRITE_MEM_IOREMAP = 0x812,
    OP_READ_MEM_FAST_IOREMAP = 0x821,
    OP_WRITE_MEM_FAST_IOREMAP = 0x822,

    OP_MODULE_BASE = 0x803,
    OP_CTRL = 0x804,
    OP_HIDE_PROCESS = 0x805,
};

/* 结构体定义 */
typedef struct _COPY_MEMORY {
    pid_t pid;
    uintptr_t addr;
    void* buffer;
    size_t size;
} COPY_MEMORY, *PCOPY_MEMORY;

typedef struct _CTRL_MEMORY {
    pid_t pid;
    uintptr_t addr;
    void* buffer;
    size_t size;
} CTRL_MEMORY, *PCTRL_MEMORY;

typedef struct _DRIVER_CMD {
    int CMD_ID;
    void* ARG;
    void* RET;
} DRIVER_CMD, *PDRIVER_CMD;

typedef struct _MODULE_BASE {
    pid_t pid;
    char* name;
    uintptr_t base;
} MODULE_BASE, *PMODULE_BASE;

typedef struct _HIDE_PROCESS {
    pid_t pid;
} HIDE_PROCESS, *PHIDE_PROCESS;

/* 设备结构体 */
struct mem_tool_device {
    struct cdev main_cdev;
    struct cdev ctrl_cdev;
    struct device *main_dev;
    struct device *ctrl_dev;
};

/* 全局变量声明 */
extern struct class *mem_tool_class;
extern dev_t main_dev_t;
extern dev_t ctrl_dev_t;
extern struct mem_tool_device *memdev;

/* 函数声明 */
int init_dev_service(void);
void cleanup_dev_service(void);

/* 设备操作函数 */
int main_open(struct inode *node, struct file *file);
int main_close(struct inode *node, struct file *file);
long main_ioctl(struct file* const file, unsigned int const cmd, unsigned long const arg);
int ctrl_open(struct inode *node, struct file *file);
int ctrl_close(struct inode *node, struct file *file);
long ctrl_ioctl(struct file* const file, unsigned int const cmd, unsigned long const arg);

/* 文件操作结构体 */
extern struct file_operations main_fops;
extern struct file_operations ctrl_fops;

#endif /* DEV_SERVICE_H */
