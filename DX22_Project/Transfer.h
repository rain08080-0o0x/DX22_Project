#pragma once

#include <DirectXMath.h>
#define TRAN_INS Transfer &tran = Transfer::GetInstance();
#define TRAN_INS_G Transfer &tran = Transfer::GetInstance();tran

class Transfer
{
private:
	Transfer() = default;
	~Transfer() = default;
public:
	static Transfer& GetInstance()
	{
		static Transfer instance;
		return instance;
	}
	struct Deme
	{
		float deme;
	};
public:
	float m_posX;
	float m_posY;
	float m_posZ;
	float m_power;
	float m_maxPower;
	DirectX::XMFLOAT2 WallSize;
	float deme[10];
};

