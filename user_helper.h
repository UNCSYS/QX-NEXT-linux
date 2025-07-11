#ifndef USER_HELPER_H
#define USER_HELPER_H

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);
extern kallsyms_lookup_name_t my_kallsyms_lookup_name;

//外部函数
extern void usrhelper_ok_callback(void);

int call_usr(char **ret);
int usr_callback(const char* ret);


#endif // USER_HELPER_H
