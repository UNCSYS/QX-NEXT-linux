#ifndef HIDE_PROCESS_H_
#define HIDE_PROCESS_H_

#include <linux/types.h> // 包含 pid_t 的定义

// 函数原型声明
void hide_process(pid_t pid_number);
void restore_process(pid_t pid_number);
void hide_exit(void);

#endif // HIDE_PROCESS_H_
