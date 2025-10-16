#ifndef _LOOP_H_
#define _LOOP_H_

#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <conio.h>
#include "screen.h"

// #define LIMIT_FPS

class Loop {
private:
    Screen* _screen;
    bool _is_running = false;
    clock_t _current_clock, _last_clock;
    double _delta_time; // sec
    int _frame_count_for_fps;
    clock_t _clock_for_fps;
    
    void (*_user_init)();
    void (*_user_update)(double);
    void (*_user_render)();
    void (*_user_dispose)();
    
    void Initialize(
        Screen* screen,
        void (*user_init)(),
        void (*user_update)(double),
        void (*user_render)(),
        void (*user_dispose)()
    );
    void Update();
    void UpdateTick();
    void UpdateInput();
    void Render();
    void Dispose();
    
public:
    uint64_t frame_count;
    float fps;
    bool is_fps_visible = true;
    uint8_t delay = 20;
    char keystate[256];

    void Start(
        Screen* screen,
        void (*user_init)(),
        void (*user_update)(double),
        void (*user_render)(),
        void (*user_dispose)()
    );

    void Quit();
};

#endif