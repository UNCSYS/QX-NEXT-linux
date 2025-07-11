#include <linux/miscdevice.h>
#include <linux/version.h>
#include "dev_service.h"
#include "process.h"
#include "memory.h"
#include "user_helper.h"
#include "utils.h"
//#include "hide_process.h"

/* 全局变量定义 */
struct class *mem_tool_class = NULL;
dev_t main_dev_t = 0;
dev_t ctrl_dev_t = 0;
struct mem_tool_device *memdev = NULL;

/* 文件操作结构体定义 */
struct file_operations main_fops = {
    .owner = THIS_MODULE,
    .open = main_open,
    .release = main_close,
    .unlocked_ioctl = main_ioctl,
};

struct file_operations ctrl_fops = {
    .owner = THIS_MODULE,
    .open = ctrl_open,
    .release = ctrl_close,
    .unlocked_ioctl = ctrl_ioctl,
};

/* 设备节点隐藏函数 */
static int hide_devices_node(void) {
    if (mem_tool_class && !IS_ERR(mem_tool_class)) {
        if (memdev && memdev->main_dev && !IS_ERR(memdev->main_dev)) {
            device_destroy(mem_tool_class, main_dev_t);
        }
        if (memdev && memdev->ctrl_dev && !IS_ERR(memdev->ctrl_dev)) {
            device_destroy(mem_tool_class, ctrl_dev_t);
        }
    }

    unregister_chrdev_region(main_dev_t, 1);
    unregister_chrdev_region(ctrl_dev_t, 1);
    /*
    if (memdev) {
        cdev_del(&memdev->main_cdev);
        cdev_del(&memdev->ctrl_cdev);
        kfree(memdev);
        memdev = NULL;
    }*/
    return 0;
}

/* 主设备操作函数 */
int main_open(struct inode *node, struct file *file) {
    file->private_data = memdev;
    hide_devices_node();
    return 0;
}

int main_close(struct inode *node, struct file *file) {
    return 0;
}

long main_ioctl(struct file* const file, unsigned int const cmd, unsigned long const arg) {
    COPY_MEMORY copy_memory;
    CTRL_MEMORY ctrl_memory;
    MODULE_BASE module_base;
    HIDE_PROCESS hide_proc;
    char name[0x100] = {0};
    static bool lock_read = false;

    switch (cmd) {
        case OP_READ_MEM:
            if (copy_from_user(&copy_memory, (void __user*)arg, sizeof(copy_memory))) {
                return -EFAULT;
            }
            if (lock_read) break;
            if (!read_process_memory(copy_memory.pid, copy_memory.addr, copy_memory.buffer, copy_memory.size)) {
                return -EIO;
            }
            break;

        case OP_WRITE_MEM:
            if (copy_from_user(&ctrl_memory, (void __user*)arg, sizeof(ctrl_memory))) {
                return -EFAULT;
            }
            if (!write_process_memory(ctrl_memory.pid, ctrl_memory.addr, ctrl_memory.buffer, ctrl_memory.size)) {
                return -EIO;
            }
            break;

        case OP_READ_MEM_IOREMAP:
            if (copy_from_user(&copy_memory, (void __user*)arg, sizeof(copy_memory))) {
                return -EFAULT;
            }
            if (lock_read) break;
            if (!read_process_memory_ioremap(copy_memory.pid, copy_memory.addr, copy_memory.buffer, copy_memory.size, false)) {
                return -EIO;
            }
            break;
        case OP_READ_MEM_FAST_IOREMAP:
            if (copy_from_user(&copy_memory, (void __user*)arg, sizeof(copy_memory))) {
                return -EFAULT;
            }
            if (lock_read) break;
            if (!read_process_memory_ioremap(copy_memory.pid, copy_memory.addr, copy_memory.buffer, copy_memory.size, true)) {
                return -EIO;
            }
            break;
        case OP_WRITE_MEM_IOREMAP:
            if (copy_from_user(&ctrl_memory, (void __user*)arg, sizeof(ctrl_memory))) {
                return -EFAULT;
            }
            if (!write_process_memory_ioremap(ctrl_memory.pid, ctrl_memory.addr, ctrl_memory.buffer, ctrl_memory.size, false)) {
                return -EIO;
            }
            break;
        case OP_WRITE_MEM_FAST_IOREMAP:
            if (copy_from_user(&ctrl_memory, (void __user*)arg, sizeof(ctrl_memory))) {
                return -EFAULT;
            }
            if (!write_process_memory_ioremap(ctrl_memory.pid, ctrl_memory.addr, ctrl_memory.buffer, ctrl_memory.size, true)) {
                return -EIO;
            }
            break;


        //-------------------- Other ----------------------
        case OP_MODULE_BASE:
            if (copy_from_user(&module_base, (void __user*)arg, sizeof(module_base)) ||
                copy_from_user(name, (void __user*)module_base.name, sizeof(name) - 1)) {
                return -EFAULT;
            }
            module_base.base = get_module_base(module_base.pid, name);
            if (copy_to_user((void __user*)arg, &module_base, sizeof(module_base))) {
                return -EFAULT;
            }
            break;

        case OP_HIDE_PROCESS:
            if (copy_from_user(&hide_proc, (void __user*)arg, sizeof(hide_proc))) {
                return -EFAULT;
            }
            //hide_process(hide_proc.pid);
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

/* 控制设备操作函数 */
int ctrl_open(struct inode *node, struct file *file) {
    file->private_data = memdev;
    hide_devices_node();
    return 0;
}

int ctrl_close(struct inode *node, struct file *file) {
    return 0;
}


char *main_device_name;
char *ctrl_device_name;
int main_dev_id;
int ctrl_dev_id;

long ctrl_ioctl(struct file* const file, unsigned int const cmd, unsigned long const arg) {
    DRIVER_CMD CTRL_CMD;
    int Ctrl_CMD_ID;
    long len;
    int err;

    int ret2 = 0;

    if (cmd != OP_CTRL) {
        return -EINVAL;
    }

    if (copy_from_user(&CTRL_CMD, (void __user*)arg, sizeof(CTRL_CMD))) {
        return -EFAULT;
    }

    printk(KERN_INFO "[QXNEXT] ctrl id%d", CTRL_CMD.CMD_ID);
    Ctrl_CMD_ID = CTRL_CMD.CMD_ID;

    switch (Ctrl_CMD_ID) {
        case 0:{
            printk(KERN_INFO "[QXNEXT] Connected kernel shell successful");
            break;
        }
        case 1:{
            printk(KERN_INFO "[QXNEXT] kshell: hide");
            // driver_exit();
            //kfree(main_device_name);
            //kfree(ctrl_device_name);
            class_destroy(mem_tool_class);

            hide_devices_node();
            //想设置用户空间内容 必须用到copy_to_user 禁止*(int*)CTRL_CMD.RET = 100;


            if (copy_to_user((void __user *)CTRL_CMD.RET, &ret2, sizeof(ret2))) {
                printk(KERN_ERR "[QXNEXT] Failed to write to user space!\n");
            } else {
                printk(KERN_INFO "[QXNEXT] Successfully modified user-space variable!\n");
            }
            break;
        }
        case 2:{
            char *ret = kasprintf(GFP_KERNEL, "%s&%d", main_device_name, main_dev_id);
            if (!ret) {
                printk(KERN_ERR "[QXNEXT] kasprintf failed!\n");
                return -ENOMEM;
            }
            if (copy_to_user((void __user *)CTRL_CMD.RET, ret, strlen(ret) + 1)) {
                printk(KERN_ERR "[QXNEXT] Failed to write to user space!\n");
                kfree(ret);
                return -EFAULT;
            }
            kfree(ret);
            break;
        }
        case 3:{
            char *ret = kasprintf(GFP_KERNEL, "%s&%d", ctrl_device_name, ctrl_dev_id);
            if (!ret) {
                printk(KERN_ERR "[QXNEXT] kasprintf failed!\n");
                return -ENOMEM;
            }
            if (copy_to_user((void __user *)CTRL_CMD.RET, ret, strlen(ret) + 1)) {
                printk(KERN_ERR "[QXNEXT] Failed to write to user space!\n");
                kfree(ret);
                return -EFAULT;
            }
            kfree(ret);
            break;
        }
        case 5:{
            char *ret = kasprintf(GFP_KERNEL, "NO_RET?");

            int err = call_usr(&ret);
            if (err) {
                kfree(ret);
                return err;
            }
            printk("[QXNEXT] ret will be GETKALLSYMS : %s",ret);

            if (!ret) {
                printk(KERN_ERR "[QXNEXT] kasprintf failed!\n");
                return -ENOMEM;
            }
            if (copy_to_user((void __user *)CTRL_CMD.RET, ret, strlen(ret) + 1)) {
                printk(KERN_ERR "[QXNEXT] Failed to write to user space!\n");
                kfree(ret);
                return -EFAULT;
            }
            kfree(ret);
            break;
        }
        case 6:{
            char *ret = kasprintf(GFP_KERNEL, "NO_RET2?");

            //===============
            char my_arg[256]; // 或动态分配：char *my_arg = kmalloc(256, GFP_KERNEL);
            len = strnlen_user((void __user *)CTRL_CMD.ARG, sizeof(my_arg) - 1);
            if (len <= 0) {
                printk(KERN_ERR "[QXNEXT] [ctrl6] Invalid user string\n");
                return -EFAULT;
            }
            if (copy_from_user(my_arg, (void __user *)CTRL_CMD.ARG, len)) {
                printk(KERN_ERR "[QXNEXT] [ctrl6] Failed to copy from user\n");
                return -EFAULT;
            }
            my_arg[len] = '\0'; // 确保字符串终止（strnlen_user 不包含 '\0'）
            printk(KERN_INFO "[QXNEXT] Kread arg: %s",my_arg);

            //===============

            //检查任务后 再构建返回内容
            if(startWith(my_arg,"CALL_BACK")){
                printk("[QXNEXT] a callback %s",my_arg);
                usr_callback(my_arg);
            }

            err = call_usr(&ret);
            if (err) {
                kfree(ret);
                return err;
            }

            if (!ret) {
                printk(KERN_ERR "[QXNEXT] kasprintf failed!\n");
                return -ENOMEM;
            }
            if (copy_to_user((void __user *)CTRL_CMD.RET, ret, strlen(ret) + 1)) {
                printk(KERN_ERR "[QXNEXT] Failed to write to user space!\n");
                kfree(ret);
                return -EFAULT;
            }
            kfree(ret);
            break;
        }
    }
    return 0;
}

/* 设备服务初始化和清理 */
int init_dev_service(void) {
    int ret;
    main_device_name = generate_random_letters(10, NULL);//main
    ctrl_device_name = generate_random_letters(11, NULL);//ctrl

    printk(KERN_ERR "[QXNEXT] create main device: %s", main_device_name);
    printk(KERN_ERR "[QXNEXT] create ctrl device: %s", ctrl_device_name);

    if (!main_device_name || !ctrl_device_name) {
        ret = -ENOMEM;
        goto fail_names;
    }

    /* 1. 动态申请设备号 */
    ret = alloc_chrdev_region(&main_dev_t, 0, 1, main_device_name);
    if (ret < 0) {
        printk(KERN_ERR "[QXNEXT] get main device id ERROR : %d", ret);
        goto fail_main_region;
    }

    ret = alloc_chrdev_region(&ctrl_dev_t, 0, 1, ctrl_device_name);
    if (ret < 0) {
        printk(KERN_ERR "[QXNEXT] get ctrl device id ERROR : %d", ret);
        goto fail_ctrl_region;
    }

    /* 2. 动态申请设备结构体的内存 */
    memdev = kzalloc(sizeof(struct mem_tool_device), GFP_KERNEL);
    if (!memdev) {
        ret = -ENOMEM;
        goto fail_alloc;
    }

    /* 3. 初始化并且添加cdev结构体 */
    cdev_init(&memdev->main_cdev, &main_fops);
    memdev->main_cdev.owner = THIS_MODULE;
    ret = cdev_add(&memdev->main_cdev, main_dev_t, 1);
    if (ret) {
        printk(KERN_ERR "[QXNEXT] create main cdev failed: %d", ret);
        goto fail_main_cdev;
    }

    cdev_init(&memdev->ctrl_cdev, &ctrl_fops);
    memdev->ctrl_cdev.owner = THIS_MODULE;
    ret = cdev_add(&memdev->ctrl_cdev, ctrl_dev_t, 1);
    if (ret) {
        printk(KERN_ERR "[QXNEXT] create ctrl cdev failed: %d", ret);
        goto fail_ctrl_cdev;
    }

    /* 4. 创建设备文件 */
    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0))
        mem_tool_class = class_create(main_device_name);
    #else
        mem_tool_class = class_create(THIS_MODULE, main_device_name);
    #endif

    if (IS_ERR(mem_tool_class)) {
        ret = PTR_ERR(mem_tool_class);
        printk(KERN_ERR "[QXNEXT] create device class failed: %d", ret);
        goto fail_class_create;
    }

    memdev->main_dev = device_create(mem_tool_class, NULL, main_dev_t, NULL, "%s", main_device_name);
    if (IS_ERR(memdev->main_dev)) {
        ret = PTR_ERR(memdev->main_dev);
        printk(KERN_ERR "[QXNEXT] create main device failed: %d", ret);
        goto fail_main_device;
    }

    memdev->ctrl_dev = device_create(mem_tool_class, NULL, ctrl_dev_t, NULL, "%s", ctrl_device_name);
    if (IS_ERR(memdev->ctrl_dev)) {
        ret = PTR_ERR(memdev->ctrl_dev);
        printk(KERN_ERR "[QXNEXT] create ctrl device failed: %d", ret);
        goto fail_ctrl_device;
    }

    //只需要ctrl传参 main先隐藏
    device_destroy(mem_tool_class, main_dev_t);
    main_dev_id = MAJOR(main_dev_t);
    ctrl_dev_id = MAJOR(ctrl_dev_t);
    return 0;

/* 错误处理 */
fail_ctrl_device:
    device_destroy(mem_tool_class, main_dev_t);
fail_main_device:
    class_destroy(mem_tool_class);
fail_class_create:
    cdev_del(&memdev->ctrl_cdev);
fail_ctrl_cdev:
    cdev_del(&memdev->main_cdev);
fail_main_cdev:
    kfree(memdev);
fail_alloc:
    unregister_chrdev_region(ctrl_dev_t, 1);
fail_ctrl_region:
    unregister_chrdev_region(main_dev_t, 1);
fail_main_region:
    kfree(ctrl_device_name);
fail_names:
    kfree(main_device_name);
    return ret;
}

void cleanup_dev_service(void) {
    hide_devices_node();
    if (mem_tool_class && !IS_ERR(mem_tool_class)) {
        if (memdev && memdev->main_dev && !IS_ERR(memdev->main_dev)) {
            device_destroy(mem_tool_class, main_dev_t);
        }
        if (memdev && memdev->ctrl_dev && !IS_ERR(memdev->ctrl_dev)) {
            device_destroy(mem_tool_class, ctrl_dev_t);
        }
        class_destroy(mem_tool_class);
        mem_tool_class = NULL;
    }

    unregister_chrdev_region(main_dev_t, 1);
    unregister_chrdev_region(ctrl_dev_t, 1);

    if (memdev) {
        cdev_del(&memdev->main_cdev);
        cdev_del(&memdev->ctrl_cdev);
        kfree(memdev);
        memdev = NULL;
    }
}
