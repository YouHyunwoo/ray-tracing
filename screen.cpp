#include "screen.h"

#include <memory>
#include <string>
#include <stdexcept>

bool Screen::IsInBounds(int x, int y) {
    return 0 <= x && x < width &&
        0 <= y && y < height;
}

Screen::Screen(uint32_t width, uint32_t height) :
    width(width), height(height),
    area(width * height),
    half_width(width / 2), half_height(height / 2),
    aspect_ratio(height / (float)width) {
    _buffer = new BufferCell*[height];
    for (int r = 0; r < height; r++)
        _buffer[r] = new BufferCell[width]; // TODO: default value check
}

Screen::~Screen() {
    for (int r = 0; r < height; r++)
        delete[] _buffer[r];
    delete[] _buffer;
}

void Screen::ClearScreen() { printf("\x1b[J"); }

void Screen::ClearBuffer() {
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            _buffer[r][c].is_empty = true;
            _buffer[r][c].character = ' ';
            _buffer[r][c].foreground_color = Color::kDefault;
            _buffer[r][c].background_color = Color::kBackgroundDefault;
            _buffer[r][c].z_index = 0;
        }
    }
}

void Screen::RenderBuffer() {
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            auto& cell = _buffer[r][c];
            if (cell.is_empty) { putchar(' '); continue; }
            
            printf("\x1b[%s;%d;%dm%c", (cell.is_dim ? "2" : "22"), cell.foreground_color, cell.background_color, cell.character);
        }
        putchar('\n');
    }
}

bool Screen::IsBufferEmpty(int x, int y) {
    return IsInBounds(x, y) && _buffer[y][x].is_empty;
}

void Screen::DrawPoint(int x, int y) {
    if (!IsInBounds(x, y)) return;

    _buffer[y][x].is_empty = false;
    _buffer[y][x].character = _context.character;
    _buffer[y][x].is_dim = _context.is_dim;
    _buffer[y][x].foreground_color = _context.foreground_color;
    _buffer[y][x].background_color = _context.background_color;
    _buffer[y][x].z_index = _context.z_index;
}

void Screen::DrawPointWithZIndex(int x, int y, float z_index) {
    if (!IsInBounds(x, y)) return;

    if (_buffer[y][x].is_empty || _buffer[y][x].z_index < z_index) {
        _buffer[y][x].is_empty = false;
        _buffer[y][x].character = _context.character;
        _buffer[y][x].is_dim = _context.is_dim;
        _buffer[y][x].foreground_color = _context.foreground_color;
        _buffer[y][x].background_color = _context.background_color;
        _buffer[y][x].z_index = z_index;
    }
}

void Screen::DrawTextWithFormat(int x, int y, const char* format, ...) {
    if (!IsInBounds(x, y)) return;

    va_list args;
    va_start(args, format);

    int len = vsnprintf(nullptr, 0, format, args);
    if (len <= 0) { throw std::runtime_error( "Error during formatting." ); }
    char* buf = new char[len + 1];
    vsnprintf(buf, len + 1, format, args);
    for (int i = 0, xi = x, yi = y; i < len; i++) {
        _buffer[yi][xi].is_empty = false;
        _buffer[yi][xi].character = buf[i];
        _buffer[yi][xi].is_dim = _context.is_dim;
        _buffer[yi][xi].foreground_color = _context.foreground_color;
        _buffer[yi][xi].background_color = _context.background_color;
        _buffer[yi][xi].z_index = _context.z_index;
        if (++xi >= width) {
            xi = 0;
            if (++yi >= height) break;
        }
    }
    
    va_end(args);
}

void Screen::ReturnCursor() { printf("\x1b[H"); }

void Screen::MoveCursor(int x, int y) { printf("\x1b[%d;%dH", y, x); }

void Screen::SetCharacter(char character) { _context.character = character; }
void Screen::ResetCharacter() { _context.character = ' '; }
void Screen::SetDimMode() { _context.is_dim = true; }
void Screen::ResetDimMode() { _context.is_dim = false; }
void Screen::SetForegroundColor(Color color) { _context.foreground_color = color; }
void Screen::ResetForegroundColor() { _context.foreground_color = Color::kDefault; }
void Screen::SetBackgroundColor(Color color) { _context.background_color = color; }
void Screen::ResetBackgroundColor() { _context.background_color = Color::kBackgroundDefault; }
void Screen::SetZIndex(float z_index) { _context.z_index = z_index; }
void Screen::ResetZIndex() { _context.z_index = 0; }

void Screen::SaveContext() {
    _context_stack.push(_context);
}
void Screen::RestoreContext() {
    _context = _context_stack.top();
    _context_stack.pop();
}