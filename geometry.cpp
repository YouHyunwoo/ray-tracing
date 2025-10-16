#include "geometry.h"

#include <math.h>

Vector3 forward = { 0, 0, +1 };
Vector3 back = { 0, 0, -1 };
Vector3 right = { +1, 0, 0 };
Vector3 left = { -1, 0, 0 };
Vector3 up = { 0, +1, 0 };
Vector3 down = { 0, -1, 0 };

Vector3 Vector3::operator+(const Vector3& other) const {
    return { x + other.x, y + other.y, z + other.z };
}

Vector3 Vector3::operator-() const {
    return { -x, -y, -z };
}

Vector3 Vector3::operator-(const Vector3& other) const {
    return { x - other.x, y - other.y, z - other.z };
}

double Vector3::Dot(const Vector3& other) const {
    return x * other.x + y * other.y + z * other.z;
}

double Vector3::Magnitude() const {
    return sqrt(Dot(*this));
}

Vector3 Vector3::Normalize() const {
    double length = Magnitude();
    return { x / length, y / length, z / length };
}

Vector3 Vector3::Floor() const {
    return { floor(x), floor(y), floor(z) };
}

Vector3 Vector3::Round() const {
    return { round(x), round(y), round(z) };
}

bool Vector3::operator==(const Vector3 other) const {
    return (
        fabs(x - other.x) < 0.0001 &&
        fabs(y - other.y) < 0.0001 &&
        fabs(z - other.z) < 0.0001
    );
}