#ifndef __PHONG_SHADER_H__
#define __PHONG_SHADER_H__

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

class PhongShader : public IShader {
public:
    // Матрицы
    Matrix uniform_M;     // Проекция * ModelView
    Matrix uniform_MIT;   // обратная транспонированная
    
    // Освещение
    Vec3f light_dir;
    Vec3f light_color;
    Vec3f ambient_color;
    
    // Камера
    Vec3f view_dir;      // Направление взгляда
    Vec3f camera_pos;    // Позиция камеры (для specular)
    
    // Блик
    float specular_exponent;
    float specular_intensity;
    
    // Текстуры
    TGAImage* diffusemap;
    TGAImage* normalmap;
    TGAImage* specularmap;
    
    // Интерполируемые значения
    mat<3, 3, float> varying_tri;   // координаты вершин в пространстве камеры
    mat<3, 3, float> varying_nrm;   // нормали вершин
    mat<3, 2, float> varying_uv;    // UV координаты
    
    virtual Vec4f vertex(int iface, int nthvert);
    virtual bool fragment(Vec3f bar, TGAColor& color);
};

#endif //__PHONG_SHADER_H__