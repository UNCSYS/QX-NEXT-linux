obj-$(CONFIG_QXNEXT) += QXNEXT.o
QXNEXT-objs := entry_dev.o
QXNEXT-objs += dev_service.o
QXNEXT-objs += user_helper.o
#QXNEXT-objs += hide_process.o
#QXNEXT-objs += mmu_manipulator.o
QXNEXT-objs += memory.o
QXNEXT-objs += utils.o



#设置取消堆栈保护
EXTRA_CFLAGS += -fno-unwind-tables -fno-asynchronous-unwind-tables -g0 # 减少异常处理表
EXTRA_LDFLAGS += -s # 去除符号表
#CFLAGS_MODULE += -fomit-frame-pointer # 省略帧指针
CFLAGS_MODULE += -O2                # 优化等级2
CFLAGS_MODULE += -fvisibility=hidden  # 隐藏符号
CFLAGS_MODULE += -fno-stack-protector
CFLAGS_MODULE += -fno-sanitize=cfi
KBUILD_CFLAGS += -fno-sanitize=cfi
KBUILD_CFLAGS += -fno-sanitize-cfi-cross-dso
KBUILD_CFLAGS += -fno-sanitize=shadow-call-stack

ifeq ($(strip $(CONFIG_QXNEXT)),y)
$(info -- QXNEXT: driver enabled!)
else
$(info -- QXNEXT: driver disabled)
ccflags-y += -DCONFIG_KSU_KPROBES_HOOK
endif
