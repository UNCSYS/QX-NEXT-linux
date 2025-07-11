#include <linux/random.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "utils.h"

/**
 * generate_random_letters - 生成随机大小写字母字符串
 * @length: 字符串长度
 * @buffer: 输出缓冲区（可为NULL，由函数分配）
 *
 * 返回: 成功返回指向字符串的指针，失败返回NULL
 * 注意: 调用者负责释放返回的缓冲区（如果未传入预分配buffer）
 */
char *generate_random_letters(size_t length, char *buffer){
    size_t i;
    unsigned int rand_val;
    char *buf = buffer ? buffer : kmalloc(length + 1, GFP_KERNEL);

    if (!buf)
        return NULL;

    for (i = 0; i < length; i++) {
        get_random_bytes(&rand_val, sizeof(rand_val));

        /* 方法1：直接生成字母（更高效） */
        // 随机选择大小写（利用ASCII码规律）
        buf[i] = (rand_val % 2) ?
                 ('A' + (rand_val % 26)) :  // 大写字母
                 ('a' + (rand_val % 26));    // 小写字母

        /* 方法2：通过位运算避免取模（可选优化） */
        // buf[i] = 'A' + (rand_val & 0x1F);  // 仅生成大写字母
        // buf[i] |= (rand_val & 0x20);       // 随机大小写
    }
    buf[length] = '\0'; // 确保字符串终止

    return buf;
}

#include <linux/kmod.h>
#include <linux/string.h>
//#include <stdarg.h>

/**
 * startWith 检查字符串 是否以 特定字符串开头
 * @param str 输入字符串
 * @param prefix 需要匹配内容
 *
 * 返回: bool 是否匹配
 */
bool startWith(const char *str, const char *prefix) {
    size_t len;
    if (!str || !prefix) return false;
    len = strlen(prefix);
    return strncmp(str, prefix, len) == 0;
}

/**
 * 分割字符串，按分隔符拆分成数组
 * @param str 输入字符串
 * @param delim 分隔符（仅支持单字符）
 * @param result 返回的字符串数组（需调用者释放）
 * @param count 返回的数组长度
 * @return 0 成功，<0 失败
 */
int split_string(const char *str, char delim, char ***result, int *count) {
    char *str_copy, *token_start, *token_end;
    int token_count = 0;
    int i;

    // 1. 计算 token 数量
    str_copy = kstrdup(str, GFP_KERNEL);
    if (!str_copy)
        return -ENOMEM;

    token_start = str_copy;
    while (*token_start != '\0') {
        token_end = strchr(token_start, delim);
        if (!token_end)
            break;
        token_count++;
        token_start = token_end + 1;
    }
    token_count++; // 最后一个 token

    // 2. 分配数组内存
    *result = kmalloc_array(token_count, sizeof(char *), GFP_KERNEL);
    if (!*result) {
        kfree(str_copy);
        return -ENOMEM;
    }

    // 3. 填充数组
    token_start = str_copy;
    for (i = 0; i < token_count; i++) {
        token_end = strchr(token_start, delim);
        if (token_end)
            *token_end = '\0'; // 替换分隔符为字符串结束符

        (*result)[i] = kstrdup(token_start, GFP_KERNEL);
        if (!(*result)[i]) {
            // 分配失败，回滚
            while (i-- > 0)
                kfree((*result)[i]);
            kfree(*result);
            kfree(str_copy);
            return -ENOMEM;
        }

        if (!token_end)
            break; // 已处理完所有 token
        token_start = token_end + 1;
    }

    *count = token_count;
    kfree(str_copy);
    return 0;
}

/**
 * @brief 高效移除包含指定字符串的所有行（适用于大文件）
 * @param buffer 输入/输出缓冲区
 * @param size 缓冲区大小
 * @param str 要匹配的字符串（如 "test_debug"）
 * @note 删除的行不会保留换行符，避免输出空行
 */
void __maybe_unused remove_lines(char *buffer, size_t size, const char *str) {
    char *src = buffer;    // 当前读取位置
    char *dst = buffer;    // 当前写入位置
    const size_t str_len = strlen(str);
    bool modified = false;

    while (*src) {
        char *line_end = strchrnul(src, '\n');  // 找到行尾（或字符串末尾）
        const size_t line_len = line_end - src;

        // 检查当前行是否包含目标字符串
        bool keep_line = true;
        if (line_len >= str_len) {
            // 手动实现 strstr 以减少函数调用开销
            const char *p = src;
            const char *end = src + line_len - str_len + 1;
            while (p < end) {
                if (memcmp(p, str, str_len) == 0) {
                    keep_line = false;
                    break;
                }
                p++;
            }
        }

        if (keep_line) {
            // 仅当之前有删除行时才移动数据
            if (modified) {
                memmove(dst, src, line_len);
            }
            dst += line_len;

            // 处理换行符（仅当保留该行时）
            if (*line_end == '\n') {
                if (modified) {
                    *dst++ = '\n';  // 写入换行符
                } else {
                    dst++;  // 未修改，直接前进
                }
                line_end++;
            }
        } else {
            modified = true;  // 标记已删除行（包括换行符）
            // 跳过换行符（如果存在）
            if (*line_end == '\n') {
                line_end++;
            }
        }

        src = line_end;  // 跳到下一行
    }

    // 终止字符串
    *dst = '\0';
}
#include <linux/kernel.h>
#include <linux/errno.h> // 错误码定义

/**
 * str_to_ul() - 将字符串转换为 unsigned long
 * @str: 输入字符串（如 "0xffffffffc1234567"）
 * @out_val: 输出转换后的值
 *
 * 返回值：
 *   0 - 成功
 *   -EINVAL - 无效的字符串格式
 */
int str_to_ul(const char *str, unsigned long *out_val)
{
    int ret;

    if (!str || !out_val)
        return -EINVAL;

    // 使用 kstrtoul 自动检测进制（0x=16进制，0=8进制，其他=10进制）
    ret = kstrtoul(str, 0, out_val);
    if (ret) {
        pr_err("Invalid address string: '%s'\n", str);
        return -EINVAL;
    }

    return 0; // 成功
}
