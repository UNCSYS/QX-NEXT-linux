#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/kallsyms.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");

// 声明与原始驱动相同的结构体（需要根据实际驱动定义调整）
struct nvt_ts_data {
    struct input_dev *input_dev;
    // 其他成员...
};

// 全局变量保存ts指针
static struct nvt_ts_data *g_ts = NULL;

// 模拟触摸屏幕中央
static void simulate_center_tap(void)
{
    struct input_dev *input_dev;
    int x_max, y_max;
    int x, y;

    if (!g_ts) {
        pr_err("ts pointer not available\n");
        return;
    }

    input_dev = g_ts->input_dev;
    if (!input_dev) {
        pr_err("input device not available\n");
        return;
    }

    // 获取屏幕尺寸（假设驱动已经设置了这些参数）
    x_max = input_abs_get_max(input_dev, ABS_MT_POSITION_X);
    y_max = input_abs_get_max(input_dev, ABS_MT_POSITION_Y);

    if (x_max <= 0 || y_max <= 0) {
        pr_err("Invalid screen dimensions: %dx%d\n", x_max, y_max);
        return;
    }

    // 计算中心点坐标
    x = x_max / 2;
    y = y_max / 2;

    pr_info("Simulating touch at center: (%d, %d)\n", x, y);

    // 上报触摸事件
    input_report_key(input_dev, BTN_TOUCH, 1);
    input_report_abs(input_dev, ABS_MT_TRACKING_ID, 0);
    input_report_abs(input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(input_dev, ABS_MT_POSITION_Y, y);
    input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, 10); // 触摸区域大小
    input_mt_sync(input_dev);
    input_sync(input_dev);

    // 短暂延迟模拟按压
    msleep(50);

    // 上报释放事件
    input_report_key(input_dev, BTN_TOUCH, 0);
    input_mt_sync(input_dev);
    input_sync(input_dev);

    pr_info("Touch simulation complete\n");
}

static int __init tap_module_init(void)
{
    // 使用kallsyms_lookup_name获取ts指针
    unsigned long ts_addr = kallsyms_lookup_name("ts");

    if (!ts_addr) {
        pr_err("Failed to find 'ts' symbol\n");
        return -ENODEV;
    }

    g_ts = (struct nvt_ts_data *)ts_addr;
    pr_info("Found ts pointer at %px\n", g_ts);

    // 模拟点击屏幕中央
    simulate_center_tap();

    return 0;
}

static void __exit tap_module_exit(void)
{
    pr_info("Tap module unloaded\n");
}

module_init(tap_module_init);
module_exit(tap_module_exit);
