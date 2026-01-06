#include "Dice.h"
#include "Geometory.h"
#include "Transfer.h"
#include "Input.h"

#include "ShaderList.h"
#include "Sprite.h"
#include "Shader.h"
#include "Defines.h"
#include<math.h>
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
	: m_pCamera(nullptr)
	, body(Vec3::Vector3Zero(),Vec3::Vector3Zero(),0.0f)
{
	float size = sqrtf(2);
	m_pos = { 0.0f,0.0f,0.0f };
	m_size = { size,size,size };
	m_mass = 1.0f;
	Vec3 pos = { m_pos.x,m_pos.y,m_pos.z };
	Vec3 sizeV = {m_size.x,m_size.y,m_size.z};
	body = RigidBodyOBB(pos, sizeV, m_mass);
	// 頂点決め
	float sizehalf = HALF(0.5f);
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
	if (m_pModel)
	{
		m_pModel->Reset();
		m_pModel = nullptr;
	}
}

// 更新処理 
void Dice::Update(float dt)
{
	body.Update(dt);

	// Transferへの反映が欲しいなら、ここで“参照だけ”して書く（計算しない）
	TRAN_INS;
	tran.dice.pos = { body.center.x,body.center.y,body.center.z };
	tran.dice.velocity = { body.velocity.x,body.velocity.y,body.velocity.z };

	// axis→クォータニオンに変換して tran.dice.rot に入れたいなら別途（後述）
}


// 描画処理 
void Dice::Draw()
{
	using namespace DirectX;

	// axisはワールド基底ベクトル（列ベクトルとして使う）
	XMMATRIX R = XMMATRIX(
		body.axis[0].x, body.axis[0].y, body.axis[0].z, 0.0f,
		body.axis[1].x, body.axis[1].y, body.axis[1].z, 0.0f,
		body.axis[2].x, body.axis[2].y, body.axis[2].z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	XMMATRIX T = XMMatrixTranslation(body.center.x, body.center.y, body.center.z);

	// extentsは半サイズなので *2 で全サイズ
	XMMATRIX S = XMMatrixScaling(body.extents.x * 2.0f, body.extents.y * 2.0f, body.extents.z * 2.0f);

	XMMATRIX W = S * R * T;

	XMFLOAT4X4 xWorld;
	XMStoreFloat4x4(&xWorld, XMMatrixTranspose(W));
	Geometory::SetWorld(xWorld);
	Geometory::DrawBox();
}


// カメラの設定 
void Dice::SetCamera(Camera* pCamera)
{
	m_pCamera = pCamera;
}



void Dice::TestUpdate()
{
	TRAN_INS;
	//--- 行列配列のそれぞれのやつ

	float size = 0.5f;

	vertex[0] = { m_pos.x - size ,m_pos.y - size,m_pos.z - size };// 左下後ろ
	vertex[1] = { m_pos.x - size ,m_pos.y - size,m_pos.z + size };// 左下手前
	vertex[2] = { m_pos.x - size ,m_pos.y + size,m_pos.z - size };// 左上後ろ
	vertex[3] = { m_pos.x - size ,m_pos.y + size,m_pos.z + size };// 左上手前
	vertex[4] = { m_pos.x + size ,m_pos.y - size,m_pos.z - size };// 右下後ろ
	vertex[5] = { m_pos.x + size ,m_pos.y - size,m_pos.z + size };// 右下手前
	vertex[6] = { m_pos.x + size ,m_pos.y + size,m_pos.z - size };// 右上後ろ
	vertex[7] = { m_pos.x + size ,m_pos.y + size,m_pos.z + size };// 右上手前

	if (true)// 底面の角度移動
	{	// 0,1,4,5
		using namespace DirectX;
		float rad = 3.1415f;
		float radius = rad;
						// 左後		  左手前	右後ろ	  右手前
		XMFLOAT3 vtx[4] = {vertex[0],vertex[1],vertex[4],vertex[5]};
		XMFLOAT3 vtx2[4] = {vertex[2],vertex[3],vertex[6],vertex[7]};

		float posX = cosf(rad);
		float posY = sinf(rad);

		posX = m_pos.x + cosf(radius); posY = m_pos.z + sinf(radius);

		float height = sqrtf(2) / 2.0f;
		vtx[0] = { posX,m_pos.y - height,posY }; radius += (3.1415f / 2.0f);posX = m_pos.x + cosf(radius); posY = m_pos.z + sinf(radius);
		vtx[1] = { posX,m_pos.y - height,posY }; radius += (3.1415f / 2.0f);posX = m_pos.x + cosf(radius); posY = m_pos.z + sinf(radius);
		vtx[2] = { posX,m_pos.y - height,posY }; radius += (3.1415f / 2.0f);posX = m_pos.x + cosf(radius); posY = m_pos.z + sinf(radius);
		vtx[3] = { posX,m_pos.y - height,posY }; radius += (3.1415f / 2.0f);posX = m_pos.x + cosf(radius); posY = m_pos.z + sinf(radius);
		vertex[0] = vtx[0];
		vertex[1] = vtx[1];
		vertex[4] = vtx[2];
		vertex[5] = vtx[3];

		vtx[0] = { posX,m_pos.y + height,posY }; radius += (3.1415f / 2.0f);posX = m_pos.x + cosf(radius); posY = m_pos.z + sinf(radius);
		vtx[1] = { posX,m_pos.y + height,posY }; radius += (3.1415f / 2.0f);posX = m_pos.x + cosf(radius); posY = m_pos.z + sinf(radius);
		vtx[2] = { posX,m_pos.y + height,posY }; radius += (3.1415f / 2.0f);posX = m_pos.x + cosf(radius); posY = m_pos.z + sinf(radius);
		vtx[3] = { posX,m_pos.y + height,posY }; radius += (3.1415f / 2.0f);posX = m_pos.x + cosf(radius); posY = m_pos.z + sinf(radius);
		vertex[2] = vtx[0];
		vertex[3] = vtx[1];
		vertex[6] = vtx[2];
		vertex[7] = vtx[3];
	}
}

void Dice::TestDraw()
{
	TRAN_INS;
	DirectX::XMFLOAT4 color = tran.dice.color;// 0,1,4,5
	if (true)
	{	// 底面
		Geometory::AddLine(vertex[0], vertex[1], color);
		Geometory::AddLine(vertex[1], vertex[4], color);
		Geometory::AddLine(vertex[4], vertex[5], color);
		Geometory::AddLine(vertex[5], vertex[0], color);

		//天面 2,3,6,7
		Geometory::AddLine(vertex[2], vertex[3], color);
		Geometory::AddLine(vertex[3], vertex[6], color);
		Geometory::AddLine(vertex[6], vertex[7], color);
		Geometory::AddLine(vertex[7], vertex[2], color);
		// 側面
		Geometory::AddLine(vertex[2], vertex[0], color);
		Geometory::AddLine(vertex[3], vertex[1], color);
		Geometory::AddLine(vertex[6], vertex[4], color);
		Geometory::AddLine(vertex[7], vertex[5], color);
		DirectX::XMFLOAT3 pos = { 
			m_pos.x + tran.dice.virtualVelocity.x,
			m_pos.y + tran.dice.virtualVelocity.y,
			m_pos.z + tran.dice.virtualVelocity.z };
		Geometory::AddLine(m_pos, pos, color);
	}
	if(false)
	{
		Geometory::AddLine(vertex[0], vertex[1], color);// 1
		Geometory::AddLine(vertex[0], vertex[2], color);// 2
		Geometory::AddLine(vertex[0], vertex[4], color);// 3
		Geometory::AddLine(vertex[3], vertex[1], color);// 4
		Geometory::AddLine(vertex[3], vertex[2], color);// 5
		Geometory::AddLine(vertex[3], vertex[7], color);// 6
		Geometory::AddLine(vertex[5], vertex[1], color);// 7
		Geometory::AddLine(vertex[5], vertex[4], color);// 8
		Geometory::AddLine(vertex[5], vertex[7], color);// 9
		Geometory::AddLine(vertex[6], vertex[2], color);// 10
		Geometory::AddLine(vertex[6], vertex[4], color);// 11
		Geometory::AddLine(vertex[6], vertex[7], color);// 12
	}

	DirectX::XMFLOAT4X4 world; DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));
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

static float ClampF(float v, float a, float b) { return (v < a) ? a : (v > b) ? b : v; }

static float Len3(const DirectX::XMFLOAT3& v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}
static void ClampVec3(DirectX::XMFLOAT3& v, float maxLen)
{
	float l = Len3(v);
	if (l > maxLen && l > 1e-6f)
	{
		float s = maxLen / l;
		v.x *= s; v.y *= s; v.z *= s;
	}
}
void Dice::RollStable(float strength)
{
	body.WakeUp();

	// 接地中の安全対策（これは物理に入れても良いが、命令側でも最小ならOK）
	if (body.isGround)
	{
		body.center.y += 0.01f;
		if (body.velocity.y < 0.2f) body.velocity.y = 0.2f;
	}

	// 方向の決定（これは最小の計算。乱数を使う以上避けられない）
	float rx = ((float)rand() / 0x7fff) * 2.0f - 1.0f;
	float rz = ((float)rand() / 0x7fff) * 2.0f - 1.0f;
	float rl = sqrtf(rx * rx + rz * rz);
	if (rl < 1e-4f) { rx = 1.0f; rz = 0.0f; rl = 1.0f; }
	rx /= rl; rz /= rl;

	// “加算するだけ”
	float vKick = (2.0f + ((float)rand() / 0x7fff) * 2.0f) * strength;
	body.velocity.x += rx * vKick;
	body.velocity.z += rz * vKick;
	body.velocity.y += 0.3f * strength;

	float wKick = (8.0f + ((float)rand() / 0x7fff) * 8.0f) * strength;
	body.angularVel.x += (-rz) * wKick;
	body.angularVel.z += (rx)*wKick;
	body.angularVel.y += (((float)rand() / 0x7fff) * 2.0f - 1.0f) * (1.0f * strength);

	// 暴走防止は body 側に関数を用意して呼ぶのが綺麗
	body.ClampLinear(8.0f);
	body.ClampAngular(25.0f);
}

