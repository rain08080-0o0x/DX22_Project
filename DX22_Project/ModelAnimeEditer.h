#pragma once
#include <DirectXMath.h>
#include "Camera.h"

class ModelAnimeEditer
{
public:
	ModelAnimeEditer();
	~ModelAnimeEditer();
	void Update();
	void Draw();
	void SetCamera(Camera*camera);
private:
	Camera* m_pCamera;

	DirectX::XMFLOAT3 subAngle;
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 size;
	DirectX::XMFLOAT3 rotate;
};

