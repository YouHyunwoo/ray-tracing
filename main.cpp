#include <string>
#include <cstring>
#include <vector>
#include <windows.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "garnet.h"

constexpr double kScreenSizeScaler = 2.5;
constexpr int kScreenWidth = (int)(200 * kScreenSizeScaler);
constexpr int kScreenHeight = (int)(50 * kScreenSizeScaler);

constexpr int kWorldWidth = 20;
constexpr int kWorldHeight = 20;
constexpr int kWorldDepth = 20;

constexpr double kBlockBorderWidth = 0.1;

constexpr double kRadianToDegree = 180.0 / M_PI;
constexpr double kDegreeToRadian = M_PI / 180.0;
constexpr double kCameraFOVDegree = 90.0;
constexpr double kCameraFOV = kCameraFOVDegree * kDegreeToRadian;
constexpr double kHalfOfCameraFOV = kCameraFOV / 2.0;

constexpr double kRayTracingMaximumStep = 2.0;
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
        CreateBlock((int)v.x, (int)v.y, (int)v.z);
    }

    void DeleteBlock(int x, int y, int z) {
        _blocks[z][y][x] = BlockState::kNone;
    }
    void DeleteBlock(const Vector3& v) {
        DeleteBlock((int)v.x, (int)v.y, (int)v.z);
    }

    bool HasBlock(int x, int y, int z) const {
        return _blocks[z][y][x] == BlockState::kExist;
    }
    bool HasBlock(const Vector3& v) const {
        return HasBlock((int)v.x, (int)v.y, (int)v.z);
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
    Vector3 position = { 0, 0, 0 };
    View view = { 0, 0 };
};

class Player {
    World& _world;
public:
    Vector3 position = { 5, 5, 5 };
    View view = { 0, 0 };
    Vector3 velocity = { 0, 0, 0 };
    bool is_physics_enabled = false;
    double move_speed = 5.0;
    double tilt_speed = 1.0;
    double jump_force = 0.35;
    Vector3 forward, right, up;

    Player(World& world) : _world(world), view(0, 0) {}

    void Update(double delta_time) {
        forward = view.ToDirection().Normalize();

        double old_yaw = view.yaw;
        view.yaw -= M_PI_2;
        right = view.ToDirection().Normalize();
        view.yaw = old_yaw;

        double old_pitch = view.pitch;
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
        double hit_step;
        double cumulative_step = 0.0;

        Vector3 position = ray.position;
        bool is_fit_x = position.x == floor(position.x);
        bool is_fit_y = position.y == floor(position.y);
        bool is_fit_z = position.z == floor(position.z);

        if (is_fit_x) position.x += 0.00001;
        if (is_fit_y) position.y += 0.00001;
        if (is_fit_z) position.z += 0.00001;
        while (IsInWorld(position, world)) {
            if (IsHitBlock(position, world)) {
                if (hit != nullptr) {
                    hit->point = hit_position + ray.direction * hit_step;
                    hit->normal = hit_normal;
                }
                return true;
            }

            {
                double current_step = kRayTracingMaximumStep;

                if (ray.direction.x > kRayTracingDetectingFactor) {
                    double step = (ceil(position.x) - position.x) / ray.direction.x;
                    if (current_step > step) {
                        current_step = step;
                        hit_position = position;
                        hit_step = current_step;
                        hit_normal = left;
                    }
                }
                else if (ray.direction.x < -kRayTracingDetectingFactor) {
                    double step = (floor(position.x) - position.x) / ray.direction.x;
                    if (current_step > step) {
                        current_step = step;
                        hit_position = position;
                        hit_step = current_step;
                        hit_normal = right;
                    }
                }
                if (ray.direction.y > kRayTracingDetectingFactor) {
                    double step = (ceil(position.y) - position.y) / ray.direction.y;
                    if (current_step > step) {
                        current_step = step;
                        hit_position = position;
                        hit_step = current_step;
                        hit_normal = down;
                    }
                }
                else if (ray.direction.y < -kRayTracingDetectingFactor) {
                    double step = (floor(position.y) - position.y) / ray.direction.y;
                    if (current_step > step) {
                        current_step = step;
                        hit_position = position;
                        hit_step = current_step;
                        hit_normal = up;
                    }
                }
                if (ray.direction.z > kRayTracingDetectingFactor) {
                    double step = (ceil(position.z) - position.z) / ray.direction.z;
                    if (current_step > step) {
                        current_step = step;
                        hit_position = position;
                        hit_step = current_step;
                        hit_normal = back;
                    }
                }
                else if (ray.direction.z < -kRayTracingDetectingFactor) {
                    double step = (floor(position.z) - position.z) / ray.direction.z;
                    if (current_step > step) {
                        current_step = step;
                        hit_position = position;
                        hit_step = current_step;
                        hit_normal = forward;
                    }
                }

                cumulative_step += current_step;
                if (max_distance < cumulative_step)
                    return false;
                
                position = position + ray.direction * (current_step + kRayTracingForwardingFactor);
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
    void UpdatePhysics(double delta_time);

    void Render();
    void RenderWithRayTracing();

    void Dispose();
};

Screen screen(kScreenWidth, kScreenHeight);
Graphic graphic(screen);
Texture *grass_tex, *moss_tex, *dirt_tex;

class RayTracingLoop : public Loop {
private:
    Play* play;
protected:
    void OnInitialize() override {
        grass_tex = Texture::Load("images/grass-texture.png");
        moss_tex = Texture::Load("images/moss-texture.png");
        dirt_tex = Texture::Load("images/dirt-texture.png");

        play = new Play(kWorldWidth, kWorldHeight, kWorldDepth);
        play->Initialize();
    }

    void OnUpdate(double delta_time) override {
        play->Update(delta_time);
        if (IsKeyPressed(VK_OEM_3))
            Quit();
    }

    void OnRender() override {
        graphic.Clear();
        play->Render();

        Sleep(10);
    }

    void OnDispose() override {
        if (grass_tex != nullptr)
            delete grass_tex;
        if (moss_tex != nullptr)
            delete moss_tex;
        if (dirt_tex != nullptr)
            delete dirt_tex;
        play->Dispose();
        delete play;
    }
};

void Play::Initialize() {
    for (int d = 0; d < _world.kDepth; d++)
        for (int r = 0; r < 5; r++)
            for (int c = 0; c < _world.kWidth; c++)
                _world.CreateBlock(c, r, d);

    _world.CreateBlock(5, 7, 8);
}

void Play::Update(double delta_time) {
    UpdateInput();
    UpdateBlockSelection();
    UpdatePlayerMovement(delta_time);
    UpdatePhysics(delta_time);
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
        _player.view.pitch = fmin(_player.view.pitch + _player.tilt_speed * delta_time, 70.0 * kDegreeToRadian);
    }
    if (IsKeyPressed('K')) {
        _player.view.pitch = fmax(_player.view.pitch - _player.tilt_speed * delta_time, -60.0 * kDegreeToRadian);
    }

    if (IsKeyPressed(VK_SPACE)) {
        if (!_player.is_physics_enabled) {
            _player.is_physics_enabled = true;
            _player.velocity = up * _player.jump_force;
        }
    }
}

void Play::UpdatePhysics(double delta_time) {
    if (_player.is_physics_enabled == false) {
        Ray ray = { _player.position, down };
        Hit hit;
        bool is_hit = _ray_tracing.CastRay(ray, &hit, 0.001, _world);
        if (!is_hit) {
            _player.is_physics_enabled = true;
        }
    }

    if (_player.is_physics_enabled) {
        _player.velocity.y += -0.5 * delta_time;

        if (_player.velocity.y != 0) {
            Ray ray = { _player.velocity.y > 0 ? _player.position + up * 0.6 : _player.position, _player.velocity.y > 0 ? up : down };
            Hit hit;
            bool is_hit = _ray_tracing.CastRay(ray, &hit, fabs(_player.velocity.y), _world);
            if (is_hit) {
                if (_player.velocity.y < 0) {
                    _player.position = hit.point + up * 0.00001;
                    _player.is_physics_enabled = false;
                }
                _player.velocity.y = 0;
            }
            else
                _player.position.y += _player.velocity.y;
        }
        else {
            _player.position.y += _player.velocity.y;
        }
    }
}

void Play::UpdateCamera() {
    _camera.position = _player.position + up * 0.5;
    _camera.view = _player.view;
}

void Play::Render() {
    RenderWithRayTracing();
}

void Play::RenderWithRayTracing() {
    View left_view(_camera.view.yaw + M_PI_2, 0);
    Vector3 left = left_view.ToDirection();
    View up_view(_camera.view.yaw, _camera.view.pitch + M_PI_2);
    Vector3 up = up_view.ToDirection();
    
    Vector3 screen_center = _camera.view.ToDirection();
    
    // horizontal
    Vector3 screen_left = screen_center + left;
    Vector3 screen_right = screen_center - left;
    Vector3 left_to_right = screen_right - screen_left;
    
    // vertical
    Vector3 screen_top = screen_center + up;
    Vector3 screen_bottom = screen_center - up;
    Vector3 top_to_bottom = (screen_bottom - screen_top) * screen.aspect_ratio * 2;

    Vector3 screen_left_top = screen_left - top_to_bottom / 2;

    // ray casting
    Vector3 horizontal_step = left_to_right * 1.0 / screen.width;
    Vector3 vertical_step = top_to_bottom * 1.0 / screen.height / 2;

    Hit hit;
    Ray ray = { _camera.position };
    Vector3 direction = screen_left_top;
    Vector3 world_size = { (double)_world.kWidth, (double)_world.kWidth, (double)_world.kWidth };
    double max_distance = world_size.Magnitude();
    for (uint32_t r = 0; r < screen.height * 2; r++) {
        Vector3 t = direction;

        for (uint32_t c = 0; c < screen.width; c++) {
            ray.direction = direction;

            bool is_hit = _ray_tracing.CastRay(ray, &hit, max_distance, _world);
            if (is_hit) {
                bool is_border_of_block = _ray_tracing.IsHitBorderOfBlock(hit.point);
                Color color;
                Vector3 uv = hit.point - hit.point.Floor();
                if (abs(hit.normal.z + 1) < 0.001)
                    grass_tex->GetColor(uv.x, 1 - uv.y, color);
                if (abs(hit.normal.z - 1) < 0.001)
                    grass_tex->GetColor(1 - uv.x, 1 - uv.y, color);
                if (abs(hit.normal.x + 1) < 0.001)
                    grass_tex->GetColor(1 - uv.z, 1 - uv.y, color);
                if (abs(hit.normal.x - 1) < 0.001)
                    grass_tex->GetColor(uv.z, 1 - uv.y, color);
                if (abs(hit.normal.y + 1) < 0.001)
                    dirt_tex->GetColor(uv.x, 1 - uv.z, color);
                if (abs(hit.normal.y - 1) < 0.001)
                    moss_tex->GetColor(uv.x, uv.z, color);
                    
                if (_is_block_selected) {
                    bool is_selected_block = (hit.point - hit.normal * 0.5).Floor() == _selected_block_position.Floor();
                    
                    if (is_selected_block) {
                        bool is_selected_surface = hit.normal.Round() == _selected_block_normal.Round();
                        if (is_selected_surface) {
                            color.r = (uint8_t)fmin(color.r * 1.5, 255);
                            color.g = (uint8_t)fmin(color.g * 1.5, 255);
                            color.b = (uint8_t)fmin(color.b * 1.5, 255);
                        }
                        else {
                            color.r = (uint8_t)fmin(color.r * 1.2, 255);
                            color.g = (uint8_t)fmin(color.g * 1.2, 255);
                            color.b = (uint8_t)fmin(color.b * 1.2, 255);
                        }
                    }
                }

                graphic.DrawPoint(c, r, color);
            }

            direction = direction + horizontal_step;
        }

        direction = t + vertical_step;
    }
    graphic.DrawPoint(graphic.width / 2, graphic.height / 2, {200, 0, 0});
    graphic.Flush();
}

void Play::Dispose() { }

int main() {
    RayTracingLoop loop;
    loop.Start(&screen);

    return 0;
}
