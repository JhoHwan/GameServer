#include "pch.h"
#include "GameObject.h"
#include "Component.h"
#include "Struct.pb.h"

GameObject::GameObject() 
{
	static uint64 id = 0;
	_id = id;
	id += 1;
}

GameObject::~GameObject()
{

}

void GameObject::GetObjectInfo(OUT Protocol::ObjectInfo* info) const
{
	info->set_id(GetId());
	Protocol::Vector3* pos = info->mutable_pos();
	pos->CopyFrom(_transform->GetPos().ToProto());
	info->set_yaw(_transform->GetYaw());
}

void GameObject::Init()
{
	_transform = AddComponent<TransformComponent>();
}
