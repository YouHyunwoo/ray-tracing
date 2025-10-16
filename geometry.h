#ifndef _GEOMETRY_H_
#define _GEOMETRY_H_

struct Vector3 {
    double x, y, z;
    Vector3 operator+(const Vector3& other) const;
    Vector3 operator-() const;
    Vector3 operator-(const Vector3& other) const;
    template<typename T>
    Vector3 operator*(T s) const {
        return { x * static_cast<double>(s), y * static_cast<double>(s), z * static_cast<double>(s) };
    }
    template<typename T>
    Vector3 operator/(T s) const {
        return { x / static_cast<double>(s), y / static_cast<double>(s), z / static_cast<double>(s) };
    }
    double Dot(const Vector3& other) const;
    double Magnitude() const;
    Vector3 Normalize() const;
    Vector3 Floor() const;
    Vector3 Round() const;
    bool operator==(const Vector3 other) const;
};

// 전역 템플릿 연산자 오버로딩 (스칼라 * Vector3)
template<typename T>
inline Vector3 operator*(T s, const Vector3& v) {
    return { v.x * static_cast<double>(s), v.y * static_cast<double>(s), v.z * static_cast<double>(s) };
}

template<typename T>
inline Vector3 operator/(T s, const Vector3& v) {
    return { v.x / static_cast<double>(s), v.y / static_cast<double>(s), v.z / static_cast<double>(s) };
}

extern Vector3 forward;
extern Vector3 back;
extern Vector3 right;
extern Vector3 left;
extern Vector3 up;
extern Vector3 down;

#endif