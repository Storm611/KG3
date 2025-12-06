#include "camera.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Camera::Camera(const Vec3f& eye, const Vec3f& center, const Vec3f& up)
    : eye(eye), center(center), up(up) {
}

void Camera::orbit(float angle_y, float angle_x, float radius) {
    // Если радиус не указан, вычисляем текущий
    if (radius <= 0) {
        Vec3f dir = eye - center;
        radius = dir.norm();
        if (radius < 1.0f) radius = 3.0f; // минимальный радиус
    }

    // Преобразуем углы в радианы
    float yaw_rad = angle_y * M_PI / 180.0f;
    float pitch_rad = angle_x * M_PI / 180.0f;

    // Ограничиваем pitch, чтобы камера не переворачивалась
    if (pitch_rad > M_PI / 2 - 0.1f) pitch_rad = M_PI / 2 - 0.1f;
    if (pitch_rad < -M_PI / 2 + 0.1f) pitch_rad = -M_PI / 2 + 0.1f;

    // Вычисляем новую позицию на сфере
    float x = radius * cosf(pitch_rad) * sinf(yaw_rad);
    float y = radius * sinf(pitch_rad);
    float z = radius * cosf(pitch_rad) * cosf(yaw_rad);

    // Устанавливаем новую позицию камеры
    eye = center + Vec3f(x, y, z);

    // Вектор "вверх" всегда направлен вверх по миру
    up = Vec3f(0, 1, 0);
}

void Camera::orbit_y(float angle) {
    // Простая орбита только вокруг оси Y
    Vec3f dir = eye - center;
    float radius = dir.norm();
    if (radius < 1.0f) radius = 3.0f;

    float angle_rad = angle * M_PI / 180.0f;

    // Текущие координаты
    float current_x = dir.x;
    float current_z = dir.z;

    // Вращаем вокруг оси Y
    float cosA = cosf(angle_rad);
    float sinA = sinf(angle_rad);

    float new_x = current_x * cosA - current_z * sinA;
    float new_z = current_x * sinA + current_z * cosA;

    // Устанавливаем новую позицию
    eye = center + Vec3f(new_x, dir.y, new_z);
    up = Vec3f(0, 1, 0);
}

void Camera::set_position(const Vec3f& new_eye, const Vec3f& new_center, const Vec3f& new_up) {
    eye = new_eye;
    center = new_center;
    up = new_up;
}

Matrix Camera::get_view_matrix() {
    Vec3f f = (center - eye).normalize();
    Vec3f s = cross(f, up).normalize();
    Vec3f u = cross(s, f);

    Matrix Minv = Matrix::identity();
    Matrix Tr = Matrix::identity();

    for (int i = 0; i < 3; i++) {
        Minv[0][i] = s[i];
        Minv[1][i] = u[i];
        Minv[2][i] = -f[i];
        Tr[i][3] = -eye[i];
    }

    return Minv * Tr;
}