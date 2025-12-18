#include <vector>
#include <iostream>
#include <cmath>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"
#include "phong_shader.h"
#include "camera.h"

Model* model = nullptr;
const int width = 800;
const int height = 800;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Vec3f light_dir(1, 1, 1);

// ======================
// Bounding box
// ======================
void compute_model_bounds(Model* model, Vec3f& min, Vec3f& max) {
    min = max = model->vert(0);
    for (int i = 1; i < model->nverts(); i++) {
        Vec3f v = model->vert(i);
        for (int j = 0; j < 3; j++) {
            min[j] = std::min(min[j], v[j]);
            max[j] = std::max(max[j], v[j]);
        }
    }
}

class DebugShader : public IShader {
public:
    mat<3, 3, float> varying_tri; // координаты вершин в пространстве камеры
    
    Vec4f vertex(int iface, int nthvert) {
        Vec3f v = model->vert(iface, nthvert);
        Vec4f gl_Vertex = Projection * ModelView * embed<4>(v, 1.f);
        varying_tri[nthvert] = proj<3>(gl_Vertex / gl_Vertex[3]);
        return gl_Vertex;
    }
    
    bool fragment(Vec3f bar, TGAColor& color) {
        // Простой красный цвет для всего
        color = TGAColor(255, 0, 0, 255);
        return false;
    }
};


int main(int argc, char** argv) {

    // ======================
    // Load model
    // ======================
    model = (argc == 2) ? new Model(argv[1])
        : new Model("obj/sponza.obj");

    // ======================
    // Model bounds
    // ======================
    Vec3f model_min, model_max;
    compute_model_bounds(model, model_min, model_max);

    Vec3f model_center = (model_min + model_max) * 0.5f;
    Vec3f model_size = model_max - model_min;
    float model_radius = model_size.norm() * 0.5f;

    std::cout << "Model center: " << model_center << std::endl;
    std::cout << "Model radius: " << model_radius << std::endl;

    // ======================
    // Viewport
    // ======================
    viewport(0, 0, width, height);
    light_dir.normalize();

    TGAImage image(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    image.clear();
    zbuffer.clear();

    // ======================
    // Camera setup (AUTO FIT)
    // ======================
    float fov = 60.0f * M_PI / 180.0f;
    float camera_distance = model_radius / std::tan(fov * 0.5f);
    camera_distance *= 1.2f; // запас

    float angle = 0.0f; // фронтальный вид

    Vec3f center = model_center;
    Vec3f eye(
        center.x + camera_distance * std::sin(angle),
        center.y + model_radius * 0.2f,
        center.z + camera_distance * std::cos(angle)
    );

    Vec3f up(0, 1, 0);

    // ========= ОТЛАДКА =========
    std::cout << "=== Camera Debug ===" << std::endl;
    std::cout << "Model center: " << center << std::endl;
    std::cout << "Model radius: " << model_radius << std::endl;
    std::cout << "Camera distance: " << camera_distance << std::endl;
    std::cout << "Eye position: " << eye << std::endl;
    std::cout << "Center: " << center << std::endl;
    std::cout << "Up: " << up << std::endl;

    // Проверьте, что камера не внутри модели
    Vec3f dir = (center - eye).normalize();
    std::cout << "Direction to center: " << dir << std::endl;
    std::cout << "Distance eye-center: " << (center - eye).norm() << std::endl;

    std::cout << "Camera eye: " << eye << std::endl;
    std::cout << "Camera center: " << center << std::endl;
    std::cout << "Camera distance: " << (eye - center).norm() << std::endl;

    Camera camera(eye, center, up);
    ModelView = camera.get_view_matrix();

    // ======================
    // Projection
    // ======================
    projection(-1.0f / camera_distance);

    // ======================
    // Shader
    // ======================
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

    shader.diffusemap = &model->diffusemap_;
    shader.normalmap = &model->normalmap_;
    shader.specularmap = &model->specularmap_;

    // ======================
    // Render
    // ======================
    int rendered_faces = 0;

    for (int i = 0; i < model->nfaces(); i++) {
        Vec4f clip_coords[3];

        for (int j = 0; j < 3; j++) {
            clip_coords[j] = shader.vertex(i, j);
        }

        triangle(clip_coords, shader, image, zbuffer);
        rendered_faces++;
    }

    std::cout << "Rendered faces: "
        << rendered_faces << " / "
        << model->nfaces() << std::endl;

    // ======================
    // Save
    // ======================
    image.flip_vertically();
    zbuffer.flip_vertically();
    image.write_tga_file("output.tga");

    delete model;
    return 0;
}
