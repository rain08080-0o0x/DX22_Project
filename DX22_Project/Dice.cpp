#include "Dice.h"
#include "Geometory.h"
#include "Transfer.h"
#include "Input.h"

using namespace DirectX;

// 摩擦
const float friction = 0.97f;
// 落下加速度
const float fall = 0.02f;;
// 地面
const float ground = 0.0f;
// 止まる加速度
const float under = 0.01f;
// 壁
const float wall = 5.0f;

Dice::Dice()
	: m_isGround(false)
	, m_pCamera(nullptr)
	, m_velocity({0.0f,0.0f,0.0f})
{
	m_pos = { 0.0f,0.0f,0.0f };
	m_size = {1.0f,1.0f,1.0f};

	for (int i = 0; i < 10; i++)
	{
		m_pModel[i] = new Model;
		m_pModel[i]->Load("");
	}
}
Dice::~Dice()
{
}

// 更新処理 
void Dice::Update()
{
	TRAN_INS;

	m_pos = tran.dice.pos;
	m_velocity = tran.dice.velocity;
	if (IsKeyTrigger('R'))
	{
		Roll();
	}


	// 重力
	m_velocity.y -= fall;
	
	// 摩擦処理
	m_velocity.x *= friction;
	m_velocity.y *= friction;
	m_velocity.z *= friction;

	// 移動量分位置情報を更新
	m_pos.x += m_velocity.x;
	m_pos.y += m_velocity.y;
	m_pos.z += m_velocity.z;

	// 地面の当たり判定(仮 -> 角度によってサイズが変動するのでそれようの地面処理を後に制作
	if (m_pos.y - (m_size.y / 2) <= ground)
	{
		m_pos.y = ground + (m_size.y / 2); // めり込み補正
		m_isGround = true;
		m_velocity.y = std::abs(m_velocity.y) * 0.9f; // 0.5は反発係数
	}
	else
	{
		m_isGround = false;
	}

	// それぞれの移動量が落下加速度より遅い場合０に
	if (abs(m_velocity.x) < under) m_velocity.x = 0.0f;
	if (abs(m_velocity.y) < under) m_velocity.y = 0.0f;
	if (abs(m_velocity.z) < under) m_velocity.z = 0.0f;

	// 壁の衝突判定
	if (m_pos.x + m_size.x > wall)
	{
		m_pos.x = wall - m_size.x;
		m_velocity.x *= -1;
	}
	else if (m_pos.x - m_size.x < -wall)
	{
		m_pos.x = -wall + m_size.x;
		m_velocity.x *= -1;
	}
	if (m_pos.z + m_size.z > wall)
	{
		m_pos.z = wall - m_size.z;
		m_velocity.z *= -1;
	}
	else if (m_pos.z - m_size.z < -wall)
	{
		m_pos.z = -wall + m_size.z;
		m_velocity.z *= -1;
	}

	// 当たり判定の更新
	m_collision.center = m_pos;
	m_collision.size = m_size;

	tran.dice.pos = m_pos;
	tran.dice.velocity = m_velocity;
}

// 描画処理 
void Dice::Draw()
{
	XMMATRIX T =
		XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
	XMMATRIX S =
		XMMatrixScaling(m_size.x, m_size.y, m_size.z);
	XMMATRIX world;
	world = XMMatrixTranspose(S * T);
	XMFLOAT4X4 xWorld;
	XMStoreFloat4x4(&xWorld, world);
	Geometory::SetWorld(xWorld);
	Geometory::DrawBox();
}

// カメラの設定 
void Dice::SetCamera(Camera* pCamera)
{
	m_pCamera = pCamera;
}
Collision::Box Dice::GetCollision()
{
	return m_collision;
}

void Dice::Roll()
{
	float a, b, c;
	a = rand();
	b = (0x10000 >> 0b1) - 1;
	c = (a / b) * 0b10;

	float random = (a / (b / 2.0f)) - 1.0f;

	// 一旦のRoll処理
	m_velocity.x = random;
	m_velocity.y = random;
	m_velocity.z = random;

	m_isGround = false;
}
