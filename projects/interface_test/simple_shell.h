#ifndef SIMPLE_SHELL_H
#define SIMPLE_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define SHELL_BUF_LEN 512

//#define MINICOM_SHELL 1     // Define if using with minicom shell, or comment 
                            // out when using with other microcontroller

typedef enum 
{
    INVALID_CMD,
    BLINK,
    ML,
    FLIR 
} shell_cmd; 

void simple_shell();

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_SHELL_H */
/*** end of file ***/
