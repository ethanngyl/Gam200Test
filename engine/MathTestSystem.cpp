#include "MathTestSystem.h"   // Include the header for our new system
#include "Vector2D.h"         // Include your math library headers
#include "Matrix3x3.h"
#include <iomanip>            // For formatting the output nicely

namespace Framework {

    // Helper function to print a Vector2D (local to this file)
    static void printVector(const Vector2D& v, const char* name) {
        std::cout << name << ": (" << v.x << ", " << v.y << ")\n";
    }

    // Helper function to print a Matrix3x3 (local to this file)
    static void printMatrix(const Matrix3x3& m, const char* name) {
        std::cout << "--- " << name << " ---\n";
        for (int r = 0; r < 3; ++r) {
            std::cout << "| ";
            for (int c = 0; c < 3; ++c) {
                // std::setw sets the width for cleaner alignment
                std::cout << std::setw(8) << m.at(r, c) << " ";
            }
            std::cout << "|\n";
        }
        std::cout << '\n';
    }

    void MathTestSystem::Initialize()
    {
        std::cout << "\n\n==============================================\n";
        std::cout << "        RUNNING MATH LIBRARY TESTS\n";
        std::cout << "==============================================\n\n";

        // Set output for floating point numbers to be more readable
        std::cout << std::fixed << std::setprecision(2);

        // ========== 1. VECTOR2D TESTS ==========
        std::cout << "========== 1. VECTOR2D TESTS ==========\n";
        Vector2D v1(3.0f, 4.0f);
        Vector2D v2(1.0f, 2.0f);

        printVector(v1, "v1");
        std::cout << "Length of v1: " << v1.length() << " (Expected: 5.00)\n";

        Vector2D v1_normalized = v1.normalized();
        printVector(v1_normalized, "v1 normalized");
        std::cout << "Length of normalized v1: " << v1_normalized.length() << " (Expected: 1.00)\n\n";

        Vector2D v_add = v1 + v2;
        printVector(v_add, "v1 + v2");

        float dot_product = Vector2D::dot(v1, v2);
        std::cout << "Dot product of v1 and v2: " << dot_product << " (Expected: 11.00)\n\n";

        // ========== 2. MATRIX3X3 CREATION TESTS ==========
        std::cout << "========== 2. MATRIX3X3 CREATION TESTS ==========\n";
        Matrix3x3 m_identity = Matrix3x3::Identity();
        printMatrix(m_identity, "Identity Matrix");

        Matrix3x3 m_rot = Matrix3x3::RotDeg(90.0f);
        printMatrix(m_rot, "Rotation Matrix (90 deg)");

        // ========== 3. TRANSFORMATION TESTS ==========
        std::cout << "========== 3. TRANSFORMATION TESTS ==========\n";
        Vector2D p(10.0f, 0.0f);
        printVector(p, "Original Point p");

        Matrix3x3 T = Matrix3x3::Translate(5.0f, 10.0f);
        Matrix3x3 R = Matrix3x3::RotDeg(90.0f);
        Matrix3x3 S = Matrix3x3::Scale(2.0f, 2.0f);
        Matrix3x3 M = T * R * S;
        printMatrix(M, "Combined Matrix (T * R * S)");

        Vector2D p_transformed = M * p;
        std::cout << "Applying transformation to p...\n";
        std::cout << "Expected result after Scale -> Rotate -> Translate is (5.00, 30.00)\n";
        printVector(p_transformed, "Final Transformed Point");

        std::cout << "\n==============================================\n";
        std::cout << "         MATH LIBRARY TESTS COMPLETE\n";
        std::cout << "==============================================\n\n";
    }

    void MathTestSystem::Update(float dt)
    {
        // This system does not need per-frame updates.
    }

    void MathTestSystem::SendMessage(Message* message)
    {
        // This system does not need to handle messages.
    }

} // namespace Framework