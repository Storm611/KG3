#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "geometry.h"
#include <cmath>
class Camera {
public:
    Vec3f eye;       // Позиция камеры
    Vec3f center;    // Точка, на которую смотрим
    Vec3f up;        // Вектор "вверх"

    // Конструктор
    Camera(const Vec3f& eye = Vec3f(0, 0, 3),
        const Vec3f& center = Vec3f(0, 0, 0),
        const Vec3f& up = Vec3f(0, 1, 0));

    // Орбитальное движение вокруг центра
    void orbit(float angle_y, float angle_x, float radius = -1.0f);

    // Простая орбита только по Y
    void orbit_y(float angle);

    // Установка позиции
    void set_position(const Vec3f& new_eye, const Vec3f& new_center, const Vec3f& new_up);

    // Получение матрицы вида
    Matrix get_view_matrix();
};

#endif // __CAMERA_H__