#pragma once
#include "Camera.h" 
#include "Input.h" 

class CameraDebug : public Camera
{
public:
	CameraDebug();
	~CameraDebug();

	void Update() final;

	void SetLook(DirectX::XMFLOAT3 set)final;
	void SetPos(DirectX::XMFLOAT3 set)final;

	void LockPos(bool set)final;
private:
	float m_radXZ;
	float m_radY;
	float m_radius;
	bool isLock;
};