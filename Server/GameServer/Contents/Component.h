#pragma once

#include "Util\Vector3.h"

class GameObject;

class Component : public std::enable_shared_from_this<Component>
{
public:
	struct CreateKey 
	{
		friend class GameObject;
		private: CreateKey() {} // 생성자가 private이라 GameObject만 생성 가능
	};
	Component(CreateKey, std::weak_ptr<GameObject> owner) : _owner(owner) {}
	virtual ~Component() {}

	inline std::weak_ptr<GameObject> GetOwner() { return _owner; }

private:
	std::weak_ptr<GameObject> _owner;
};

class TransformComponent : public Component
{
public:
	TransformComponent(CreateKey key, std::weak_ptr<GameObject> owner) : Component(key, owner) {}

	const Vector3& GetPos() const { return _position; }
	double GetYaw() const { return _yaw; }

private:
	Vector3 _position;
	double _yaw = 0;
};
