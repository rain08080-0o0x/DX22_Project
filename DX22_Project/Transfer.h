#pragma once

#include <DirectXMath.h>

#define TRAN_INS Transfer &tran = Transfer::GetInstance();
#define TRAN_INS_Get Transfer &tran = Transfer::GetInstance();tran

class Transfer
{
private:
	Transfer() = default;
	~Transfer() = default;

	struct CameraInfo
	{
		DirectX::XMFLOAT3 eye;
		DirectX::XMFLOAT3 look;
	};

	struct PlayerInfo
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 velocity;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 lcokColor;
	};
	struct DiceInfo
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 velocity;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 lcokColor;
		const float ground = 0.0f;
		// ここからtest用		   
		DirectX::XMFLOAT4X4 world;	// ワールド座標系
		DirectX::XMFLOAT4X4 obj;	// オブジェクト座標系
	};
public:
	static Transfer& GetInstance()
	{
		static Transfer instance;
		return instance;
	}
public:
	PlayerInfo player;
	DiceInfo dice;
	CameraInfo camera;
};
