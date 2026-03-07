#pragma once

#include <DirectXMath.h>
#include <list>
#include "Component.h"
#include "Geometory.h"

class GameObject 
{
protected:
	DirectX::XMFLOAT3 m_position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f); // オブジェクトの座標 
	DirectX::XMFLOAT3 m_rotation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 m_scale	 = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	DirectX::XMFLOAT4X4 m_world =
		DirectX::XMFLOAT4X4(
			1.0f,0.0f,0.0f,0.0f,
			0.0f,1.0f,0.0f,0.0f,
			0.0f,0.0f,1.0f,0.0f,
			0.0f,0.0f,0.0f,1.0f
		);
	std::list<Component*> m_Component;
	std::list<GameObject*> m_ChildGameObject;

	bool m_Destroy = false;

public:
	//--- 基本処理 
	GameObject() {}
	virtual ~GameObject() {}

	//--- 座標操作 
	DirectX::XMFLOAT3 GetPosition() { return m_position; }
	DirectX::XMFLOAT3 GetRotation() { return m_rotation; }
	DirectX::XMFLOAT3 GetScale() { return m_scale; }
	DirectX::XMFLOAT4X4* GetWorld() { return &m_world; }

	void SetPosition(DirectX::XMFLOAT3 position) { m_position = position; }
	void SetRotation(DirectX::XMFLOAT3 rotation) { m_rotation = rotation; }
	void SetScale(DirectX::XMFLOAT3 scale) { m_scale = scale; }

	// 前方向ベクトル取得
	DirectX::XMFLOAT3 GetForward()
	{
		//DirectX::XMFLOAT4X4 rot;
		//DirectX::XMStoreFloat4x4(&rot,
		//	DirectX::XMMatrixRotationRollPitchYaw(
		//		m_Rotation.x, m_Rotation.y, m_Rotation.z));

		DirectX::XMFLOAT3 forward;
		//forward.x = rot._31;
		//forward.y = rot._32;
		//forward.z = rot._33;
		forward.x = m_world._31;
		forward.y = m_world._32;
		forward.z = m_world._33;

		return forward;
	}

	void SetDestroy()
	{
		m_Destroy = true;
	}

	bool Destroy()
	{
		if (m_Destroy)
		{
			UninitBase();
			delete this;
			return true;
		}
		return false;
	}

	virtual void Init() {}
	virtual void Uninit() {}
	virtual void Update() {}
	virtual void Draw() {}

	template<typename T>
	T* AddComponent()
	{
		for (Component* component : m_Component)
		{
			if (typeid(*component) == typeid(T))
			{
				return (T*)component;
			}
		}
		return nullptr;
	}

	template<typename T>
	T* GetComponent()
	{
		for (Component* component : m_Component)
		{
			if (typeid(*component) == typeid(T))
			{
				return (T*)component;
			}
		}
		return nullptr;
	}
	template<typename T>
	T* AddChild()
	{
		T* child = new T();
		m_ChildGameObject.push_back(child);
		child->InitBase();
		return child;
	}
	void InitBase()
	{
		Init();
	}

	void UninitBase()
	{
		Uninit();

		for (Component* component : m_Component)
		{
			component->Uninit();
			delete component;
		}
		m_Component.clear();
	}

	void UpDateBase()
	{
		for (Component* component : m_Component)
		{
			component->Update();
		}

		Update();
	}

	void DrawBase(DirectX::XMFLOAT4X4* ParentMatrix = nullptr)
	{
		// マトリクス設定
		DirectX::XMFLOAT4X4 world;
		DirectX::XMMATRIX scale, rot, trans, w;
		scale = DirectX::XMMatrixScaling(m_scale.x,m_scale.y,m_scale.z);
		rot = DirectX::XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
		trans = DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
		w = scale * rot;
		if (ParentMatrix)
		{
			w *= DirectX::XMLoadFloat4x4(ParentMatrix);
		}
		DirectX::XMStoreFloat4x4(&m_world, w);
		DirectX::XMStoreFloat4x4(&world, scale*w);

		for (GameObject* child : m_ChildGameObject)
		{
			child->DrawBase(&world);
		}

		Geometory::SetWorld(world);

		for (Component* component : m_Component)
		{
			component->Draw();
		}

		Draw();
	}

};