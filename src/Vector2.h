#pragma once

#include <cmath>

class Vector2
{
public:
    float x, y;

    Vector2() : x(0.0f), y(0.0f) {}
    Vector2(float x, float y) : x(x), y(y) {}

    // Basic operations
    Vector2 operator-() const
    {
        return Vector2(-x, -y);
    }

    Vector2 operator+(const Vector2& other) const
    {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2& operator+=(const Vector2& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2 operator-(const Vector2& other) const
    {
        return Vector2(x - other.x, y - other.y);
    }

    Vector2& operator-=(const Vector2& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2 operator*(float scalar) const
    {
        return Vector2(x * scalar, y * scalar);
    }

    Vector2& operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    // Dot product
    float dot(const Vector2& other) const
    {
        return x * other.x + y * other.y;
    }

    // Reflect
    Vector2 reflect(const Vector2& normal) const
    {
        return *this - normal * (2.0f * this->dot(normal));
    }

    // Length calculations
    float lengthSquared() const
    {
        return x * x + y * y;
    }

    float length() const
    {
        return std::sqrt(lengthSquared());
    }

    // Normalization
    Vector2 normalized() const
    {
        float len = length();
        if (len < 0.0001f) return Vector2(); // avoid division by zero
        float invLen = 1.0f / len;
        return Vector2(x * invLen, y * invLen);
    }

    void normalize()
    {
        float len = length();
        if (len < 0.0001f) return; // avoid division by zero
        float invLen = 1.0f / len;
        x *= invLen;
        y *= invLen;
    }
};