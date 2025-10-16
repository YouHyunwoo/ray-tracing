#include <windows.h>

char keystate[256];

bool IsKeyPressed(int vkey) {
    bool pressed = GetAsyncKeyState(vkey) != 0;
    keystate[vkey] = pressed;
    return pressed;
}

bool IsKeyDown(int vkey) {
    bool pressed = GetAsyncKeyState(vkey) != 0;
    bool result = !keystate[vkey] && pressed;
    keystate[vkey] = pressed;
    return result;
}

bool IsKeyUp(int vkey) {
    bool pressed = GetAsyncKeyState(vkey) != 0;
    bool result = keystate[vkey] && !pressed;
    keystate[vkey] = pressed;
    return result;
}