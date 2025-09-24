#pragma once
#include <cstddef>

namespace Framework {

    class Vector2D; // forward declaration

    class Matrix3x3 {
    public:
        // Row-major storage: element at (r,c) is m[r*3 + c]
        float m[9];

        // ----- helpers -----
        static constexpr std::size_t idx(std::size_t r, std::size_t c) { return r * 3 + c; }

        // ----- ctors/assign -----
        Matrix3x3();                                   // zero matrix
        explicit Matrix3x3(float const* pArr);         // copy 9 floats
        Matrix3x3(float _00, float _01, float _02,
            float _10, float _11, float _12,
            float _20, float _21, float _22);    // element-wise

        Matrix3x3(Matrix3x3 const&) = default;
        Matrix3x3& operator=(Matrix3x3 const&) = default;

        // ----- element access -----
        float& at(std::size_t r, std::size_t c);
        float  at(std::size_t r, std::size_t c) const;

        // ----- set builders (mutating) -----
        Matrix3x3& setIdentity();
        Matrix3x3& setTranslate(float x, float y);
        Matrix3x3& setScale(float sx, float sy);
        Matrix3x3& setRotRad(float radians);
        Matrix3x3& setRotDeg(float degrees);

        // ----- algebra -----
        Matrix3x3& operator*=(Matrix3x3 const& rhs);
        Matrix3x3& transpose();              // in-place
        float      determinant() const;
        bool       invert(float* outDet = nullptr); // in-place inverse, false if singular

        // ----- factories -----
        static Matrix3x3 Identity();
        static Matrix3x3 Translate(float x, float y);
        static Matrix3x3 Scale(float sx, float sy);
        static Matrix3x3 RotRad(float radians);
        static Matrix3x3 RotDeg(float degrees);

        // friend for mat-mat multiply
        friend Matrix3x3 operator*(Matrix3x3 const& a, Matrix3x3 const& b);
    };

    // ----- free functions (compat with your old API) -----
    Matrix3x3 operator*(Matrix3x3 const& a, Matrix3x3 const& b);
    Vector2D  operator*(Matrix3x3 const& M, Vector2D const& v);

    void Mtx33Identity(Matrix3x3& out);
    void Mtx33Translate(Matrix3x3& out, float x, float y);
    void Mtx33Scale(Matrix3x3& out, float sx, float sy);
    void Mtx33RotRad(Matrix3x3& out, float radians);
    void Mtx33RotDeg(Matrix3x3& out, float degrees);
    void Mtx33Transpose(Matrix3x3& out, Matrix3x3 const& in);
    void Mtx33Inverse(Matrix3x3* pOut, float* det, Matrix3x3 const& in);

}