#include "Dice.h"
#include "Geometory.h"
#include "Transfer.h"
#include "Input.h"

#include "ShaderList.h"
#include "Sprite.h"
#include "Shader.h"
#include "Defines.h"

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
	, m_velocity({ 0.0f,0.0f,0.0f })
	, tran(Transfer::GetInstance())
{
	m_pos = { 0.0f,0.0f,0.0f };
	m_size = { 1.0f,1.0f,1.0f };
	// 角運動量用変数の初期化
	m_rot = { 0,0,0,1 };
	m_angVel = { 0,0,0 };
	m_mass = 1.0f;
	m_restitution = 0.3f;
	m_mu = 0.6f;
	// ここからテスト用変数の初期化
	// テスト関数でのみ使用
	world = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};
	obj = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};
	tran.dice.world = world;
	tran.dice.obj = obj;
	// 頂点決め
	float size = 0.5f;
	vertex[0] = {m_pos.x - size ,m_pos.y - size,m_pos.z - size};// 左下後ろ
	vertex[1] = {m_pos.x - size ,m_pos.y - size,m_pos.z + size};// 左下手前
	vertex[2] = {m_pos.x - size ,m_pos.y + size,m_pos.z - size};// 左上後ろ
	vertex[3] = {m_pos.x - size ,m_pos.y + size,m_pos.z + size};// 左上手前
	vertex[4] = {m_pos.x + size ,m_pos.y - size,m_pos.z - size};// 右下後ろ
	vertex[5] = {m_pos.x + size ,m_pos.y - size,m_pos.z + size};// 右下手前
	vertex[6] = {m_pos.x + size ,m_pos.y + size,m_pos.z - size};// 右上後ろ
	vertex[7] = {m_pos.x + size ,m_pos.y + size,m_pos.z + size};// 右上手前

	m_pModel = new Model();

	if (!m_pModel->Load("Assets/Model/Dice/dice.fbx", 1.f, Model::ZFlip)) { // 倍率と反転は省略可
		MessageBox(NULL, "Not found for dice", "Error", MB_OK); // エラーメッセージの表示
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
	if (IsKeyPress('R'))
	{
		Roll();
	}

	// 重力
	m_velocity.y -= fall;

	// 摩擦処理
	m_velocity.x *= friction;
	m_velocity.y *= friction;
	m_velocity.z *= friction;

	m_angVel.x *= friction;
	m_angVel.y *= friction;
	m_angVel.z *= friction;
	
	// 移動量分位置情報を更新
	m_pos.x += m_velocity.x;
	m_pos.y += m_velocity.y;
	m_pos.z += m_velocity.z;

	// 地面の当たり判定(仮) -> 角度によってサイズが変動するのでそれようの地面処理を後に制作
	const float halfY = m_size.y * 0.5f;
	const float g = tran.dice.ground;
	const DirectX::XMVECTOR n = XMVectorSet(0, 1, 0, 0);

	if (m_pos.y - halfY < g)
	{
		m_pos.y = g + halfY;
		m_isGround = true;
		m_velocity.y = abs(m_velocity.y) * 0.5f;
	}
	else
	{
		m_isGround = false;
	}

	// それぞれの移動量が落下加速度より遅い場合０に
	if (abs(m_velocity.x) < under) m_velocity.x = 0.0f;
	if (abs(m_velocity.y) < under) m_velocity.y = 0.0f;
	if (abs(m_velocity.z) < under) m_velocity.z = 0.0f;
	if (abs(m_angVel.x) < under) m_angVel.x = 0.0f;
	if (abs(m_angVel.y) < under) m_angVel.y = 0.0f;
	if (abs(m_angVel.z) < under) m_angVel.z = 0.0f;

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
	XMVECTOR q = XMVectorSet(m_rot.x, m_rot.y, m_rot.z, m_rot.w);
	XMMATRIX R = XMMatrixRotationQuaternion(q);

	XMMATRIX T = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
	XMMATRIX S = XMMatrixScaling(m_size.x, m_size.y, m_size.z);

	XMMATRIX world = XMMatrixTranspose(S * R * T);


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
	m_velocity.x += random; random = ((float)rand() / 0x7fff) * 2.0f - 1.0f;
	m_velocity.y += random; random = ((float)rand() / 0x7fff) * 2.0f - 1.0f;
	m_velocity.z += random; random = ((float)rand() / 0x7fff) * 2.0f - 1.0f;

	// ===== 角速度 =====
	if(true) // 一旦なし
	{
		m_angVel.x = random * 5.0f; random = ((float)rand() / 0x7fff) * 2.0f - 1.0f;
		m_angVel.y = random * 5.0f; random = ((float)rand() / 0x7fff) * 2.0f - 1.0f;
		m_angVel.z = random * 5.0f; random = ((float)rand() / 0x7fff) * 2.0f - 1.0f;
	}

	m_isGround = false;
}


void Dice::TestUpdate()
{
	//--- 行列配列のそれぞれのやつ
	if(IsKeyTrigger('T'))
		world = tran.dice.world;

	float size = 0.5f;

	vertex[0] = { m_pos.x - size ,m_pos.y - size,m_pos.z - size };// 左下後ろ
	vertex[1] = { m_pos.x - size ,m_pos.y - size,m_pos.z + size };// 左下手前
	vertex[2] = { m_pos.x - size ,m_pos.y + size,m_pos.z - size };// 左上後ろ
	vertex[3] = { m_pos.x - size ,m_pos.y + size,m_pos.z + size };// 左上手前
	vertex[4] = { m_pos.x + size ,m_pos.y - size,m_pos.z - size };// 右下後ろ
	vertex[5] = { m_pos.x + size ,m_pos.y - size,m_pos.z + size };// 右下手前
	vertex[6] = { m_pos.x + size ,m_pos.y + size,m_pos.z - size };// 右上後ろ
	vertex[7] = { m_pos.x + size ,m_pos.y + size,m_pos.z + size };// 右上手前
}

void Dice::TestDraw()
{
	DirectX::XMFLOAT4 color = {1.0f,1.0f,0.0f,1.0f};
	Geometory::AddLine(vertex[0],vertex[1],color);// 1
	Geometory::AddLine(vertex[0],vertex[2],color);// 2
	Geometory::AddLine(vertex[0],vertex[4],color);// 3
	Geometory::AddLine(vertex[3],vertex[1],color);// 4
	Geometory::AddLine(vertex[3],vertex[2],color);// 5
	Geometory::AddLine(vertex[3],vertex[7],color);// 6
	Geometory::AddLine(vertex[5],vertex[1],color);// 7
	Geometory::AddLine(vertex[5],vertex[4],color);// 8
	Geometory::AddLine(vertex[5],vertex[7],color);// 9
	Geometory::AddLine(vertex[6],vertex[2],color);// 10
	Geometory::AddLine(vertex[6],vertex[4],color);// 11
	Geometory::AddLine(vertex[6],vertex[7],color);// 12

	DirectX::XMFLOAT4X4 fWVP[3];
	fWVP[0] = world;
	fWVP[1] = m_pCamera->GetViewMatrix(true);
	fWVP[2] = m_pCamera->GetProjectionMatrix(true);


	ShaderList::SetWVP(fWVP); // SetWVP関数の引数にはXMFLOAT4X4型で要素数３の配列のアドレスを渡す 

	// Spriteへカメラの行列を設定 
	Sprite::SetView(m_pCamera->GetViewMatrix());
	Sprite::SetProjection(m_pCamera->GetProjectionMatrix());

	

	// モデルに使用する頂点シェーダー、ピクセルシェーダーを設定 
	m_pModel->SetVertexShader(ShaderList::GetVS(ShaderList::VS_WORLD));
	m_pModel->SetPixelShader(ShaderList::GetPS(ShaderList::PS_LAMBERT));

	if(false)
	// マテリアル別にメッシュを表示 
	{
		float ambient = 0.8f;
		for (unsigned int i = 0; i < m_pModel->GetMeshNum(); ++i) {
			// モデルのメッシュを取得 
			const Model::Mesh* mesh = m_pModel->GetMesh(i);
			// メッシュに割り当てられているマテリアルを取得 
			Model::Material material = *m_pModel->GetMaterial(mesh->materialID);
			material.ambient = { ambient,ambient,ambient,ambient };
			// シェーダーへマテリアルを設定 
			ShaderList::SetMaterial(material);
			// モデルの描画 
			m_pModel->Draw(i);
		}
	}
}
