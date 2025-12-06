
#include <vector>
#include <iostream>
#include <cmath>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"
#include "phong_shader.h"
#include "camera.h"

Model* model = NULL;
const int width = 800;
const int height = 800;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


Vec3f light_dir(1, 1, 1);

// Функция для вычисления ограничивающего объема модели
void compute_model_bounds(Model* model, Vec3f& min, Vec3f& max) {
    if (model->nverts() == 0) return;

    min = model->vert(0);
    max = model->vert(0);

    for (int i = 1; i < model->nverts(); i++) {
        Vec3f v = model->vert(i);
        for (int j = 0; j < 3; j++) {
            if (v[j] < min[j]) min[j] = v[j];
            if (v[j] > max[j]) max[j] = v[j];
        }
    }
}

int main(int argc, char** argv) {
    // Загрузка модели
    if (argc == 2) {
        model = new Model(argv[1]);
    }
    else {
        model = new Model("obj/african_head.obj");
    }

    // Вычисляем размеры модели
    Vec3f model_min, model_max;
    compute_model_bounds(model, model_min, model_max);

    Vec3f model_center = (model_min + model_max) * 0.5f;
    Vec3f model_size = model_max - model_min;
    float model_radius = model_size.norm() * 0.5f;

    std::cout << "Model center: " << model_center << std::endl;
    std::cout << "Model size: " << model_size << std::endl;
    std::cout << "Model radius: " << model_radius << std::endl;

    // Инициализация viewport
    viewport(0, 0, width, height);
    light_dir.normalize();

    TGAImage image(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    // Углы: 0° (спереди), 90° (слева), 180° (сзади), 270° (справа)
    float angles[4] = { 0.0f, 90.0f, 180.0f, 270.0f };

    // Подбираем подходящий радиус для орбиты (в 2-3 раза больше размера модели)
    float orbit_radius = 1.5f;  // Фиксированный радиус
    float camera_height = 0.1f;  // На уровне нуля
    Vec3f center(0, 0, 0);  // Центр в начале координат
    float model_scale = 2.0f;

    // ИЗМЕНЕНИЕ: Рендерим только первый вид (0°)
    int i = 0; // Только первый вид

    image.clear();
    zbuffer.clear();

    // Позиция камеры для орбитального облета
    float angle_rad = angles[i] * M_PI / 180.0f;
    float x = orbit_radius * sinf(angle_rad);
    float z = orbit_radius * cosf(angle_rad);

    Vec3f eye(x + model_center.x, camera_height, z + model_center.z);

    Vec3f up(0, 1, 0);

    std::cout << "Camera " << i << ":" << std::endl;
    std::cout << "  Eye: " << eye << std::endl;
    std::cout << "  Center: " << center << std::endl;
    std::cout << "  Distance: " << (eye - center).norm() << std::endl;

    // Создаем камеру
    Camera camera(eye, center, up);

    // Получаем матрицу вида
    Matrix view_matrix = camera.get_view_matrix();

    // Сохраняем для использования в шейдере
    ModelView = view_matrix;

    // Настройка проекции - используем перспективную проекцию
    // Коэффициент для перспективной проекции (focal length)
    float focal_length = 1.0f;
    float distance = (eye - center).norm();
    projection(-focal_length / distance);  // Перспективная проекция

    // Шейдер
    PhongShader shader;
    shader.uniform_M = Projection * ModelView;
    shader.uniform_MIT = (Projection * ModelView).invert_transpose();
    shader.light_dir = light_dir;
    shader.light_color = Vec3f(1.0f, 1.0f, 1.0f);
    shader.ambient_color = Vec3f(0.1f, 0.1f, 0.1f);
    shader.specular_exponent = 32.0f;
    shader.specular_intensity = 0.5f;
    shader.view_dir = (center - eye).normalize();
    shader.camera_pos = eye;

    // Текстуры
    shader.diffusemap = &model->diffusemap_;
    shader.normalmap = &model->normalmap_;
    shader.specularmap = &model->specularmap_;

    // Рендеринг
    std::cout << "Rendering view " << i << "..." << std::endl;
    int rendered_faces = 0;
    for (int j = 0; j < model->nfaces(); j++) {
        Vec4f screen_coords[3];
        // ПРОВЕРКА: ВЕРШИНА ПОЗАДИ КАМЕРЫ
        bool valid_triangle = true;

        for (int k = 0; k < 3; k++) {
            screen_coords[k] = shader.vertex(j, k);
            // Проверяем, находится ли вершина в поле зрения
            if (screen_coords[k][3] <= 0) {  // w <= 0 означает, что вершина позади камеры
                valid_triangle = false;
                break;
            }
        }

        if (valid_triangle) {
            triangle(screen_coords, shader, image, zbuffer);
            rendered_faces++;
        }
    }

    std::cout << "Rendered " << rendered_faces << " faces out of " << model->nfaces() << std::endl;

    // Сохранение
    image.flip_vertically();
    zbuffer.flip_vertically();

    std::string filename = "output_0.tga";
    image.write_tga_file(filename.c_str());

    std::cout << "Saved to: " << filename << std::endl;
    std::cout << std::endl;

    delete model;
    return 0;
}