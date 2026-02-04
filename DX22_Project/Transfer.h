#pragma once

#include <DirectXMath.h>
#include "Defines.h"

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
	struct ObjectfromAtoB
	{
		DirectX::XMFLOAT3 A;
		DirectX::XMFLOAT3 Avel;
		DirectX::XMFLOAT3 AangVel;
		DirectX::XMFLOAT3 B;
		DirectX::XMFLOAT3 Bvel;
		DirectX::XMFLOAT3 BangVel;
	};
	struct PlayerInfo
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 velocity;
		float hp = 0.0f;
		float maxHp = 0.0f;
		float moveSpeed = 0.0f;
		float dashDistance = 0.0f;
		float dashCooldown = 0.0f;
		float dashDuration = 0.0f;
		float stageSize = 0.0f;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 lcokColor;
	};
	struct DiceInfo
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 velocity;
		DirectX::XMFLOAT4 rot;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 lcokColor;
		const float ground = 0.0f;
		// ここからtest用		   
		DirectX::XMFLOAT4X4 world;	// ワールド座標系
		DirectX::XMFLOAT4X4 obj;	// オブジェクト座標系
		DirectX::XMFLOAT3 virtualVelocity;	//仮想運動量
		int currentFaceNumber[MAX_DICE];	// 現在の表面ナンバー
		float underVel = 0.0f; // これ以下の運動量なら停止用変数
	};
	struct UIobj
	{
		DirectX::XMFLOAT2 pos;
		DirectX::XMFLOAT2 size;
		DirectX::XMFLOAT4 color = { 1,1,1,1 };
	};
	struct UIInfo
	{
		UIobj role;
	};
	struct ModelInfo
	{
		DirectX::XMFLOAT3 subAngle;
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 rotate;
	};
	struct ModelBodyInfo
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 angle;
		DirectX::XMFLOAT3 jointRightArmPos;
		DirectX::XMFLOAT3 jointLeftArmPos;
		DirectX::XMFLOAT3 jointRightLegPos;
		DirectX::XMFLOAT3 jointLeftLegPos;
	};
	struct ModelEditer
	{
		ModelInfo armRight1;
		ModelInfo armRight2;
		ModelInfo armLeft1;
		ModelInfo armLeft2;
		ModelInfo legRight1;
		ModelInfo legRight2;
		ModelInfo legLeft1;
		ModelInfo legLeft2;
		ModelBodyInfo body;
	};
	struct Tyabudai
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 rotate;
	};
	struct Arrow
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 rotate;
		DirectX::XMFLOAT3 scale;
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
	ObjectfromAtoB obj;
	UIInfo diceui;
	DirectX::XMFLOAT2 mousePos;
	UIobj yukari;
	UIobj fuki;
	Tyabudai tyabu;
	Tyabudai tyawan;
	ModelEditer modelediter;
	Arrow arrow;
};