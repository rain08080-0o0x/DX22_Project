#include "SceneGame.h"
#include"Geometory.h"
#include "ShaderList.h" 
#include"Defines.h"
#include"CameraDebug.h"
#include "Transfer.h"

SceneGame::SceneGame()
{
	m_pModel = new Model();

	//if (!m_pModel->Load("Assets/Model/Furina/furina.pmx", 0.1f,Model::None)) { // 倍率と反転は省
	m_pCamera = new CameraDebug();
// 略可
	if (!m_pModel->Load("Assets/Model/KayKit/Assets/fbx/green/platform_1x1x1_green.fbx", 1.f,Model::ZFlip)) { // 倍率と反転は省略可
		MessageBox(NULL, "Branch_01","Error", MB_OK); // エラーメッセージの表示
	}
	//--- モデルの描画
	RenderTarget* pRTV = GetDefaultRTV(); // デフォルトのRenderTargetViewを取得
	DepthStencil* pDSV = GetDefaultDSV(); // デフォルトのDepthStencilViewを取得
	SetRenderTargets(1, &pRTV, pDSV); // 第3引数がnullの場合、2D表示となる

	SetDepthTest(true);
	m_pPlayer = new Player();
	m_pPlayer->SetCamera(m_pCamera);
	m_pBlock = new Block();
	m_pBlock->SetPos(DirectX::XMFLOAT3(0.0f, 0.0f, -5.0f));

	TRAN_INS;

	tran.m_maxPower = 100.0f;

	DirectX::XMFLOAT3 pos = { 0.0f,0.0f,0.0f };
	m_pDice = new Dice();
	m_pDice->Init(pos,1.0f);
	m_diceCount = 3;
	m_dice = new Dice[m_diceCount];

	m_dice[0].Init({ 0.0f, 2.0f,  0.0f }, 1.0f);
	m_dice[1].Init({ 0.0f, 8.0f,  0.0f }, 1.0f);
	m_dice[2].Init({ 0.0f, 15.0f, 0.0f }, 1.0f);

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
	if (m_pDice)
	{
		delete m_pDice;
		m_pDice = nullptr;
	}
}

void SceneGame::Update()
{
	m_pCamera->Update();
	m_pPlayer->Update();
	m_pBlock->Update();
	m_pPlayer->SetCamera(m_pCamera);
	m_pCamera->SetLook(m_pPlayer->GetPos());
	m_pDice->Update(1.0f / 60.0f);
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

	for (int i = 0; i < m_diceCount; ++i)
	{
		m_dice[i].Update(1.0f / 60.0f);
	}

	// 2個以上ある前提
	for (int i = 0; i < m_diceCount; ++i)
	{
		for (int j = i + 1; j < m_diceCount; ++j)
		{
			Collision::Box a = m_dice[i].GetCollision();
			Collision::Box b = m_dice[j].GetCollision();

			Collision::Result r = Collision::Hit(a, b);
			if (!r.isHit) continue;

			// r.dir は「押し戻す方向（どの軸で当たったか）」のつもりで使う
			// （あなたの Hit 実装に合わせて dir が (±1,0,0) みたいに入ってる前提）
			const float ax = (a.size.x + b.size.x) * 0.5f;
			const float ay = (a.size.y + b.size.y) * 0.5f;
			const float az = (a.size.z + b.size.z) * 0.5f;

			const float dx = (a.center.x - b.center.x);
			const float dy = (a.center.y - b.center.y);
			const float dz = (a.center.z - b.center.z);

			// めり込み量（どの軸で押し戻すかは r.dir に従う）
			float push = 0.0f;
			DirectX::XMFLOAT3 sep(0, 0, 0);

			if (r.dir.x != 0.0f)
			{
				push = ax - fabsf(dx);
				sep.x = (dx >= 0.0f ? 1.0f : -1.0f) * (push * 0.5f);
			}
			else if (r.dir.y != 0.0f)
			{
				push = ay - fabsf(dy);
				sep.y = (dy >= 0.0f ? 1.0f : -1.0f) * (push * 0.5f);
			}
			else if (r.dir.z != 0.0f)
			{
				push = az - fabsf(dz);
				sep.z = (dz >= 0.0f ? 1.0f : -1.0f) * (push * 0.5f);
			}

			// ①位置を少し離す（半分ずつ押し戻し）
			m_dice[i].AddPos(sep);
			m_dice[j].AddPos(DirectX::XMFLOAT3(-sep.x, -sep.y, -sep.z));

			// ②速度を入れ替える（衝突した軸成分だけ）
			DirectX::XMFLOAT3 vi = m_dice[i].GetVel();
			DirectX::XMFLOAT3 vj = m_dice[j].GetVel();

			// 反発を少し入れたいなら係数（0.8とか）
			const float e = 0.8f;

			if (r.dir.x != 0.0f)
			{
				std::swap(vi.x, vj.x);
				vi.x *= e; vj.x *= e;
			}
			if (r.dir.y != 0.0f)
			{
				std::swap(vi.y, vj.y);
				vi.y *= e; vj.y *= e;
			}
			if (r.dir.z != 0.0f)
			{
				std::swap(vi.z, vj.z);
				vi.z *= e; vj.z *= e;
			}

			m_dice[i].SetVel(vi);
			m_dice[j].SetVel(vj);
		}
	}

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
	if (m_pDice)
		m_pDice->Draw();
	m_pDice->GetCollision();
	for (int i = 0; i < m_diceCount; ++i)
	{
		m_dice[i].Draw();
	}
}
