#ifndef CHOOSE_CHAR_H
#define CHOOSE_CHAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define CHOOSE_CHAR_WIDTH   160U
#define CHOOSE_CHAR_HEIGHT  120U
#define CHOOSE_CHAR_SIZE    38400U

extern const uint8_t choose_char_map[CHOOSE_CHAR_SIZE];

#ifdef __cplusplus
}
#endif

#endif /* CHOOSE_CHAR_H */
