#include "SceneGame.h"
#include"Geometory.h"
#include "ShaderList.h" 
#include"Defines.h"
#include"CameraDebug.h"
#include "Transfer.h"

#include <cstdlib>
#include <ctime>
#include <cmath>

// 0.0 ～ 1.0
static float Rand01()
{
	return (float)std::rand() / (float)RAND_MAX;
}

// -1.0 ～ 1.0
static float RandN11()
{
	return Rand01() * 2.0f - 1.0f;
}

// ランダムな単位ベクトル（方向）
static DirectX::XMFLOAT3 RandomUnitVector()
{
	float x = RandN11();
	float y = RandN11();
	float z = RandN11();

	float len = std::sqrt(x * x + y * y + z * z);
	if (len < 0.0001f)
	{
		return { 0.0f, 1.0f, 0.0f };
	}

	return { x / len, y / len, z / len };
}

SceneGame::SceneGame()
{
	// 乱数初期化（1回だけ）
	std::srand((unsigned)time(nullptr));

	//--- モデルの描画
	RenderTarget* pRTV = GetDefaultRTV();
	DepthStencil* pDSV = GetDefaultDSV();
	SetRenderTargets(1, &pRTV, pDSV);
	SetDepthTest(true);

	m_pModel = new Model();

	//if (!m_pModel->Load("Assets/Model/Furina/furina.pmx", 0.1f,Model::None)) { // 倍率と反転は省
	m_pCamera = new CameraDebug();
// 略可
	if (!m_pModel->Load("Assets/Model/KayKit/Assets/fbx/green/platform_1x1x1_green.fbx", 1.f,Model::ZFlip)) { // 倍率と反転は省略可
		MessageBox(NULL, "Branch_01","Error", MB_OK); // エラーメッセージの表示
	}

	m_pPlayer = new Player();
	m_pPlayer->SetCamera(m_pCamera);
	m_pBlock = new Block();
	m_pBlock->SetPos(DirectX::XMFLOAT3(0.0f, 0.0f, -5.0f));

	TRAN_INS;

	tran.m_maxPower = 1.0f;
	DirectX::XMFLOAT3 pos = { 0.0f,0.0f,0.0f };
	m_diceCount = 7;
	m_dice = new Dice[m_diceCount];

	for(int i = 0;i < 10;i++)
	{
		tran.deme[i] = 0;
	}

	m_dice[0].Init({ 0.0f, 2.0f,  0.0f }, 1.0f);
	m_dice[1].Init({ 1.0f, 2.0f,  0.0f }, 1.0f);
	m_dice[2].Init({ -1.0f, 2.0f, 0.0f }, 1.0f);
	m_dice[3].Init({  2.0f, 2.0f, 0.0f }, 1.0f);
	m_dice[4].Init({ -2.0f, 2.0f, 0.0f }, 1.0f);
	m_dice[5].Init({  3.0f, 2.0f, 0.0f }, 1.0f);
	m_dice[6].Init({ -3.0f, 2.0f, 0.0f }, 1.0f);
	for (int i = 0; i < m_diceCount; ++i)
	{
		m_dice[i].SetCamera(m_pCamera);
	}
	//RollAll();

}

SceneGame::~SceneGame()
{
	if (m_pModel) {
		delete m_pModel;
		m_pModel = nullptr;
	}
	if (m_pCamera) {
		delete m_pCamera;
		m_pCamera = nullptr;
	}
	if (m_pPlayer) {
		delete m_pPlayer;
		m_pPlayer = nullptr;
	}
	if (m_pBlock) {
		delete m_pBlock;
		m_pBlock = nullptr;
	}
}

void SceneGame::Update()
{
	TRAN_INS;
	m_pCamera->Update();
	m_pPlayer->Update();
	m_pBlock->Update();
	m_pPlayer->SetCamera(m_pCamera);
	m_pCamera->SetLook(m_pPlayer->GetPos());
	Collision::Box a = m_pPlayer->GetCollision();
	Collision::Box b = m_pBlock->GetCollision();

	Collision::Result result;
	result = Collision::Hit(a, b);

	if (result.isHit)
	{
		if (result.dir.x != 0.0f)m_pPlayer->Bound(Player::BoundX);
		if (result.dir.y != 0.0f)m_pPlayer->Bound(Player::BoundY);
		if (result.dir.z != 0.0f)m_pPlayer->Bound(Player::BoundZ);
	}

	static bool isPressed[3];
	if (IsKeyTrigger('R') || IsKeyRelease('R'))
	{
		for (int i = 0; i < m_diceCount; i++)
		{
			m_dice[i].ResetIsSleeping();
		}

		MoveAllDice();
		for(int i = 0;i < 3;i++)
			isPressed[i] = true;
	}


	for (int i = 0; i < m_diceCount; i++)
	{
		m_dice[i].Update(1.0f / 60.0f);

		if (m_dice[i].IsSleeping())
		{
			int face = m_dice[i].GetTopFace();
			// スコア計算、UI表示、ログなど
			if (isPressed[i] && false)
			{
				switch (face)
				{
				case 1:
					m_dice[i].SetFaceUp(1);
					break;
				case 2:
					m_dice[i].SetFaceUp(6);
					break;
				case 3:
					m_dice[i].SetFaceUp(4);
					break;
				case 4:
					m_dice[i].SetFaceUp(3);
					break;
				case 5:
					m_dice[i].SetFaceUp(5);
					break;
				case 6:
					m_dice[i].SetFaceUp(2);
					break;
				}
				tran.deme[i] = face;
				isPressed[i] = false;
			}
		}
	}
	// ★ サイコロ同士の衝突
	ResolveDiceCollisions();

}

void SceneGame::Draw()
{

	// 頂点シェーダーに渡す変換行列の変数を宣言 
	DirectX::XMFLOAT4X4 fWVP[3];    // World,View,Projectionの略  
	DirectX::XMMATRIX world, view, proj; // 各変換行列の格納先 

	// 作成した行列を各変数へ格納 
	world = DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	view = DirectX::XMMatrixLookAtLH(
		DirectX::XMVectorSet(0.0f, 1.5f, -2.0f, 0.0f),
		DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), 
		DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	proj = 
	DirectX::XMMatrixOrthographicOffCenterLH(
		-640,640,	// 横下限上限値
		-360,360,	// 縦下限上限値
		0.001f,		// Near
		1000.0f);	// Far
	proj = DirectX::XMMatrixPerspectiveFovLH(
		// DirectXMathに用意されている角度をラジアン角に変換する関数
		DirectX::XMConvertToRadians(70.0f),	//角度
		16.0f / 9.0f,						//アス比
		0.1f,								//最小描画距離
		100.0f);							//最長描画距離



	// 計算用のデータから読み取り用のデータに変換 
	DirectX::XMStoreFloat4x4(&fWVP[0], DirectX::XMMatrixTranspose(world));
	DirectX::XMStoreFloat4x4(&fWVP[1], DirectX::XMMatrixTranspose(view));
	DirectX::XMStoreFloat4x4(&fWVP[2], DirectX::XMMatrixTranspose(proj));

	// モデルに変換行列を設定 
	fWVP[1] = m_pCamera->GetViewMatrix();
	fWVP[2] = m_pCamera->GetProjectionMatrix();

	// シェーダーへ変換行列を設定 
	ShaderList::SetWVP(fWVP); // SetWVP関数の引数にはXMFLOAT4X4型で要素数３の配列のアドレスを渡す 

	// モデルに使用する頂点シェーダー、ピクセルシェーダーを設定 
	m_pModel->SetVertexShader(ShaderList::GetVS(ShaderList::VS_WORLD));
	m_pModel->SetPixelShader(ShaderList::GetPS(ShaderList::PS_LAMBERT));

	// 仮置きしているボックスにカメラを設定
	//Geometory::SetView(fWVP[1]);
	//Geometory::SetProjection(fWVP[2]);

	//// 仮置きしているボックスにカメラを設定 
	//Geometory::SetView(m_pCamera->GetViewMatrix());
	//Geometory::SetProjection(m_pCamera->GetProjectionMatrix());

	if(false)
	// マテリアル別にメッシュを表示 
	for (unsigned int i = 0; i < m_pModel->GetMeshNum(); ++i) {
		// モデルのメッシュを取得 
		const Model::Mesh* mesh = m_pModel->GetMesh(i);
		// メッシュに割り当てられているマテリアルを取得 
		Model::Material material = *m_pModel->GetMaterial(mesh->materialID);
		// シェーダーへマテリアルを設定 
		ShaderList::SetMaterial(material);
		// モデルの描画 
		m_pModel->Draw(i);
	}

	Geometory::SetView(fWVP[1]);
	Geometory::SetProjection(fWVP[2]);

	DirectX::XMMATRIX T;
	DirectX::XMMATRIX S;
	DirectX::XMMATRIX mat;
	DirectX::XMFLOAT4X4 fMat;

	static float rad;

	//--- 地面
	T = DirectX::XMMatrixTranslation(cosf(rad) * tanf(rad), 0.0f, sinf(rad) * tanf(rad));
	S = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
	mat = S * T;
	mat = DirectX::XMMatrixTranspose(mat);
	fMat; // 行列の格納先
	DirectX::XMStoreFloat4x4(&fMat, mat);
	Geometory::SetWorld(fMat); // ボックスに変換行列を設定
	//Geometory::DrawCylinder();

	if(m_pPlayer)
		m_pPlayer->Draw();
	if(m_pBlock)
		m_pBlock->Draw();
	for (int i = 0; i < m_diceCount; ++i)
	{
		m_dice[i].Draw();
	}
}

void SceneGame::MoveAllDice()
{
	// ===== 調整用パラメータ =====
	const float MOVE_SPEED_MIN = 4.0f;
	const float MOVE_SPEED_MAX = 8.0f;

	const float SPIN_SPEED_MIN = 5.0f;   // rad/s
	const float SPIN_SPEED_MAX = 15.0f;

	for (int i = 0; i < m_diceCount; ++i)
	{
		// ---------- 1) 移動方向 ----------
		// 水平方向をメインにしたいので Y は少しだけ
		DirectX::XMFLOAT3 dir = RandomUnitVector();
		dir.y = 0.3f + 0.7f * Rand01();

		// 再正規化
		float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (len < 0.0001f) len = 1.0f;
		dir.x /= len;
		dir.y /= len;
		dir.z /= len;

		// ---------- 2) 移動速度 ----------
		float speed = MOVE_SPEED_MIN +
			(MOVE_SPEED_MAX - MOVE_SPEED_MIN) * Rand01();

		m_dice[i].SetVel({
			dir.x * speed,
			dir.y * speed,
			dir.z * speed
			});

		// ---------- 3) 回転（角速度） ----------
		DirectX::XMFLOAT3 spinAxis = RandomUnitVector();
		float spinSpeed = SPIN_SPEED_MIN +
			(SPIN_SPEED_MAX - SPIN_SPEED_MIN) * Rand01();

		m_dice[i].SetAngVel({
			spinAxis.x * spinSpeed,
			spinAxis.y * spinSpeed,
			spinAxis.z * spinSpeed
			});

		// ---------- 4) 初期姿勢をランダムに ----------
		float angle = Rand01() * DirectX::XM_2PI;
		DirectX::XMVECTOR axis =
			DirectX::XMVectorSet(spinAxis.x, spinAxis.y, spinAxis.z, 0.0f);

		DirectX::XMVECTOR q =
			DirectX::XMQuaternionRotationAxis(axis, angle);

		DirectX::XMFLOAT4 qf;
		DirectX::XMStoreFloat4(&qf,
			DirectX::XMQuaternionNormalize(q));

		m_dice[i].SetRotation(qf); 

		m_dice[i].WakeUp();
	}

}

void SceneGame::ResolveDiceCollisions()
{
	for (int i = 0; i < m_diceCount; ++i)
	{
		for (int j = i + 1; j < m_diceCount; ++j)
		{
			ResolveDicePair(m_dice[i], m_dice[j]);
		}
	}
	for (int i = 0; i < m_diceCount; ++i)
	{
		for (int j = i + 1; j < m_diceCount; ++j)
		{
			Collision::OBB a = m_dice[i].GetOBB();
			Collision::OBB b = m_dice[j].GetOBB();
			Collision::Manifold m;
			if (Collision::HitOBB(a, b))
			{
				// とりあえず確認用
				printf("Dice %d and %d hit!\n", i, j);
			}
			if (Collision::HitOBB_Full(a, b, m))
			{
				// ① 位置を引き離す
				ResolveDicePosition(m_dice[i], m_dice[j], m);
				// ② 速度を反射
				ResolveDiceVelocity(m_dice[i], m_dice[j], m);
				// ③ 角度を反射
				ResolveDiceAngular(m_dice[i], m_dice[j], m);
			}
		}
	}

}void SceneGame::ResolveDicePosition(Dice& a, Dice& b, const Collision::Manifold& m)
{
	// 数値誤差対策（ほんの少し）
	const float SLOP = 0.001f;

	float depth = m.penetration + SLOP;

	// 押し戻し量（半分ずつ）
	DirectX::XMFLOAT3 correction =
	{
		m.normal.x * depth * 0.5f,
		m.normal.y * depth * 0.5f,
		m.normal.z * depth * 0.5f
	};

	// A を -normal 側へ
	a.AddPos({
		-correction.x,
		-correction.y,
		-correction.z
		});

	// B を +normal 側へ
	b.AddPos(correction);
}


void SceneGame::ResolveDicePair(Dice& a, Dice& b)
{
	Collision::Box boxA = a.GetCollision();
	Collision::Box boxB = b.GetCollision();

	Collision::Result r = Collision::Hit(boxA, boxB);
	if (!r.isHit)
		return;

	// ---- 中心差 ----
	DirectX::XMFLOAT3 d =
	{
		boxA.center.x - boxB.center.x,
		boxA.center.y - boxB.center.y,
		boxA.center.z - boxB.center.z
	};

	// ---- 半サイズ合計 ----
	const float hx = (boxA.size.x + boxB.size.x) * 0.5f;
	const float hy = (boxA.size.y + boxB.size.y) * 0.5f;
	const float hz = (boxA.size.z + boxB.size.z) * 0.5f;

	// ---- めり込み量 ----
	const float px = hx - fabsf(d.x);
	const float py = hy - fabsf(d.y);
	const float pz = hz - fabsf(d.z);

	// ---- 最小の軸で処理 ----
	DirectX::XMFLOAT3 sep = { 0,0,0 };

	if (px <= py && px <= pz)
	{
		sep.x = (d.x >= 0.0f ? px : -px);
	}
	else if (py <= px && py <= pz)
	{
		sep.y = (d.y >= 0.0f ? py : -py);
	}
	else
	{
		sep.z = (d.z >= 0.0f ? pz : -pz);
	}

	// ---- 位置補正（半分ずつ押し戻す）----
	a.AddPos({ sep.x * 0.5f,  sep.y * 0.5f,  sep.z * 0.5f });
	b.AddPos({ -sep.x * 0.5f, -sep.y * 0.5f, -sep.z * 0.5f });

	// ---- 速度交換（衝突軸のみ）----
	DirectX::XMFLOAT3 va = a.GetVel();
	DirectX::XMFLOAT3 vb = b.GetVel();

	const float restitution = 0.8f;

	if (sep.x != 0.0f)
	{
		std::swap(va.x, vb.x);
		va.x *= restitution;
		vb.x *= restitution;
	}
	if (sep.y != 0.0f)
	{
		std::swap(va.y, vb.y);
		va.y *= restitution;
		vb.y *= restitution;
	}
	if (sep.z != 0.0f)
	{
		std::swap(va.z, vb.z);
		va.z *= restitution;
		vb.z *= restitution;
	}

	a.SetVel(va);
	b.SetVel(vb);
}

void SceneGame::ResolveDiceVelocity(Dice& a, Dice& b, const Collision::Manifold& m)
{
	using namespace DirectX;

	const float restitution = 0.6f; // 反発係数（調整用）

	// A の速度
	XMFLOAT3 va = a.GetVel();
	XMFLOAT3 vb = b.GetVel();

	XMVECTOR n = XMLoadFloat3(&m.normal); // A → B

	// A 側（法線の逆向きに当たる）
	{
		XMVECTOR v = XMLoadFloat3(&va);
		float vn = XMVectorGetX(XMVector3Dot(v, n));

		if (vn > 0.0f) // A が B に向かって動いている時だけ
		{
			XMVECTOR vr =
				XMVectorSubtract(
					v,
					XMVectorScale(n, (1.0f + restitution) * vn)
				);

			XMStoreFloat3(&va, vr);
		}
	}

	// B 側（法線向きに当たる）
	{
		XMVECTOR v = XMLoadFloat3(&vb);
		float vn = XMVectorGetX(XMVector3Dot(v, n));

		if (vn < 0.0f) // B が A に向かって動いている時だけ
		{
			XMVECTOR vr =
				XMVectorSubtract(
					v,
					XMVectorScale(n, (1.0f + restitution) * vn)
				);

			XMStoreFloat3(&vb, vr);
		}
	}

	a.SetVel(va);
	b.SetVel(vb);
}

void SceneGame::ResolveDiceAngular(Dice& a, Dice& b, const Collision::Manifold& m)
{
	using namespace DirectX;

	// 調整用（回りやすさ）
	const float ANGULAR_IMPULSE = 0.3f;

	XMVECTOR n = XMLoadFloat3(&m.normal); // A → B

	// ---- A 側 ----
	{
		XMFLOAT3 va = a.GetVel();
		XMVECTOR v = XMLoadFloat3(&va);

		// 回転方向 = v × n
		XMVECTOR spin = XMVector3Cross(v, n);

		spin = XMVectorScale(spin, ANGULAR_IMPULSE);

		XMFLOAT3 dw;
		XMStoreFloat3(&dw, spin);

		a.AddAngVel(dw);
	}

	// ---- B 側（逆向き）----
	{
		XMFLOAT3 vb = b.GetVel();
		XMVECTOR v = XMLoadFloat3(&vb);

		XMVECTOR spin = XMVector3Cross(v, n);

		spin = XMVectorScale(spin, -ANGULAR_IMPULSE);

		XMFLOAT3 dw;
		XMStoreFloat3(&dw, spin);

		b.AddAngVel(dw);
	}
}
