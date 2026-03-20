#pragma once

#include <typeindex>
#include <type_traits>
#include "Component.h"

class Field;
using GameObjectRef = std::weak_ptr<class GameObject>;
namespace Protocol
{
	class ObjectInfo;
}

class TransformComponent;

enum class EObjectType : uint8
{
	None = 0,
	Player = 1,
	Monster = 2,
	NPC = 3,
	Projectile = 4,
};

template <typename T>
concept ComponentType = std::is_base_of_v<Component, T>;

template <typename T>
concept GameObjectType = std::is_base_of_v<GameObject, T>;

class GameObject : public enable_shared_from_this<GameObject>
{
	friend class Field;
public:
	virtual ~GameObject();

protected:
	GameObject();
	virtual void Init();
	virtual void OnSpawn();
	virtual void OnDespawn();


public:
	uint16 GetTag() const
	{
		return static_cast<uint16>(GetId() >> OBJECT_TAG_SHIFT);
	}

	EObjectType GetType() const
	{
		return static_cast<EObjectType>(GetId() >> 60);
	}

	uint64 GetInstanceID() const
	{
		return (GetId() & INSTANCE_MASK);
	}

	uint16 GetSubID() const
	{
		return static_cast<uint16>((GetId() >> OBJECT_TAG_SHIFT) & SUB_ID_MASK);
	}

protected:
	constexpr static uint16 MakeTag(EObjectType objectType, uint16 subId)
	{
		return (static_cast<uint16>(objectType) << OBJECT_TYPE_SHIFT) | (subId & SUB_ID_MASK);
	}


private:
	template <GameObjectType T, typename... Args>
	static std::shared_ptr<T> Create(Args&&... args)
	{
		std::shared_ptr<T> newObject{ std::make_shared<T>(std::forward<Args>(args)...) };
		newObject->SetId(T::TAG);
		newObject->Init();

		return newObject;
	}

	void SetId(uint16 tag) { _id = MakeId(tag, _instanceIdGenerator.fetch_add(1)); }
	void SetField(const shared_ptr<Field>& field) { _field = field; }
	static uint64 MakeId(uint16 objectTag, uint64 instanceID)
	{ return (static_cast<uint64>(objectTag) << OBJECT_TAG_SHIFT) | (instanceID & INSTANCE_MASK); }


public:
	uint64 GetId() const { return _id; }
	std::shared_ptr<TransformComponent> Transform() { return _transform; }
	const std::shared_ptr<TransformComponent>& Transform() const { return _transform; }

	template<ComponentType T>
	std::shared_ptr<T> GetComponent();

	template<ComponentType T, typename... Args>
	std::shared_ptr<T> AddComponent(Args&&... args);
	shared_ptr<Field> GetField() { return _field; }
	void GetObjectInfo(Protocol::ObjectInfo* info) const;

private:
	static inline atomic<uint64> _instanceIdGenerator{ 1 };
	static constexpr uint64 OBJECT_TAG_SHIFT = 48;
	static constexpr uint64 OBJECT_TYPE_SHIFT = 12;
	static constexpr uint64 INSTANCE_MASK = 0x0000FFFFFFFFFFFFULL;
	static constexpr uint64 SUB_ID_MASK = 0x0FFFULL;

	uint64 _id = 0;
	std::unordered_map<std::type_index, std::shared_ptr<Component>> _components;
	std::shared_ptr<TransformComponent> _transform;
	std::shared_ptr<Field> _field;

};

template<ComponentType T>
shared_ptr<T> GameObject::GetComponent()
{
	if (_components.find(typeid(T)) == _components.end()) return nullptr;
	shared_ptr<T> component = static_pointer_cast<T>(_components[typeid(T)]);

	return component;
}

template<ComponentType T, typename... Args>
shared_ptr<T> GameObject::AddComponent(Args&&... args)
{
	if (_components.find(typeid(T)) != _components.end()) return nullptr;
	shared_ptr<T> component = make_shared<T>(
		Component::CreateKey{}, 
		shared_from_this(), 
		std::forward<Args>(args)...
	);
	_components[typeid(T)] = component;
	return component;
}


