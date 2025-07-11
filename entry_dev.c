#include <linux/module.h>
#include <linux/tty.h>
#include <linux/proc_fs.h>
#include <linux/kmod.h>
#include <linux/version.h>
//#include "proc_mask.c"
#include "dev_service.h"
#include "user_helper.h"
#include "utils.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(6, 14, 0))
    MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif


/*-----------------------------------------------------------------------------------------------------------*/
// Tool method by use in "mem_tool_device" "dispatch_functions"

int hide_module_node(void) {
    //list_del_init(&__this_module.list);
    kobject_del(&THIS_MODULE->mkobj.kobj);

    //memcpy(THIS_MODULE->name, "nfc\0", 4);

    //解除模块链表关联（需处理并发）
    list_del_rcu(&THIS_MODULE->list);
    list_del_rcu(&THIS_MODULE->mkobj.kobj.entry);
    //不可二次移除
    //list_del(&THIS_MODULE->list); //lsmod,/proc/modules
    //list_del(&THIS_MODULE->mkobj.kobj.entry); // kobj struct list_head entry

    //内存屏障保持数据一致性
    synchronize_rcu();

    //擦除模块元数据指纹
    memset(&THIS_MODULE->holders_dir, 0, sizeof(THIS_MODULE->holders_dir));
    THIS_MODULE->sect_attrs = NULL;
    THIS_MODULE->notes_attrs = NULL;


    return 0;
}

static int __init driver_entry(void) {
    int ret;

    //1. 完成注册设备
    ret = init_dev_service();
    if(ret != 0){
        printk(KERN_ERR "[QXNEXT] ERROR in init_dev_service : %d", ret);
        return ret;
    }

    // 2. 隐藏模块
    hide_module_node();
    //init_Proc_Mask();

    // 3. 其他
    //nl_init();
    //依赖netlink -> user_helper产出的内容
    //例如kallsyms_lookup_name 转 Method(usrhelper_ok_callback)

    printk(KERN_INFO "[QXNEXT] QXNEXT Init OK\n");

    return 0;
}
// 用户空间支柱正常工作后运行此区域的代码
// WARNING: 不要放必要代码 如init_dev_service()
// TODO:    允许放例如ProcMask的代码
typedef unsigned long (*printk_t)(const char *str);
printk_t my_printk;
typedef struct file* (*filp_open_t)(const char *filename, int flags, umode_t mode);
filp_open_t my_filp_open;
void usrhelper_ok_callback(void){


    printk(KERN_INFO "[QXNEXT] usrhelper_ok_callback working\n");
    my_printk = (printk_t)my_kallsyms_lookup_name("printk");
    my_printk("[QXNEXT] first my printk");
    my_filp_open = (filp_open_t)my_kallsyms_lookup_name("filp_open");

    if (!IS_ERR(my_filp_open("/proc/sched_debug", O_RDONLY, 0))) {
        remove_proc_subtree("sched_debug", NULL);
    }
    if (!IS_ERR(my_filp_open("/proc/uevents_records", O_RDONLY, 0))) {
        remove_proc_entry("uevents_records", NULL);
    }
}


static void driver_exit(void) {
	//quit_Proc_Mask();
    //nl_exit();
    device_destroy(mem_tool_class, main_dev_t);
    device_destroy(mem_tool_class, ctrl_dev_t);
    class_destroy(mem_tool_class);
    cdev_del(&memdev->main_cdev);
    cdev_del(&memdev->ctrl_cdev);
    kfree(memdev);
    unregister_chrdev_region(main_dev_t, 1);
    unregister_chrdev_region(ctrl_dev_t, 1);
    printk(KERN_INFO "[QXNEXT] remove devices successful");
    printk(KERN_INFO "[QXNEXT] kernel driver END");
}

static void __exit driver_unload(void) {
    driver_exit();
}

module_init(driver_entry);
module_exit(driver_unload);

MODULE_DESCRIPTION("Linux Kernel.");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Micro @Neo_Micro");
