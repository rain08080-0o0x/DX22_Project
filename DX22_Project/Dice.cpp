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
{
	float size = sqrtf(2);
	m_pos = { 0.0f,0.0f,0.0f };
	m_size = { size,size,size };
	m_mass = 1.0f;
	Vec3 pos = { m_pos.x,m_pos.y,m_pos.z };
	Vec3 sizeV = {m_size.x,m_size.y,m_size.z};

	for (int i = 0; i < MAX_DICE; i++)
	{
		body[i] = nullptr;
	}

	body[0] = new RigidBodyOBB({0.0f,5.0f,0.0f}, {1.0f,1.0f,1.0f}, 10.0f);
	body[1] = new RigidBodyOBB({0.0f,0.0f,0.0f}, {10.0f,1.0f,10.0f}, 0.0f);

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
	{
		// 2. 衝突判定 (SAT等で別途実装が必要。ここでは結果が得られたと仮定)
		// 例えば、boxAの頂点を計算し、boxBに含まれるかチェックするなど
		Vec3 vertsA[8];
		body[0]->GetWorldVertices(vertsA);

		// boxA の頂点から、boxB の上面（簡易）への接触点を作って解決する
		{
			const float planeY = body[1]->center.y + body[1]->extents.y;

			// Aの最下点を取る（面接地なら4頂点がここに集まる）
			float minY = vertsA[0].y;
			for (int i = 1; i < 8; ++i)
			{
				if (vertsA[i].y < minY) minY = vertsA[i].y;
			}

			// 多少の誤差許容
			const float contactEps = 0.001f;
			// 「最下層付近」とみなす高さ幅（ここを広げすぎると傾きやすくなる）
			const float minLayer = 0.01f;

			// B上面の矩形範囲（Bを軸平行の床として扱う簡易）
			const float minX = body[1]->center.x - body[1]->extents.x;
			const float maxX = body[1]->center.x + body[1]->extents.x;
			const float minZ = body[1]->center.z - body[1]->extents.z;
			const float maxZ = body[1]->center.z + body[1]->extents.z;

			// 反復（2回だけ）
			for (int iter = 0; iter < 2; ++iter)
			{
				for (int i = 0; i < 8; ++i)
				{
					// B上面の真上にある頂点だけ採用（外なら無視）
					if (vertsA[i].x < minX || vertsA[i].x > maxX) continue;
					if (vertsA[i].z < minZ || vertsA[i].z > maxZ) continue;

					// 最下層付近の頂点だけ接触点にする（面なら複数点になる）
					if (vertsA[i].y > (minY + minLayer)) continue;

					// めり込み（または接触）しているか
					const float depth = planeY - vertsA[i].y;
					if (depth <= -contactEps) continue;

					CollisionResolver::Manifold m;
					m.bodyA = body[1];                 // 支持側（床）
					m.bodyB = body[0];                 // 乗る側
					m.normal = Vec3(0, 1, 0);    // 上向き法線（簡易）
					m.depth = depth;

					m.contactPoint = vertsA[i];
					m.contactPoint.y = planeY;   // 接触点を面上へ

					CollisionResolver::ResolveCollision(m);
				}
			}
		}
	}

	// 3. 物理更新
	for(int i = 0;i < MAX_DICE;i++)
	{
		if (body[i] == nullptr)continue;

		body[i]->Update(1.0f / 60.0f);
	}
	TRAN_INS;
	DirectX::XMFLOAT3
		pos =
	{
		body[0]->center.x,
		body[0]->center.y,
		body[0]->center.z
	};

	tran.obj.A = pos;
	pos =
	{
		body[1]->center.x,
		body[1]->center.y,
		body[1]->center.z
	};
	tran.obj.B = pos;
	tran.obj.Avel = { body[0]->velocity.x,body[0]->velocity.y,body[0]->velocity.z };
	tran.obj.Bvel = { body[1]->velocity.x,body[1]->velocity.y,body[1]->velocity.z };
	tran.obj.AangVel = { body[0]->angularVel.x,body[0]->angularVel.y,body[0]->angularVel.z };
	tran.obj.BangVel = { body[1]->angularVel.x,body[1]->angularVel.y,body[1]->angularVel.z };
	if (IsKeyTrigger('Y'))body[0]->center = { 0.0,5.0f,0.0f };

	if (IsKeyTrigger('R'))
	{
		body[0]->angularVel.z += 3.14f;
	}
	// axis→クォータニオンに変換して tran.dice.rot に入れたいなら別途（後述）
}


// 描画処理 
void Dice::Draw()
{
	using namespace DirectX;


	DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f };
	for(int i = 0;i < MAX_DICE;i++)
	{
		if (body[i] != nullptr)
		{
			color = { 1.0f,1.0f,1.0f,1.0f };
			color.x = 1.0f - (((i + 1) % 2) == 0);
			color.y = 1.0f - (((i + 1) % 4) == 0);
			color.z = 1.0f - (((i + 1) % 8) == 0);
			DirectX::XMFLOAT3 vtxA[8];
			body[i]->GetWorldVertices(vtxA);
			Geometory::AddLine(vtxA[0], vtxA[1], color); // 
			Geometory::AddLine(vtxA[1], vtxA[3], color); // 
			Geometory::AddLine(vtxA[3], vtxA[2], color); // 
			Geometory::AddLine(vtxA[2], vtxA[0], color); // 
			Geometory::AddLine(vtxA[0 + 4], vtxA[1 + 4], color); // 
			Geometory::AddLine(vtxA[1 + 4], vtxA[3 + 4], color); // 
			Geometory::AddLine(vtxA[3 + 4], vtxA[2 + 4], color); // 
			Geometory::AddLine(vtxA[2 + 4], vtxA[0 + 4], color); // 
			Geometory::AddLine(vtxA[0], vtxA[4], color); // 
			Geometory::AddLine(vtxA[1], vtxA[5], color); // 
			Geometory::AddLine(vtxA[2], vtxA[6], color); // 
			Geometory::AddLine(vtxA[3], vtxA[7], color); // 
		}
	}
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

