#include "Matrix3x3.h"
#include "Vector2D.h"
#include <algorithm>
#include <cmath>


// ----- ctors -----
Matrix3x3::Matrix3x3() {
    std::fill(std::begin(m), std::end(m), 0.0f);
}

Matrix3x3::Matrix3x3(float const* pArr) {
    for (int i = 0; i < 9; ++i) m[i] = pArr[i];
}

Matrix3x3::Matrix3x3(float _00, float _01, float _02,
    float _10, float _11, float _12,
    float _20, float _21, float _22) {
    m[0] = _00; m[1] = _01; m[2] = _02;
    m[3] = _10; m[4] = _11; m[5] = _12;
    m[6] = _20; m[7] = _21; m[8] = _22;
}

// ----- element access -----
float& Matrix3x3::at(std::size_t r, std::size_t c) { return m[idx(r, c)]; }
float  Matrix3x3::at(std::size_t r, std::size_t c) const { return m[idx(r, c)]; }

// ----- set builders -----
Matrix3x3& Matrix3x3::setIdentity() {
    m[0] = 1; m[1] = 0; m[2] = 0;
    m[3] = 0; m[4] = 1; m[5] = 0;
    m[6] = 0; m[7] = 0; m[8] = 1;
    return *this;
}

Matrix3x3& Matrix3x3::setTranslate(float x, float y) {
    setIdentity();
    m[idx(0, 2)] = x;
    m[idx(1, 2)] = y;
    return *this;
}

Matrix3x3& Matrix3x3::setScale(float sx, float sy) {
    setIdentity();
    m[idx(0, 0)] = sx;
    m[idx(1, 1)] = sy;
    return *this;
}

Matrix3x3& Matrix3x3::setRotRad(float a) {
    setIdentity();
    float c = std::cos(a), s = std::sin(a);
    m[idx(0, 0)] = c; m[idx(0, 1)] = -s;
    m[idx(1, 0)] = s; m[idx(1, 1)] = c;
    return *this;
}

Matrix3x3& Matrix3x3::setRotDeg(float deg) {
    return setRotRad(deg * 3.14159265358979323846f / 180.0f);
}

// ----- algebra -----
Matrix3x3& Matrix3x3::operator*=(Matrix3x3 const& rhs) {
    Matrix3x3 out;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            out.m[idx(r, c)] =
                m[idx(r, 0)] * rhs.m[idx(0, c)] +
                m[idx(r, 1)] * rhs.m[idx(1, c)] +
                m[idx(r, 2)] * rhs.m[idx(2, c)];
        }
    }
    *this = out;
    return *this;
}

Matrix3x3& Matrix3x3::transpose() {
    std::swap(m[1], m[3]);
    std::swap(m[2], m[6]);
    std::swap(m[5], m[7]);
    return *this;
}

float Matrix3x3::determinant() const {
    return
        m[idx(0, 0)] * (m[idx(1, 1)] * m[idx(2, 2)] - m[idx(1, 2)] * m[idx(2, 1)]) -
        m[idx(0, 1)] * (m[idx(1, 0)] * m[idx(2, 2)] - m[idx(1, 2)] * m[idx(2, 0)]) +
        m[idx(0, 2)] * (m[idx(1, 0)] * m[idx(2, 1)] - m[idx(1, 1)] * m[idx(2, 0)]);
}

bool Matrix3x3::invert(float* outDet) {
    float a00 = m[0], a01 = m[1], a02 = m[2];
    float a10 = m[3], a11 = m[4], a12 = m[5];
    float a20 = m[6], a21 = m[7], a22 = m[8];

    float c00 = (a11 * a22 - a12 * a21);
    float c01 = -(a10 * a22 - a12 * a20);
    float c02 = (a10 * a21 - a11 * a20);
    float c10 = -(a01 * a22 - a02 * a21);
    float c11 = (a00 * a22 - a02 * a20);
    float c12 = -(a00 * a21 - a01 * a20);
    float c20 = (a01 * a12 - a02 * a11);
    float c21 = -(a00 * a12 - a02 * a10);
    float c22 = (a00 * a11 - a01 * a10);

    float det = a00 * c00 + a01 * c01 + a02 * c02;
    if (outDet) *outDet = det;
    if (std::fabs(det) < 1e-8f) return false;

    float invDet = 1.0f / det;
    // adjugate (transpose of cofactor matrix) * 1/det
    m[0] = c00 * invDet; m[1] = c10 * invDet; m[2] = c20 * invDet;
    m[3] = c01 * invDet; m[4] = c11 * invDet; m[5] = c21 * invDet;
    m[6] = c02 * invDet; m[7] = c12 * invDet; m[8] = c22 * invDet;
    return true;
}

// ----- factories -----
Matrix3x3 Matrix3x3::Identity() { Matrix3x3 M; return M.setIdentity(); }
Matrix3x3 Matrix3x3::Translate(float x, float y) { Matrix3x3 M; return M.setTranslate(x, y); }
Matrix3x3 Matrix3x3::Scale(float sx, float sy) { Matrix3x3 M; return M.setScale(sx, sy); }
Matrix3x3 Matrix3x3::RotRad(float radians) { Matrix3x3 M; return M.setRotRad(radians); }
Matrix3x3 Matrix3x3::RotDeg(float degrees) { Matrix3x3 M; return M.setRotDeg(degrees); }

// ----- free operators -----
Matrix3x3 operator*(Matrix3x3 const& a, Matrix3x3 const& b) {
    Matrix3x3 out;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            out.m[Matrix3x3::idx(r, c)] =
                a.m[Matrix3x3::idx(r, 0)] * b.m[Matrix3x3::idx(0, c)] +
                a.m[Matrix3x3::idx(r, 1)] * b.m[Matrix3x3::idx(1, c)] +
                a.m[Matrix3x3::idx(r, 2)] * b.m[Matrix3x3::idx(2, c)];
        }
    }
    return out;
}

Vector2D operator*(Matrix3x3 const& M, Vector2D const& v) {
    float x = M.at(0, 0) * v.x + M.at(0, 1) * v.y + M.at(0, 2) * 1.0f;
    float y = M.at(1, 0) * v.x + M.at(1, 1) * v.y + M.at(1, 2) * 1.0f;
    float w = M.at(2, 0) * v.x + M.at(2, 1) * v.y + M.at(2, 2) * 1.0f;
    if (w != 0.0f) { x /= w; y /= w; }
    return Vector2D{ x, y };
}

// ----- legacy-style wrappers -----
void Mtx33Identity(Matrix3x3& out) { out.setIdentity(); }
void Mtx33Translate(Matrix3x3& out, float x, float y) { out.setTranslate(x, y); }
void Mtx33Scale(Matrix3x3& out, float sx, float sy) { out.setScale(sx, sy); }
void Mtx33RotRad(Matrix3x3& out, float radians) { out.setRotRad(radians); }
void Mtx33RotDeg(Matrix3x3& out, float degrees) { out.setRotDeg(degrees); }
void Mtx33Transpose(Matrix3x3& out, Matrix3x3 const& in) { out = in; out.transpose(); }
void Mtx33Inverse(Matrix3x3* pOut, float* det, Matrix3x3 const& in) {
    if (!pOut) return;
    *pOut = in;
    if (!pOut->invert(det)) {
        // Singular; leave as-is (or set identity if you prefer)
    }
}
