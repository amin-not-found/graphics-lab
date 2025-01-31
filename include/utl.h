#ifndef COMMON_H
#define COMMON_H

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Setting allocation functions
#define UTL_FREE free
#define UTL_REALLOC realloc
#define UTL_ASSERT assert

#define UTL_DA_MIN_CAP 4

#define UTL_ANSI_RED "\x1b[31m"
#define UTL_ANSI_GREEN "\x1b[32m"
#define UTL_ANSI_YELLOW "\x1b[33m"
#define UTL_ANSI_BLUE "\x1b[34m"
#define UTL_ANSI_MAGENTA "\x1b[35m"
#define UTL_ANSI_CYAN "\x1b[36m"
#define UTL_ANSI_RESET "\x1b[0m"

/******* Logging *******/
typedef enum {
    UTL_DEBUG,
    UTL_INFO,
    UTL_WARNING,
    UTL_ERROR,
} utl_log_level;

// Wouldn't log stuff with lower priority
void utl_set_log_level(utl_log_level level);
void utl_log(utl_log_level level, const char *format, ...);

/******* Dynamic array *******
Works with a struct that looks like this:
struct {
  type *items;
  size_t capacity;
  size_t count;
}
*/

// you can initialize values yourself or use this macro
#define utl_da_init(a, size)                                                   \
    do {                                                                       \
        (a).count = 0;                                                         \
        (a).items = NULL;                                                      \
        utl_da_resize((a), (size) < UTL_DA_MIN_CAP ? UTL_DA_MIN_CAP : (size)); \
    } while (0);

#define utl_da_resize(a, new_size)                                            \
    do {                                                                      \
        (a).capacity = (new_size);                                            \
        (a).items =                                                           \
            UTL_REALLOC((a).items, (a).capacity * sizeof(*((a).items)));      \
        UTL_ASSERT(                                                           \
            (a).items != NULL && "Couldn't allocate memory for dynamic array" \
        );                                                                    \
        if ((a).count > (a).capacity) (a).count = (a).capacity;               \
    } while (0);

#define utl_da_append(a, item)                                 \
    do {                                                       \
        if ((a).capacity == 0) utl_da_init(a, UTL_DA_MIN_CAP); \
        if ((a).count >= (a).capacity) {                       \
            utl_da_resize(a, (a).capacity * 2);                \
        }                                                      \
        (a).items[(a).count++] = (item);                       \
    } while (0);

#define utl_da_append_many(a, new_items, items_count)              \
    do {                                                           \
        if ((a).count + (items_count) > (a).capacity) {            \
            if ((a).capacity == 0) utl_da_init(a, UTL_DA_MIN_CAP); \
            while ((a).count + (items_count) > (a).capacity) {     \
                (a).capacity *= 2;                                 \
            }                                                      \
            utl_da_resize(a, (a).capacity);                        \
        }                                                          \
        memcpy(                                                    \
            (a).items + (a).count, &(new_items),                    \
            (items_count) * sizeof(*((a).items))                   \
        );                                                         \
        (a).count += (items_count);                                \
    } while (0);

#define utl_da_free(a) UTL_FREE((a).items)

/******* Other utilities *******/
#define utl_array_size(a) (sizeof(a) / sizeof((a)[0]))
int utl_safe_wrap(int value, int max);

inline float util_wrap_angle( float angle );


#ifdef UTL_IMPLEMENTATION
// dont't show debug logs by default
#ifdef DEBUG
static utl_log_level log_level = UTL_DEBUG;
#else
static utl_log_level log_level = UTL_INFO;
#endif

void utl_set_log_level(utl_log_level level) { log_level = level; }
void utl_log(utl_log_level level, const char *format, ...) {
    if (level < log_level) return;

    switch (level) {
        case UTL_DEBUG:
            fprintf(stderr, UTL_ANSI_GREEN "[DEBUG] " UTL_ANSI_RESET);
            break;
        case UTL_INFO:
            fprintf(stderr, "[INFO] ");
            break;
        case UTL_WARNING:
            fprintf(stderr, UTL_ANSI_YELLOW "[WARNING] " UTL_ANSI_RESET);
            break;
        case UTL_ERROR:
            fprintf(stderr, UTL_ANSI_RED "ERROR " UTL_ANSI_RESET);
            break;
        default:
            utl_log(
                UTL_ERROR, "{%d} utl log level doesn't exist.\n", (int)level
            );
            exit(-1);
    }
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

int utl_safe_wrap(int value, int max) { return ((value % max) + max) % max; }

inline float util_wrap_angle( float angle )
{
    double twoPi = 2.0 * PI;
    return angle - twoPi * floor( angle / twoPi );
}

#endif  // end of UTL_IMPLEMENTATION

#endif  // end of COMMON_H header guard