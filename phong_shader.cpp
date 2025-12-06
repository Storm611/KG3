#include "phong_shader.h"
#include <cmath>

// Внешняя переменная модели
extern Model* model;

Vec4f PhongShader::vertex(int iface, int nthvert) {
    // Получаем UV координаты (возвращает Vec2f)
    Vec2f uv = model->uv(iface, nthvert);

    // ФИКС: Правильно устанавливаем UV в матрицу
    // mat<3, 2, float> - 3 колонки по 2 значения (u,v)
    // Устанавливаем для каждой вершины (nthvert) ее UV
    for (int i = 0; i < 2; i++) {
        varying_uv[nthvert][i] = uv[i];  // ith компонента UV для nthvert вершины
    }

    // Получаем нормаль вершины
    Vec3f n = model->normal(iface, nthvert);
    Vec3f transformed_norm = proj<3>(uniform_MIT * embed<4>(n, 0.f));

    // Устанавливаем нормаль в матрицу
    for (int i = 0; i < 3; i++) {
        varying_nrm[nthvert][i] = transformed_norm[i];
    }

    // Получаем позицию вершины
    Vec3f vert_pos = model->vert(iface, nthvert);
    Vec4f gl_Vertex = uniform_M * embed<4>(vert_pos, 1.f);
    Vec3f proj_vertex = proj<3>(gl_Vertex / gl_Vertex[3]);

    // Устанавливаем позицию в матрицу
    for (int i = 0; i < 3; i++) {
        varying_tri[nthvert][i] = proj_vertex[i];
    }

    // Преобразуем в экранные координаты
    return Viewport * Projection * ModelView * gl_Vertex;
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

    // Получаем нормаль из карты нормалей
    Vec3f n = n_interpolated;

    if (normalmap && normalmap->get_width() > 0 && normalmap->get_height() > 0) {
        int tex_width = normalmap->get_width();
        int tex_height = normalmap->get_height();

        // Нормализуем UV координаты к [0, 1]
        float u = uv_interpolated.x - floor(uv_interpolated.x);
        float v = uv_interpolated.y - floor(uv_interpolated.y);

        int tex_x = (int)(u * tex_width);
        int tex_y = (int)(v * tex_height);

        tex_x = std::max(0, std::min(tex_width - 1, tex_x));
        tex_y = std::max(0, std::min(tex_height - 1, tex_y));

        TGAColor normal_color = normalmap->get(tex_x, tex_y);

        // Извлекаем нормаль из текстуры
        n.x = (float)normal_color[2] / 255.f * 2.f - 1.f;  // Red -> X
        n.y = (float)normal_color[1] / 255.f * 2.f - 1.f;  // Green -> Y
        n.z = (float)normal_color[0] / 255.f * 2.f - 1.f;  // Blue -> Z

        n = n.normalize();
    }

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

    // Интенсивность блика из карты
    if (specularmap && specularmap->get_width() > 0 && specularmap->get_height() > 0) {
        int tex_width = specularmap->get_width();
        int tex_height = specularmap->get_height();

        float u = uv_interpolated.x - floor(uv_interpolated.x);
        float v = uv_interpolated.y - floor(uv_interpolated.y);

        int tex_x = (int)(u * tex_width);
        int tex_y = (int)(v * tex_height);
        tex_x = std::max(0, std::min(tex_width - 1, tex_x));
        tex_y = std::max(0, std::min(tex_height - 1, tex_y));

        float spec_intensity = specularmap->get(tex_x, tex_y)[0] / 255.f;
        spec *= spec_intensity * specular_intensity;
    }

    // Окружающая компонента
    Vec3f ambient = ambient_color;

    // Цвет из диффузной текстуры
    Vec3f diffuse_color(0.8f, 0.8f, 0.8f);

    if (diffusemap && diffusemap->get_width() > 0 && diffusemap->get_height() > 0) {
        int tex_width = diffusemap->get_width();
        int tex_height = diffusemap->get_height();

        float u = uv_interpolated.x - floor(uv_interpolated.x);
        float v = uv_interpolated.y - floor(uv_interpolated.y);

        int tex_x = (int)(u * tex_width);
        int tex_y = (int)(v * tex_height);
        tex_x = std::max(0, std::min(tex_width - 1, tex_x));
        tex_y = std::max(0, std::min(tex_height - 1, tex_y));

        TGAColor tex_color = diffusemap->get(tex_x, tex_y);

        diffuse_color = Vec3f(
            tex_color[2] / 255.f,  // Blue -> Red
            tex_color[1] / 255.f,  // Green -> Green
            tex_color[0] / 255.f   // Red -> Blue
        );
    }

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