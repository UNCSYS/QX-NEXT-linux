#ifndef UTILS_H
#define UTILS_H

#include <linux/types.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>

/**
 * generate_random_letters - 生成随机大小写字母字符串
 * @length: 字符串长度
 * @buffer: 输出缓冲区（可为NULL，由函数分配）
 *
 * 返回: 成功返回指向字符串的指针，失败返回NULL
 * 注意: 调用者负责释放返回的缓冲区（如果未传入预分配buffer）
 */
char *generate_random_letters(size_t length, char *buffer);

/**
 * split_string - 分割字符串，按分隔符拆分成数组
 * @str: 输入字符串
 * @delim: 分隔符（仅支持单字符）
 * @result: 返回的字符串数组（需调用者释放）
 * @count: 返回的数组长度
 *
 * 返回: 0 成功，<0 失败
 */
int split_string(const char *str, char delim, char ***result, int *count);

/**
 * startWith 检查字符串 是否以 特定字符串开头
 * @param str 输入字符串
 * @param prefix 需要匹配内容
 *
 * 返回: bool 是否匹配
 */
bool startWith(const char *str, const char *prefix);
/**
 * remove_lines - 高效移除包含指定字符串的所有行（适用于大文件）
 * @buffer: 输入/输出缓冲区
 * @size: 缓冲区大小
 * @str: 要匹配的字符串（如 "test_debug"）
 *
 * 注意: 删除的行不会保留换行符，避免输出空行
 */
void remove_lines(char *buffer, size_t size, const char *str);

/**
 * str_to_ul - 将字符串转换为 unsigned long
 * @str: 输入字符串（如 "0xffffffffc1234567"）
 * @out_val: 输出转换后的值
 *
 * 返回:
 *   0 - 成功
 *   -EINVAL - 无效的字符串格式
 */
int str_to_ul(const char *str, unsigned long *out_val);

#endif /* UTILS_H */
