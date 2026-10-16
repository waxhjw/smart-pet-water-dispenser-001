#ifndef APP_TYPES_H
#define APP_TYPES_H

/*
 * 这个项目的 MCU 资源很小，所以业务层不用动态内存、不依赖标准库大对象。
 * app_u16 在目标 MCU 上按 16 位使用，避免和编译器 int 宽度绑定。
 */
typedef unsigned char app_u8;
typedef unsigned short app_u16;
typedef unsigned long app_u32;

typedef enum {
    APP_FALSE = 0,
    APP_TRUE = 1
} app_bool_t;

#endif
