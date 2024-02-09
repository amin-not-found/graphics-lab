/* Some utilities defined for use with raylib only */
#include <raylib.h>
#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

#ifdef PLATFORM_WEB
#define rayutl_mainloop(update, fps) emscripten_set_main_loop(update, fps, 1);
#else
#define rayutl_mainloop(update, fps)                                           \
    do {                                                                       \
        SetTargetFPS(fps);                                                     \
        while (!WindowShouldClose()) {                                         \
            update();                                                          \
        }                                                                      \
    } while (0);

#endif
