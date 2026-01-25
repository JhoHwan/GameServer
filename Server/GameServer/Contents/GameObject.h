#pragma once

#include <typeindex>
#include <type_traits>
#include "Component.h"

using GameObjectRef = std::weak_ptr<class GameObject>;
using ObjectId = uint64;

namespace Protocol
{
	class ObjectInfo;
}

class TransformComponent;

template <typename T>
concept ComponentType = std::is_base_of_v<Component, T>;

template <typename T>
concept GameObjectType = std::is_base_of_v<GameObject, T>;

class GameObject : public enable_shared_from_this<GameObject>
{
protected:
	GameObject();
	virtual void Init();

public:
	template <GameObjectType T, typename... Args>
	static std::shared_ptr<T> Create(Args&&... args)
	{
		std::shared_ptr<T> newObject{ std::make_shared<T>(std::forward<Args>(args)...) };
		newObject->Init();
		return newObject;
	}

	virtual ~GameObject();

	virtual void Destroy() {}

public:
	inline ObjectId GetId() const { return _id; }
	inline std::shared_ptr<TransformComponent> Transform() { return _transform; }

	template<ComponentType T>
	std::shared_ptr<T> GetComponent();

	template<ComponentType T, typename... Args>
	std::shared_ptr<T> AddComponent(Args&&... args);

	shared_ptr<class Field> GetField() { return _field; }
	void SetField(shared_ptr<class Field> field) { _field = field; }

	void GetObjectInfo(Protocol::ObjectInfo* info) const;

private:
	ObjectId _id;
	std::unordered_map<std::type_index, std::shared_ptr<Component>> _components;

	std::shared_ptr<TransformComponent> _transform;
	std::shared_ptr<class Field> _field;

};

template<ComponentType T>
inline shared_ptr<T> GameObject::GetComponent()
{
	if (_components.find(typeid(T)) == _components.end()) return nullptr;
	shared_ptr<T> component = static_pointer_cast<T>(_components[typeid(T)]);

	return component;
}

template<ComponentType T, typename... Args>
inline shared_ptr<T> GameObject::AddComponent(Args&&... args)
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
