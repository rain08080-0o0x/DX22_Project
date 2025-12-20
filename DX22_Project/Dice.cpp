#include "Dice.h"
#include "Geometory.h"
#include "Transfer.h"
#include "Input.h"

using namespace DirectX;

// 摩擦
const float friction = 0.97f;
// 落下加速度
const float fall = 0.02f;
// 止まる加速度
const float under = 0.01f;
// 壁
const float wall = 5.0f;

Dice::Dice()
	: m_isGround(false)
	, m_pCamera(nullptr)
	, m_velocity({0.0f,0.0f,0.0f})
	, tran(Transfer::GetInstance())
{
	m_pos = { 0.0f,0.0f,0.0f };
	m_size = {1.0f,1.0f,1.0f};
	// 角運動量用変数の初期化
	m_rot = { 0,0,0,1 };
	m_angVel = { 0,0,0 };
	m_mass = 1.0f;
	m_restitution = 0.3f;
	m_mu = 0.6f;
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
	const float halfY = m_size.y * 0.5f;
	const float g = tran.dice.ground;
	const DirectX::XMVECTOR n = XMVectorSet(0, 1, 0, 0);

	if (m_pos.y - halfY < g)
	{
		m_pos.y = g + halfY;
		m_isGround = true;

		// 接触点 r
		XMVECTOR q = XMVectorSet(m_rot.x, m_rot.y, m_rot.z, m_rot.w);
		XMVECTOR r_local = XMVectorSet(0, -halfY, 0, 0);
		XMVECTOR r = XMVector3Rotate(r_local, q);

		// 接触点速度 v + w×r
		XMVECTOR v = XMVectorSet(m_velocity.x, m_velocity.y, m_velocity.z, 0);
		XMVECTOR w = XMVectorSet(m_angVel.x, m_angVel.y, m_angVel.z, 0);
		XMVECTOR vc = XMVectorAdd(v, XMVector3Cross(w, r));
		float vn = XMVectorGetX(XMVector3Dot(vc, n));

		if (vn < 0.0f)
		{
			// インパルス（簡略）
			float jn = -(1.0f + m_restitution) * vn;

			v = XMVectorAdd(v, XMVectorScale(n, jn / m_mass));

			// 摩擦 → 回転を生む
			XMVECTOR vt = XMVectorSubtract(vc, XMVectorScale(n, vn));
			float len = XMVectorGetX(XMVector3Length(vt));
			if (len > 0.0001f)
			{
				XMVECTOR t = XMVectorScale(vt, 1.0f / len);
				float jt = -m_mu * jn;
				XMVECTOR Jt = XMVectorScale(t, jt);

				v = XMVectorAdd(v, XMVectorScale(Jt, 1.0f / m_mass));
				w = XMVectorAdd(w, XMVector3Cross(r, Jt));
			}

			XMStoreFloat3(&m_velocity, v);
			XMStoreFloat3(&m_angVel, w);
		}
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

	// ====== 回転の積分 ======
	XMVECTOR q = XMVectorSet(m_rot.x, m_rot.y, m_rot.z, m_rot.w);
	XMVECTOR w = XMVectorSet(m_angVel.x, m_angVel.y, m_angVel.z, 0);
	XMVECTOR dq = 0.5f * XMQuaternionMultiply(w, q);

	q = XMVectorAdd(q, dq);
	q = XMQuaternionNormalize(q);
	XMStoreFloat4(&m_rot, q);

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

	XMVECTOR q = XMVectorSet(m_rot.x, m_rot.y, m_rot.z, m_rot.w);
	XMMATRIX R = XMMatrixRotationQuaternion(q);

	XMMATRIX W = S * R * T;


	XMMATRIX world;
	world = XMMatrixTranspose(W);
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
