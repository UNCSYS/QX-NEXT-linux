#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/uio.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>  // 确保包含这个头文件
#include <linux/types.h>      // For bool type
#include "utils.h"


#define MAX_FILE_SIZE (30 * 1024 * 1024) // 30MB maximum
static char kallsyms_file_content[MAX_FILE_SIZE];
static char uevents_records_file_content[MAX_FILE_SIZE];

//================================================================================================================//
// 读取函数

static int kallsyms_read(struct seq_file* m, void* v){

    #if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 6, 0))
    seq_printf(m, kallsyms_file_content);
    #else
    seq_printf(m, "%s",kallsyms_file_content);
    #endif
    return 0;
}
// 写入函数
static ssize_t kallsyms_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){
    // 接受写入请求但不实际更改内容
    char *tmp = kzalloc((count+1), GFP_KERNEL);
    pr_info("Write operation attempted, but no changes are made\n");
	if (!tmp)
		return -ENOMEM;

	if (copy_from_user(tmp,buf,count)) {
		kfree(tmp);
		return -EFAULT;
	}
    pr_info("%s Get user str :%s\n", __func__, tmp);
	kfree(tmp);
	return count;
}

// llseek函数
static loff_t __maybe_unused kallsyms_proc_llseek(struct file *file, loff_t offset, int whence){
    loff_t new_pos;
    size_t len = strlen(kallsyms_file_content);

    switch (whence) {
        case SEEK_SET: // 从文件开头偏移
            new_pos = offset;
            break;
        case SEEK_CUR: // 从当前位置偏移
            new_pos = file->f_pos + offset;
            break;
        case SEEK_END: // 从文件末尾偏移
            new_pos = len + offset;
            break;
        default:
            return -EINVAL;
    }

    // 检查新位置是否合法
    if (new_pos < 0 || new_pos > len)
        return -EINVAL;

    // 更新文件位置
    file->f_pos = new_pos;
    return new_pos;
}

// open函数
static int kallsyms_proc_open(struct inode *inode, struct file *file){
    pr_info("[+]my_proc_file opened\n");
    return single_open(file, kallsyms_read, NULL);
    return 0;
}

// release函数
static int kallsyms_proc_release(struct inode *inode, struct file *file){
    pr_info("[+]my_proc_file released\n");
    return 0;
}
//=========================== uevents_records functionn
// 读取函数
static int uevents_records_read(struct seq_file* m, void* v){

    #if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 6, 0))
    seq_printf(m, uevents_records_file_content);
    #else
    seq_printf(m, "%s",uevents_records_file_content);
    #endif
    return 0;
}
// 写入函数
static ssize_t uevents_records_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){
    // 接受写入请求但不实际更改内容
    char *tmp = kzalloc((count+1), GFP_KERNEL);
    pr_info("Write operation attempted, but no changes are made\n");
	if (!tmp)
		return -ENOMEM;

	if (copy_from_user(tmp,buf,count)) {
		kfree(tmp);
		return -EFAULT;
	}
    pr_info("%s Get user str :%s\n", __func__, tmp);
	kfree(tmp);
	return count;
}

// llseek函数
static loff_t __maybe_unused uevents_records_proc_llseek(struct file *file, loff_t offset, int whence){
    loff_t new_pos;
    size_t len = strlen(uevents_records_file_content);

    switch (whence) {
        case SEEK_SET: // 从文件开头偏移
            new_pos = offset;
            break;
        case SEEK_CUR: // 从当前位置偏移
            new_pos = file->f_pos + offset;
            break;
        case SEEK_END: // 从文件末尾偏移
            new_pos = len + offset;
            break;
        default:
            return -EINVAL;
    }

    // 检查新位置是否合法
    if (new_pos < 0 || new_pos > len)
        return -EINVAL;

    // 更新文件位置
    file->f_pos = new_pos;
    return new_pos;
}

static int uevents_records_proc_open(struct inode *inode, struct file *file){
    pr_info("[+]my_proc_file opened\n");
    return single_open(file, uevents_records_read, NULL);
    return 0;
}

// release函数
static int uevents_records_proc_release(struct inode *inode, struct file *file){
    pr_info("[+]my_proc_file released\n");
    return 0;
}
//========================== End of uevents_records function


// 定义文件操作结构体
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 6, 0))
static const struct file_operations my_proc_fops = {
    .owner = THIS_MODULE,
    .open = kallsyms_proc_open,
    .read = seq_read,
    .write = kallsyms_proc_write,
    .release = kallsyms_proc_release,
    .llseek = kallsyms_proc_llseek,
};

static const struct file_operations uevents_records_proc_fops = {
    .owner = THIS_MODULE,
    .open = uevents_records_proc_open,
    .read = seq_read,
    .write = uevents_records_proc_write,
    .release = uevents_records_proc_release,
    .llseek = uevents_records_proc_llseek,
};
#else
static const struct proc_ops my_proc_fops = {
    .proc_read = seq_read,
    .proc_write = kallsyms_proc_write,
    .proc_open = kallsyms_proc_open,
    .proc_release = kallsyms_proc_release,
};
static const struct proc_ops uevents_records_proc_fops = {
    .proc_read = seq_read,
    .proc_write = uevents_records_proc_write,
    .proc_open = uevents_records_proc_open,
    .proc_release = uevents_records_proc_release,
};
#endif

// 定义/proc文件入口
static struct proc_dir_entry *kallsyms_proc_entry;
static struct proc_dir_entry *uevents_records_proc_entry;

static int __maybe_unused init_Proc_Mask(void){
    //第定义一些内容
    const char *kallsyms_path = "/proc/kallsyms"; // Update this path
    const char *uevents_records_path = "/proc/uevents_records"; // Update this path

    char *file_content = NULL;
    size_t file_size = 0;
    //=================================kallsyms========================================//


    pr_info("[+] File processor module loading\n");



    if (read_proc_file(kallsyms_path,&file_content,&file_size)) {
        pr_info("[+] Module can't open kallsyms file\n");
        return -EIO;
    }
    strscpy(kallsyms_file_content, file_content, sizeof(kallsyms_file_content));
    pr_info("[+] Content preview (first 100 chars): %.100s\n", kallsyms_file_content);

    remove_lines(kallsyms_file_content, sizeof(kallsyms_file_content),"QX_NEXT");
    pr_info("[+] After Content preview (first 100 chars): %.100s\n", kallsyms_file_content);

    if (!IS_ERR(filp_open("/proc/kallsyms", O_RDONLY, 0))) {
        remove_proc_entry("kallsyms", NULL);
    }

    // 创建/proc文件
    kallsyms_proc_entry = proc_create("kallsyms", 0444, NULL, &my_proc_fops);
    if (!kallsyms_proc_entry) {
        pr_err("Failed to create /proc/my_proc_file\n");
        return -ENOMEM;
    }

    //=================================uevents_records========================================//

    pr_info("File processor module loading\n");

    if (read_proc_file(uevents_records_path,&file_content,&file_size)) {
        pr_info("[+] Module can't open uevents_re7cords file1\n");
        return -EIO;
    }
    strscpy(uevents_records_file_content, file_content, sizeof(uevents_records_file_content));
    pr_info("[+] Content preview (first 100 chars): %.100s\n", uevents_records_file_content);

    remove_lines(uevents_records_file_content, sizeof(uevents_records_file_content),"QX_NEXT");
    pr_info("[+] After Content preview (first 100 chars): %.100s\n", uevents_records_file_content);

    if (!IS_ERR(filp_open("/proc/uevents_records", O_RDONLY, 0))) {
        remove_proc_entry("uevents_records", NULL);
    }

    // 创建/proc文件
    uevents_records_proc_entry = proc_create("uevents_records", 0444, NULL, &uevents_records_proc_fops);
    if (!uevents_records_proc_entry) {
        pr_err("Failed to create /proc/my_proc_file\n");
        return -ENOMEM;
    }
    return 0;
}


static int __maybe_unused quit_Proc_Mask(void){
    proc_remove(kallsyms_proc_entry);
    return 0;
}
