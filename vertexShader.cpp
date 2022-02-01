#include "vertexShader.h"

VertexShader::VertexShader(const float &a, const float &s, const Eigen::Vector3f &m, const Eigen::Vector3f &e, const Frustum &f) :
    angleY(a), scale(s), move(m), fru(f)
{
    float radian = fru.fov * MY_PI / 180.f;
    float t = tan(radian / 2) * abs(fru.zNear);
    float b = -t;
    float r = fru.aspect * t;
    float l = -r;

    // 计算 projection 矩阵。
    Eigen::Matrix4f Mpo;
    Mpo << fru.zNear, 0, 0, 0,
        0, fru.zNear, 0, 0,
        0, 0, fru.zNear + fru.zFar, -fru.zNear * fru.zFar,
        0, 0, 1, 0;
    Eigen::Matrix4f Mo;
    Mo << 2 / (r - l), 0, 0, (r + l) / -2,
        0, 2 / (t - b), 0, (t + b) / -2,
        0, 0, 2 / (fru.zNear - fru.zFar), (fru.zNear + fru.zFar) / -2,
        0, 0, 0, 1;
    projection = Mo * Mpo;

    // 计算 view 矩阵。
    view << 1, 0, 0, -e[0],
        0, 1, 0, -e[1],
        0, 0, 1, -e[2],
        0, 0, 0, 1;

    // 计算 model 矩阵中移动的部分。
    Mmove << 1, 0, 0, move[0],
        0, 1, 0, move[1],
        0, 0, 1, move[2],
        0, 0, 0, 1;
}

void VertexShader::Update(const float &a, const float &s) {
    angleY = a;
    scale = s;
}

Eigen::Matrix4f VertexShader::GetModelMatrix() {
    Eigen::Matrix4f Mrotation;
    float radian = angleY * MY_PI / 180.f;
    Mrotation << cos(radian), 0, sin(radian), 0,
        0, 1, 0, 0,
        -sin(radian), 0, cos(radian), 0,
        0, 0, 0, 1;

    Eigen::Matrix4f Mscale;
    Mscale << scale, 0, 0, 0,
        0, scale, 0, 0,
        0, 0, scale, 0,
        0, 0, 0, 1;

    return Mmove * Mscale * Mrotation;
}

void VertexShader::Transform(Vertex *vertex) {
    Eigen::Matrix4f model = GetModelMatrix();

    // 用于保留 viewSpace 坐标系。
    Eigen::Matrix4f modelView = view * model;
    // mvp 矩阵。
    Eigen::Matrix4f mvp = projection * modelView;
    // mv 矩阵的逆的转置，用于处理顶点法向量。
    Eigen::Matrix4f modelViewInvTran = modelView.inverse().transpose();

    for (int i = 0; i < 3; ++i) {
        // 给片元着色器提供用于计算光照的线性空间坐标。
        vertex[i].viewPos = (modelView * vertex[i].pos).head<3>();

        Eigen::Vector4f p = mvp * vertex[i].pos;

        // 归一化至 NDC 坐标系。
        p[3] = 1.f / p[3];
        p[0] *= p[3];
        p[1] *= p[3];
        p[2] *= p[3];

        // 视口变换。
        p[0] = float(0.5f * WIDTH * (p[0] + 1.f));
        p[1] = float(0.5f * HEIGHT * (p[1] + 1.f));
        //v[2] *= (fru.zFar - fru.zNear) / 2.f + (fru.zFar + fru.zNear) / 2.f;

        vertex[i].pos = p;

        // 法向量变换。
        vertex[i].normal = modelViewInvTran * vertex[i].normal;
    }
}
