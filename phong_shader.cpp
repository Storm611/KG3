#include "phong_shader.h"
#include <cmath>

// Внешняя переменная модели
extern Model* model;

Vec4f PhongShader::vertex(int iface, int nthvert) {
    Vec2f uv = model->uv(iface, nthvert);
    varying_uv[nthvert] = uv;

    Vec3f n = model->normal(iface, nthvert);
    varying_nrm[nthvert] = proj<3>(uniform_MIT * embed<4>(n, 0.f));

    Vec3f v = model->vert(iface, nthvert);
    Vec4f gl_Vertex = embed<4>(v, 1.f);

    Vec4f clip = Projection * ModelView * gl_Vertex;
    Vec3f ndc = proj<3>(clip / clip[3]);
    varying_tri[nthvert] = ndc;

    return Viewport * clip;
}

bool PhongShader::fragment(Vec3f bar, TGAColor& color) {
    // Интерполируем UV координаты
    Vec2f uv_interpolated(0, 0);
    for (int i = 0; i < 3; i++) {
        uv_interpolated.x += varying_uv[i][0] * bar[i];
        uv_interpolated.y += varying_uv[i][1] * bar[i];
    }

    // Интерполируем нормаль
    Vec3f n_interpolated(0, 0, 0);
    for (int i = 0; i < 3; i++) {
        n_interpolated.x += varying_nrm[i][0] * bar[i];
        n_interpolated.y += varying_nrm[i][1] * bar[i];
        n_interpolated.z += varying_nrm[i][2] * bar[i];
    }
    n_interpolated = n_interpolated.normalize();

    // ====================
    // ОТКЛЮЧАЕМ НОРМАЛЬНУЮ КАРТУ
    // ====================
    Vec3f n = n_interpolated; // Используем интерполированные нормали, а не из текстуры

    // Интерполируем позицию вершины
    Vec3f p(0, 0, 0);
    for (int i = 0; i < 3; i++) {
        p.x += varying_tri[i][0] * bar[i];
        p.y += varying_tri[i][1] * bar[i];
        p.z += varying_tri[i][2] * bar[i];
    }

    // Вычисляем освещение
    Vec3f light_dir_normalized = light_dir.normalize();
    Vec3f view_dir_normalized = view_dir.normalize();
    Vec3f to_camera = (camera_pos - p).normalize();

    // Диффузная компонента
    float NdotL = std::max(0.0f, n * light_dir_normalized);
    float diff = NdotL;

    // Бликовая компонента
    float spec = 0.0f;
    if (diff > 0) {
        Vec3f half_vector = (light_dir_normalized + to_camera).normalize();
        float NdotH = std::max(0.0f, n * half_vector);
        spec = powf(NdotH, specular_exponent);
    }

    // ====================
    // ОТКЛЮЧАЕМ СПЕКУЛЯРНУЮ КАРТУ
    // ====================
    // Просто используем uniform значение
    spec *= specular_intensity;

    // Окружающая компонента
    Vec3f ambient = ambient_color;

    // ====================
    // ОТКЛЮЧАЕМ ДИФФУЗНУЮ КАРТУ
    // Используем постоянный цвет вместо текстуры
    // ====================
    Vec3f diffuse_color(0.8f, 0.8f, 0.8f); // Серый цвет

    // Вместо этого кода:
    // if (diffusemap && diffusemap->get_width() > 0 && diffusemap->get_height() > 0) {
    //     // ... код загрузки из текстуры ...
    // }

    // Просто используем diffuse_color как есть

    // Комбинируем освещение
    Vec3f result_color;
    for (int i = 0; i < 3; i++) {
        result_color[i] = diffuse_color[i] * ambient[i] +
            diffuse_color[i] * light_color[i] * diff +
            light_color[i] * spec;

        result_color[i] = std::min(1.0f, std::max(0.0f, result_color[i]));
    }

    // Преобразуем в TGAColor
    color = TGAColor(
        (unsigned char)(result_color[0] * 255),
        (unsigned char)(result_color[1] * 255),
        (unsigned char)(result_color[2] * 255),
        255
    );

    return false;
}