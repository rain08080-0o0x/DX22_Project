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
	m_diceCount = 3;
	m_dice = new Dice[m_diceCount];

	m_dice[0].Init({ 0.0f, 2.0f,  0.0f }, 1.0f);
	m_dice[1].Init({ 1.0f, 2.0f,  0.0f }, 1.0f);
	m_dice[2].Init({ -1.0f, 2.0f, 0.0f }, 1.0f);
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

	for (int i = 0; i < 3; i++)
	{
		m_dice[i].Update(1.0f / 60.0f);
	}

	if (IsKeyTrigger('R') || IsKeyRelease('R'))MoveAllDice();
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
	}
}