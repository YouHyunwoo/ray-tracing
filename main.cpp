#define _USE_MATH_DEFINES
#include <math.h>
#include "loop.h"
#include "screen.h"
#include "geometry.h"
#include "input.h"

constexpr int kScreenWidth = 200 * 2;
constexpr int kScreenHeight = 50 * 2;

constexpr int kWorldWidth = 20;
constexpr int kWorldHeight = 20;
constexpr int kWorldDepth = 20;

constexpr double kBlockBorderWidth = 0.1;

constexpr double kRadianToDegree = 180.0 / M_PI;
constexpr double kDegreeToRadian = M_PI / 180.0;
constexpr double kCameraFOVDegree = 90.0;
constexpr double kCameraFOV = kCameraFOVDegree * kDegreeToRadian;
constexpr double kHalfOfCameraFOV = kCameraFOV / 2.0;

constexpr double kRayTracingMinimumDistance = 2.0;
constexpr double kRayTracingDetectingFactor = 0.001;
constexpr double kRayTracingForwardingFactor = 0.001;

class View {
public:
    float yaw, pitch;
    View(float yaw, float pitch) : yaw(yaw), pitch(pitch) {} 
    Vector3 ToDirection() const {
        return {
            cos(yaw + M_PI_2) * cos(pitch),
            sin(pitch),
            sin(yaw + M_PI_2) * cos(pitch),
        };
    }
};

enum class BlockState {
    kNone,
    kExist,
};

class World {
private:
    BlockState*** _blocks;
public:
    const int kWidth, kHeight, kDepth;

    World(int width, int height, int depth) : kWidth(width), kHeight(height), kDepth(depth) {
        _blocks = new BlockState**[kDepth];
        for (int d = 0; d < kDepth; d++) {
            _blocks[d] = new BlockState*[kHeight];
            for (int r = 0; r < kHeight; r++) {
                _blocks[d][r] = new BlockState[kWidth];
            }
        }
    }

    ~World() {
        for (int d = 0; d < kDepth; d++) {
            for (int r = 0; r < kHeight; r++) {
                delete[] _blocks[d][r];
            }
            delete[] _blocks[d];
        }
        delete[] _blocks;
    }

    void CreateBlock(int x, int y, int z) {
        _blocks[z][y][x] = BlockState::kExist;
    }

    void DeleteBlock(int x, int y, int z) {
        _blocks[z][y][x] = BlockState::kNone;
    }

    bool HasBlock(int x, int y, int z) const {
        return _blocks[z][y][x] == BlockState::kExist;
    }

    bool IsInBounds(float x, float y, float z) const {
        return 0 <= x && x < kWidth &&
            0 <= y && y < kHeight &&
            0 <= z && z < kDepth;
    }
};

class Camera {
public:
    Vector3 position = { 5, 6, 5 };
    View view = { 0, 0 };
    Vector3 forward, right, up;
    float move_speed = 5.0f, tilt_speed = 2.0f;

    void Update(double delta_time);
};

struct Ray {
    Vector3 position, direction;
};

struct Hit {
    Vector3 point, normal;
};

class RayTracing {
public:
    bool IsHitBlock(const Vector3& point, const World& world) {
        return world.HasBlock((int)point.x, (int)point.y, (int)point.z);
    }

    bool IsHitBorderOfBlock(const Vector3& point) {
        int count = 0;
        if (fabs(round(point.x) - point.x) < kBlockBorderWidth) count++;
        if (fabs(round(point.y) - point.y) < kBlockBorderWidth) count++;
        if (fabs(round(point.z) - point.z) < kBlockBorderWidth) count++;
        return count >= 2;
    }

    bool IsInWorld(const Vector3& point, const World& world) {
        return world.IsInBounds(point.x, point.y, point.z);
    }

    bool CastRay(const Ray& ray, Hit* hit, const World& world) {
        Vector3 hit_position;
        Vector3 hit_normal;
        double hit_dist;

        Vector3 position = ray.position;
        while (IsInWorld(position, world)) {
            if (IsHitBlock(position, world)) {
                if (hit != nullptr) {
                    hit->point = hit_position + ray.direction * hit_dist;
                    hit->normal = hit_normal;
                }
                return true;
            }

            {
                double dist = kRayTracingMinimumDistance;

                if (ray.direction.x > kRayTracingDetectingFactor) {
                    double step = (ceil(position.x) - position.x) / ray.direction.x;
                    if (dist > step) {
                        dist = step;
                        hit_position = position;
                        hit_dist = dist;
                        hit_normal = left;
                    }
                }
                else if (ray.direction.x < -kRayTracingDetectingFactor) {
                    double step = (floor(position.x) - position.x) / ray.direction.x;
                    if (dist > step) {
                        dist = step;
                        hit_position = position;
                        hit_dist = dist;
                        hit_normal = right;
                    }
                }
                if (ray.direction.y > kRayTracingDetectingFactor) {
                    double step = (ceil(position.y) - position.y) / ray.direction.y;
                    if (dist > step) {
                        dist = step;
                        hit_position = position;
                        hit_dist = dist;
                        hit_normal = down;
                    }
                }
                else if (ray.direction.y < -kRayTracingDetectingFactor) {
                    double step = (floor(position.y) - position.y) / ray.direction.y;
                    if (dist > step) {
                        dist = step;
                        hit_position = position;
                        hit_dist = dist;
                        hit_normal = up;
                    }
                }
                if (ray.direction.z > kRayTracingDetectingFactor) {
                    double step = (ceil(position.z) - position.z) / ray.direction.z;
                    if (dist > step) {
                        dist = step;
                        hit_position = position;
                        hit_dist = dist;
                        hit_normal = back;
                    }
                }
                else if (ray.direction.z < -kRayTracingDetectingFactor) {
                    double step = (floor(position.z) - position.z) / ray.direction.z;
                    if (dist > step) {
                        dist = step;
                        hit_position = position;
                        hit_dist = dist;
                        hit_normal = forward;
                    }
                }
                
                position = position + ray.direction * (dist + kRayTracingForwardingFactor);
            }
        }

        return false;
    }
};

class Play {
private:
    bool _is_block_selected;
    
    RayTracing _ray_tracing;
    World _world;
public:
    Vector3 _selected_block_position;
    Vector3 _selected_block_normal;
    Camera _camera;
    Play() : _world(kWorldWidth, kWorldHeight, kWorldDepth) {}

    void Initialize();

    void Update(double delta_time);
    void UpdateInput();
    void UpdateBlockSelection();

    void Render();
    void RenderWithRayTracing();

    void Dispose();
};

Screen screen(kScreenWidth, kScreenHeight);
Loop loop;
Play play;

void print_vec(int x, int y, const Vector3& v) {
    screen.DrawTextWithFormat(x, y, "%f %f %f", v.x, v.y, v.z);
}

// * --------------------------------------------------------------------

void Camera::Update(double delta_time) {
    if (IsKeyPressed('W')) {
        position.x += cos(view.yaw + M_PI_2) * move_speed * delta_time;
        position.z += sin(view.yaw + M_PI_2) * move_speed * delta_time;
    }
    if (IsKeyPressed('S')) {
        position.x -= cos(view.yaw + M_PI_2) * move_speed * delta_time;
        position.z -= sin(view.yaw + M_PI_2) * move_speed * delta_time;
    }
    if (IsKeyPressed('A')) {
        position.x -= cos(view.yaw) * move_speed * delta_time;
        position.z -= sin(view.yaw) * move_speed * delta_time;
    }
    if (IsKeyPressed('D')) {
        position.x += cos(view.yaw) * move_speed * delta_time;
        position.z += sin(view.yaw) * move_speed * delta_time;
    }
    if (IsKeyPressed('J')) {
        view.yaw += tilt_speed * delta_time;
    }
    if (IsKeyPressed('L')) {
        view.yaw -= tilt_speed * delta_time;
    }
    if (IsKeyPressed('I')) {
        view.pitch += tilt_speed * delta_time;
    }
    if (IsKeyPressed('K')) {
        view.pitch -= tilt_speed * delta_time;
    }

    forward = view.ToDirection().Normalize();

    float old_yaw = view.yaw;
    view.yaw -= M_PI_2;
    right = view.ToDirection().Normalize();
    view.yaw = old_yaw;

    float old_pitch = view.pitch;
    view.pitch += M_PI_2;
    up = view.ToDirection().Normalize();
    view.pitch = old_pitch;
}

void Play::Initialize() {
    for (int d = 0; d < _world.kDepth; d++)
        for (int r = 0; r < 5; r++)
            for (int c = 0; c < _world.kWidth; c++)
                _world.CreateBlock(c, r, d);

    _world.CreateBlock(5, 6, 8);
}

void Play::Update(double delta_time) {
    UpdateInput();
    UpdateBlockSelection();
    _camera.Update(delta_time);

    Vector3 cam_pos = _camera.position;
    while (true) {
        if (cam_pos.y <= _world.kHeight - 2 && _world.HasBlock(cam_pos.x, cam_pos.y - 1, cam_pos.z)) {
            cam_pos.y++;
        }
        else if (cam_pos.y >= 1 && !_world.HasBlock(cam_pos.x, cam_pos.y - 2, cam_pos.z)) {
            cam_pos.y--;
        }
        else break;
    }
    _camera.position = cam_pos;
}

void Play::UpdateInput() {
    if (IsKeyDown(VK_SPACE)) {
        if (!_is_block_selected) return;
        Vector3 block_position = (_selected_block_position + _selected_block_normal).Floor();
        int x = (int)block_position.x;
        int y = (int)block_position.y;
        int z = (int)block_position.z;
        _world.CreateBlock(x, y, z);
    }
    else if (IsKeyDown(VK_OEM_1)) {
        if (!_is_block_selected) return;
        int x = (int)_selected_block_position.x;
        int y = (int)_selected_block_position.y;
        int z = (int)_selected_block_position.z;
        _world.DeleteBlock(x, y, z);
    }
    else if (IsKeyDown(VK_OEM_3))
        loop.Quit();
}

void Play::UpdateBlockSelection() {
    Hit hit;
    Ray ray = { _camera.position, _camera.view.ToDirection() };
    bool is_hit = _ray_tracing.CastRay(ray, &hit, _world);
    _is_block_selected = is_hit;
    if (is_hit) {
        _selected_block_position = hit.point - hit.normal * 0.5;
        _selected_block_normal = hit.normal;
    }
}

void Play::Render() {
    RenderWithRayTracing();
}

void Play::RenderWithRayTracing() {
    // horizontal
    _camera.view.yaw += kHalfOfCameraFOV;
    Vector3 screen_left = _camera.view.ToDirection();
    _camera.view.yaw -= kCameraFOV;
    Vector3 screen_right = _camera.view.ToDirection();
    _camera.view.yaw += kHalfOfCameraFOV;
    Vector3 left_to_right = screen_right - screen_left;

    // vertical
    _camera.view.pitch += kHalfOfCameraFOV / 2;
    Vector3 screen_top = _camera.view.ToDirection();
    _camera.view.pitch -= kCameraFOV / 2;
    Vector3 screen_bottom = _camera.view.ToDirection();
    _camera.view.pitch += kHalfOfCameraFOV / 2;
    Vector3 top_to_bottom = screen_bottom - screen_top;

    Vector3 screen_center = _camera.view.ToDirection();
    Vector3 screen_left_top = screen_top + screen_left - screen_center;

    // ray casting
    Vector3 horizontal_step = left_to_right * 1.0 / screen.width;
    Vector3 vertical_step = top_to_bottom * 1.0 / screen.height;

    Hit hit;
    Ray ray = { _camera.position };
    Vector3 direction = screen_left_top;
    for (int r = 0; r < screen.height; r++) {
        Vector3 t = direction;

        for (int c = 0; c < screen.width; c++) {
            ray.direction = direction;

            screen.SaveContext();

            bool is_hit = _ray_tracing.CastRay(ray, &hit, _world);
            if (is_hit) {
                bool is_border_of_block = _ray_tracing.IsHitBorderOfBlock(hit.point);
                screen.SetCharacter(is_border_of_block ? '.' : '#');

                if (_is_block_selected) {
                    bool is_selected_block = (hit.point - hit.normal * 0.5).Floor() == _selected_block_position.Floor();

                    if (is_selected_block) {
                        bool is_selected_surface = hit.normal.Round() == _selected_block_normal.Round();
                        if (!is_selected_surface)
                            screen.SetDimMode();
                        screen.SetForegroundColor(Color::kGreen);
                    }
                }
            }
            else
                screen.SetCharacter(' ');
            screen.DrawPoint(c, r);

            screen.RestoreContext();

            direction = direction + horizontal_step;
        }

        direction = t + vertical_step;
    }
}

void Play::Dispose() { }

void init() {
    play.Initialize();
}
void update(double delta_time) {
    play.Update(delta_time);
}
void render() {
    play.Render();

    screen.DrawTextWithFormat(0, 20, "%d", GetAsyncKeyState(VK_SPACE));
}
void dispose() {
    play.Dispose();
}

int main() {
    loop.Start(
        &screen,
        init,
        update,
        render,
        dispose
    );

    return 0;
}
