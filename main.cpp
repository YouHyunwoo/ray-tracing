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
    void CreateBlock(const Vector3& v) {
        CreateBlock(v.x, v.y, v.z);
    }

    void DeleteBlock(int x, int y, int z) {
        _blocks[z][y][x] = BlockState::kNone;
    }
    void DeleteBlock(const Vector3& v) {
        DeleteBlock(v.x, v.y, v.z);
    }

    bool HasBlock(int x, int y, int z) const {
        return _blocks[z][y][x] == BlockState::kExist;
    }
    bool HasBlock(const Vector3& v) const {
        return HasBlock(v.x, v.y, v.z);
    }

    bool IsInBounds(double x, double y, double z) const {
        return (
            0 <= x && x < kWidth &&
            0 <= y && y < kHeight &&
            0 <= z && z < kDepth
        );
    }
    bool IsInBounds(const Vector3& v) const {
        return IsInBounds(v.x, v.y, v.z);
    }
};

class View {
public:
    double yaw, pitch;

    View(double yaw, double pitch) : yaw(yaw), pitch(pitch) {}

    Vector3 ToDirection() const {
        return {
            cos(yaw + M_PI_2) * cos(pitch),
            sin(pitch),
            sin(yaw + M_PI_2) * cos(pitch),
        };
    }
};

class Camera {
public:
    Vector3 position = { 5, 5.5, 5 };
    View view = { 0, 0 };
    double move_speed = 5.0f, tilt_speed = 2.0f;
    Vector3 forward, right, up;
};

class Player {
    World& _world;
public:
    Vector3 position = { 5, 5.5, 5 };
    View view = { 0, 0 };
    Vector3 velocity = { 0, 0, 0 };
    bool is_jumping = false;
    double move_speed = 5.0;
    double tilt_speed = 2.0;
    Vector3 forward, right, up;

    Player(World& world) : _world(world), view(0, 0) {}

    void Update(double delta_time) {
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
};

struct Ray {
    Vector3 position, direction;
};

struct Hit {
    Vector3 point, normal;
};

class RayTracing {
public:
    bool IsHitBlock(const Vector3& point, const World& world) const {
        return world.HasBlock((int)point.x, (int)point.y, (int)point.z);
    }

    bool IsHitBorderOfBlock(const Vector3& point) const {
        size_t count = 0;
        if (fabs(round(point.x) - point.x) < kBlockBorderWidth) count++;
        if (fabs(round(point.y) - point.y) < kBlockBorderWidth) count++;
        if (fabs(round(point.z) - point.z) < kBlockBorderWidth) count++;
        return count >= 2;
    }

    bool IsInWorld(const Vector3& point, const World& world) const {
        return world.IsInBounds(point.x, point.y, point.z);
    }

    bool CastRay(const Ray& ray, Hit* hit, double max_distance, const World& world) const {
        Vector3 hit_position;
        Vector3 hit_normal;
        double hit_dist;
        double cumulative_dist = 0.0;

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

                cumulative_dist += dist;
                if (max_distance < cumulative_dist)
                    return false;
                
                position = position + ray.direction * (dist + kRayTracingForwardingFactor);
            }
        }

        return false;
    }
};

class Play {
private:
    bool _is_block_selected;
    Vector3 _selected_block_position;
    Vector3 _selected_block_normal;
    
    RayTracing _ray_tracing;
    World _world;
    Camera _camera;
    Player _player;
public:
    Play(int worldWidth, int worldHeight, int worldDepth) : _world(worldWidth, worldHeight, worldDepth), _player(_world) {}

    void Initialize();

    void Update(double delta_time);
    void UpdateInput();
    void UpdateBlockSelection();
    void UpdatePlayerMovement(double delta_time);
    void UpdateCamera();

    void Render();
    void RenderWithRayTracing();

    void Dispose();
};

Screen screen(kScreenWidth, kScreenHeight);
Loop loop;
Play play(kWorldWidth, kWorldHeight, kWorldDepth);

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
    UpdatePlayerMovement(delta_time);
    _player.Update(delta_time);
    UpdateCamera();
}

void Play::UpdateInput() {
    if (IsKeyDown(VK_CAPITAL)) {
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
    bool is_hit = _ray_tracing.CastRay(ray, &hit, 3, _world);
    _is_block_selected = is_hit;
    if (is_hit) {
        _selected_block_position = hit.point - hit.normal * 0.5;
        _selected_block_normal = hit.normal;
    }
}

void Play::UpdatePlayerMovement(double delta_time) {
    bool is_moved = false;
    Vector3 next_position = _player.position;

    if (IsKeyPressed('W')) {
        is_moved = true;
        next_position.x = _player.position.x + cos(_player.view.yaw + M_PI_2) * _player.move_speed * delta_time;
        next_position.z = _player.position.z + sin(_player.view.yaw + M_PI_2) * _player.move_speed * delta_time;
    }
    if (IsKeyPressed('S')) {
        is_moved = true;
        next_position.x = _player.position.x - cos(_player.view.yaw + M_PI_2) * _player.move_speed * delta_time;
        next_position.z = _player.position.z - sin(_player.view.yaw + M_PI_2) * _player.move_speed * delta_time;
    }

    if (IsKeyPressed('A')) {
        is_moved = true;
        next_position.x = _player.position.x - cos(_player.view.yaw) * _player.move_speed * delta_time;
        next_position.z = _player.position.z - sin(_player.view.yaw) * _player.move_speed * delta_time;
    }
    if (IsKeyPressed('D')) {
        is_moved = true;
        next_position.x = _player.position.x + cos(_player.view.yaw) * _player.move_speed * delta_time;
        next_position.z = _player.position.z + sin(_player.view.yaw) * _player.move_speed * delta_time;
    }

    if (is_moved) {
        Vector3 toward = next_position - _player.position;
        Vector3 direction = toward.Normalize();
        Ray ray = { _player.position, direction };
        Hit hit;
        bool is_hit = _ray_tracing.CastRay(ray, &hit, 0.5 + _player.move_speed * delta_time, _world);
        if (is_hit) {
            hit.point = hit.point - direction * 0.5;
            Vector3 remainder = next_position - hit.point;
            _player.position = hit.point + remainder - hit.normal * remainder.Dot(hit.normal);
        }
        else {
            _player.position = next_position;
        }
    }
    
    if (IsKeyPressed('J')) {
        _player.view.yaw += _player.tilt_speed * delta_time;
    }
    if (IsKeyPressed('L')) {
        _player.view.yaw -= _player.tilt_speed * delta_time;
    }
    if (IsKeyPressed('I')) {
        _player.view.pitch = fmin(_player.view.pitch + _player.tilt_speed * delta_time, 50.0 * kDegreeToRadian);
    }
    if (IsKeyPressed('K')) {
        _player.view.pitch = fmax(_player.view.pitch - _player.tilt_speed * delta_time, -45.0 * kDegreeToRadian);
    }

    if (IsKeyPressed(VK_SPACE)) {
        _player.is_jumping = true;
        _player.velocity = up * 0.2;
    }

    if (_player.is_jumping) {
        _player.velocity.y += -0.01;
        if (_player.velocity.y < 0)
        {
            Ray ray = { _player.position, down };
            Hit hit;
            bool is_hit = _ray_tracing.CastRay(ray, &hit, _player.velocity.y + 1, _world);
            if (is_hit) {
                _player.position = hit.point + up * 0.5;
                _player.is_jumping = false;
            }
            else {
                _player.position.y += _player.velocity.y;
            }
        }
        else {
            _player.position.y += _player.velocity.y;
        }
    }
}

void Play::UpdateCamera() {
    _camera.position = _player.position;
    _camera.view = _player.view;
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
    Vector3 world_size = { (double)_world.kWidth, (double)_world.kWidth, (double)_world.kWidth };
    double max_distance = world_size.Magnitude();
    for (int r = 0; r < screen.height; r++) {
        Vector3 t = direction;

        for (int c = 0; c < screen.width; c++) {
            ray.direction = direction;

            screen.SaveContext();

            bool is_hit = _ray_tracing.CastRay(ray, &hit, max_distance, _world);
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
