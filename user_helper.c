#include "user_helper.h"
#include "utils.h"

static bool kallsyms_ok;
//static bool run_shell_ok;
kallsyms_lookup_name_t my_kallsyms_lookup_name;

//Main
int call_usr(char **ret){
    char *new_ret;

    if(!kallsyms_ok){
        new_ret = kasprintf(GFP_KERNEL, "GET_KALLSYMS&kallsyms_lookup_name");
    }else{
        new_ret = kasprintf(GFP_KERNEL, "EXIT");
    }

    if (!new_ret) {
        return -ENOMEM;
    }

    // 释放原来的内存
    kfree(*ret);
    *ret = new_ret;
    return 0;
}


int usr_callback(const char* ret){
    char **tokens = NULL;
    int num_tokens = 0;
    int i = 0;
    int i2 = 0;

    //==
    unsigned long kallsyms_lookup_name_addr;
    unsigned long printk_addr;

        //back_getkallsyms_name
    if (startWith(ret, "CALL_BACK_GETKALLSYMS&kallsyms_lookup_name")) {
        if (split_string(ret, '&', &tokens, &num_tokens) == 0) {
            printk("[QXNEXT] Split result:\n");
            for (i = 0; i < num_tokens; i++) {
                printk("[QXNEXT] Token %d: %s\n", i, tokens[i]);
            }

            if (num_tokens >= 3 && str_to_ul(tokens[2], &kallsyms_lookup_name_addr) == 0) {
                my_kallsyms_lookup_name = (kallsyms_lookup_name_t)kallsyms_lookup_name_addr;
                printk_addr = my_kallsyms_lookup_name("printk");
                printk("[QXNEXT] ok!! kallsyms_lookup_name addr is %lx", kallsyms_lookup_name_addr);
                printk("[QXNEXT] ok!! printk addr is %lx", printk_addr);
                if(!kallsyms_ok){
                    usrhelper_ok_callback();

                }
                kallsyms_ok = true;
            } else {
                printk(KERN_ERR "[QXNEXT] Failed to convert address\n");
            }

                // 释放内存
            for (i2 = 0; i2 < num_tokens; i2++)
                kfree(tokens[i2]);
            kfree(tokens);
        }
    }
    return 0;
}
