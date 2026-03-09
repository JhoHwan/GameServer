#pragma once
#include <cmath>
#include "Struct.pb.h"
#include "nlohmann/json.hpp"

struct Vector3
{
public:
	float x;
	float y;
	float z;

public:
	Vector3() : Vector3(0, 0, 0) {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
	Vector3(const Protocol::Vector3& vec) : Vector3(vec.x(), vec.y(), vec.z()) {}

	Protocol::Vector3 ToProto() const
	{
		Protocol::Vector3 vec;
		vec.set_x(x);
		vec.set_y(y);
		vec.set_z(z);
		return vec;
	}

	static Vector3 Zero() { return Vector3(0, 0, 0); }

public:
	Vector3 operator+(const Vector3& other) const { return Vector3(x + other.x, y + other.y, z + other.z); }
	Vector3 operator-(const Vector3& other) const { return Vector3(x - other.x, y - other.y, z - other.z); }
	Vector3 operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
	Vector3 operator/(float scalar) const { return Vector3(x / scalar, y / scalar, z / scalar); }

	bool operator==(const Vector3& other) const { return x == other.x && y == other.y && z == other.z; }
	bool operator!=(const Vector3& other) const { return !(*this == other); }

	Vector3& operator+=(const Vector3& other) { x += other.x; y += other.y; z += other.z; return *this; }
	Vector3& operator-=(const Vector3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
	Vector3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }

public:
	float LengthSquared() const { return x * x + y * y + z * z; }
	float Length() const { return std::sqrt(LengthSquared()); }

	void Normalize()
	{
		float len = Length();
		if (len < 0.000001f) return;
		x /= len; y /= len; z /= len;
	}

	Vector3 GetNormalized() const
	{
		Vector3 v = *this;
		v.Normalize();
		return v;
	}

	float Dot(const Vector3& other) const { return x * other.x + y * other.y + z * other.z; }
	Vector3 Cross(const Vector3& other) const
	{
		return Vector3(
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x
		);
	}

	static float Dist(const Vector3& v1, const Vector3& v2) { return (v1 - v2).Length(); }
	static float Dist2D(const Vector3& v1, const Vector3& v2)
	{
		return std::sqrt((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y));
	}
	static float DistSquared(const Vector3& v1, const Vector3& v2) { return (v1 - v2).LengthSquared(); }
};

inline void from_json(const nlohmann::json& j, Vector3& v)
{
	j.at("X").get_to(v.x);
	j.at("Y").get_to(v.y);
	j.at("Z").get_to(v.z);
}