#pragma once

#include <DirectXMath.h>
#include "Model.h"

class GameObject 
{
protected:
	DirectX::XMFLOAT3 m_pos; // オブジェクトの座標 

public:
	//--- 基本処理 
	GameObject();
	virtual ~GameObject() {}

	virtual void Init() {}
	virtual void Uninit() {}
	virtual void Update() {}
	virtual void Draw() {}

	//--- 座標操作 
	DirectX::XMFLOAT3 GetPos();
	void SetPos(DirectX::XMFLOAT3 pos);

};