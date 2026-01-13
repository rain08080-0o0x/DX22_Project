/*****************************************************************//**
 * \file   SceneGame.cpp
 * \brief  ゲームシーン
 * 
 * \author 山本郁也
 * \date   January 2026
 *********************************************************************/
#include "SceneGame.h"
#include"Geometory.h"
#include "ShaderList.h" 
#include"Defines.h"
#include"CameraDebug.h"
#include "Transfer.h"
#include "SceneManager.h"


static void Sort3(int& a, int& b, int& c)
{
	if (a > b) std::swap(a, b);
	if (b > c) std::swap(b, c);
	if (a > b) std::swap(a, b);
}

static RoleResult CalcRole(int d0, int d1, int d2)
{
	Sort3(d0, d1, d2);

	// ピンゾロ
	if (d0 == 1 && d1 == 1 && d2 == 1)
		return { RoleType::Pinzoro, 100, 0 };

	// ゾロ目
	if (d0 == d1 && d1 == d2)
		return { RoleType::Zorome, 50 + d0 * 10, 0 };

	// シゴロ
	if (d0 == 4 && d1 == 5 && d2 == 6)
		return { RoleType::Shigoro, 30, 0 };

	// ヒフミ
	if (d0 == 1 && d1 == 2 && d2 == 3)
		return { RoleType::Hifumi, -20, 0 };

	// 通常役（2個同じ + 残り1個）
	if (d0 == d1 && d1 != d2)
		return { RoleType::Me, d2, d2 };

	if (d0 != d1 && d1 == d2)
		return { RoleType::Me, d0, d0 };

	// 役なし
	return { RoleType::None, 0, 0 };
}


const float panelW = 375.0f;
const float panelH = 520.0f;

// 表示位置（開いてるときは右に寄せて少し余白）
const float openX = SCREEN_WIDTH - panelW * 0.5f - 20.0f;
// 閉じてるときは画面外（右に逃がす）
const float closeX = SCREEN_WIDTH + panelW * 0.5f + 20.0f;

SceneGame::SceneGame()
	:OnlyDice(true)
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
	m_pGaugeUI = new GaugeUI();

	// 一つだけ
	m_pDice = new Dice();
	m_pDice->SetCamera(m_pCamera);
	TRAN_INS;



	m_pScore = new ScoreLite("Number/number.png", 360.0f, 40.0f, 48.0f, 64.0f, 56.0f);
	m_pScore->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_scoredThisRoll = false;

	m_pRoleUI = new UIObject(
		"Role/Role_None.png",   // 仮（あとで差し替える）
		360.0f, 120.0f,
		256.0f, 96.0f
	);

	m_pRoleUI->SetColor(1, 1, 1, 1);
	m_roleFixedThisRoll = false;

	// 例：役一覧UI生成済みとして
	//const float panelW = 375.0f;
	//const float panelH = 520.0f;

	//// 表示位置（開いてるときは右に寄せて少し余白）
	//const float openX = SCREEN_WIDTH - panelW * 0.5f - 20.0f;
	//// 閉じてるときは画面外（右に逃がす）
	//const float closeX = SCREEN_WIDTH + panelW * 0.5f + 20.0f;

	// 高さは好み。上寄せなら 140〜200 くらいが見やすい
	m_roleListY = SCREEN_HEIGHT - panelH * 0.5f;

	m_roleListOpen = false;
	m_roleListX = closeX;
	m_roleListTargetX = closeX;

	// 速度（1秒でほぼ到達するくらい）
	m_roleListSpeed = 14.0f;

	tran.diceui.role.pos = { m_roleListX ,m_roleListY};
	tran.diceui.role.size = {panelW,panelH};

	// 生成
	m_role = new UIObject("tintiro.png",
		tran.diceui.role.pos.x,
		tran.diceui.role.pos.y,
		tran.diceui.role.size.x,
		tran.diceui.role.size.y);
	// 初期位置を反映
	m_role->SetPosition(m_roleListX, m_roleListY);
	m_role->SetSize(panelW, panelH);


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
		m_pBlock= nullptr;
	}
	if (m_pDice)
	{
		delete m_pDice;
		m_pDice = nullptr;
	}
	if (m_pGaugeUI)
	{
		delete m_pGaugeUI;
		m_pGaugeUI = nullptr;
	}
	if (m_role)
	{
		delete m_role;
		m_role = nullptr;
	}
	if(m_pScore)
	{
		delete m_pScore;
		m_pScore = nullptr;
	}
	if (m_pRoleUI)
	{
		delete m_pRoleUI;
		m_pRoleUI = nullptr;
	}
}

void SceneGame::Update()
{
	m_pCamera->Update();
	if(!OnlyDice)
	{
		m_pBlock->Update();
		m_pPlayer->SetCamera(m_pCamera);
		m_pPlayer->Update();
		m_pCamera->SetLook(m_pPlayer->GetPos());
		m_pDice->Update();

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

		DirectX::XMFLOAT3 shadowPos = m_pPlayer->GetPos();
		Collision::Box s = m_pPlayer->GetShadowCollision();
		result = Collision::Hit(b, s);
		if (result.isHit)
			shadowPos.y = b.center.y + b.size.y * 0.5f;
		else
			shadowPos.y = 0.0f;
		m_pPlayer->SetShadowPos(shadowPos);

		m_pGaugeUI->SetGauge(m_pPlayer->GetPower());
		m_pGaugeUI->Update();
	}
	else
	{
		m_pDice->Update(1);
		m_pDice->SetCamera(m_pCamera);

		if (m_pDice->IsStop())
		{
			m_pCamera->LockPos(true);

			TRAN_INS;
			const int a = tran.dice.currentFaceNumber[0];
			const int b = tran.dice.currentFaceNumber[2];
			const int c = tran.dice.currentFaceNumber[3];
			if (!m_roleFixedThisRoll)
			{

				if (a >= 1 && a <= 6 && b >= 1 && b <= 6 && c >= 1 && c <= 6)
				{
					RoleResult r = CalcRole(a, b, c);

					// スコア加算
					m_pScore->AddScore(r.addScore);

					// 役名表示
					switch (r.role)
					{
					case RoleType::None:
						m_pRoleUI->SetTexture("Role/Role_None.png");
						break;
					case RoleType::Hifumi:
						m_pRoleUI->SetTexture("Role/Role_Hifumi.png");
						break;
					case RoleType::Shigoro:
						m_pRoleUI->SetTexture("Role/Role_Shigoro.png");
						break;
					case RoleType::Zorome:
						m_pRoleUI->SetTexture("Role/Role_Zorome.png");
						break;
					case RoleType::Pinzoro:
						m_pRoleUI->SetTexture("Role/Role_Pinzoro.png");
						break;
					case RoleType::Me:
					{
						char path[64];
						sprintf_s(path, "Role/Role_Me%d.png", r.me);
						m_pRoleUI->SetTexture(path);
						break;
					}
					default:
						// 役なしなら表示消す or --- にする
						break;
					}

					m_roleFixedThisRoll = true;
				}
			}
		}
		else
		{
			m_pCamera->LockPos(false);
		}
		if (IsKeyTrigger('R'))
		{
			if (m_pDice)
			{
				m_roleFixedThisRoll = false;
				m_scoredThisRoll = false;
				m_pDice->RollRandom(0);
				m_pDice->RollRandom(2);
				m_pDice->RollRandom(3);
			}
		}
		// Shiftを押すたびに開閉
		if (IsKeyTrigger(VK_SHIFT))
		{
			m_roleListOpen = !m_roleListOpen;

			//const float panelW = 520.0f;
			//const float openX  = 1280.0f - panelW * 0.5f - 20.0f;
			//const float closeX = 1280.0f + panelW * 0.5f + 20.0f;

			m_roleListTargetX = m_roleListOpen ? openX : closeX;
		}

		// dt（あなたの環境に合わせて）
		const float dt = 1.0f / 120.0f;

		// Lerpで滑らかに追従（指数追従）
		{
			float t = 1.0f - expf(-m_roleListSpeed * dt);
			m_roleListX = m_roleListX + (m_roleListTargetX - m_roleListX) * t;

			m_role->SetPosition(m_roleListX, m_roleListY);
		}

		if (IsKeyTrigger(VK_ESCAPE))
		{
			SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
		}
	}
}

void SceneGame::Draw()
{
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
				-640, 640,	// 横下限上限値
				-360, 360,	// 縦下限上限値
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
		Geometory::SetView(fWVP[1]);
		Geometory::SetProjection(fWVP[2]);

		// 仮置きしているボックスにカメラを設定 
		Geometory::SetView(m_pCamera->GetViewMatrix());
		Geometory::SetProjection(m_pCamera->GetProjectionMatrix());

		// Spriteへカメラの行列を設定 
		Sprite::SetView(m_pCamera->GetViewMatrix());
		Sprite::SetProjection(m_pCamera->GetProjectionMatrix());
	}
	// モデルの描画 基本それぞれのDrawで出力させるのでいらないがサンプルとして残す
	if(false)
	{
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
	}

	if(!OnlyDice)
	{
		if (m_pBlock)
			m_pBlock->Draw();
		if (m_pPlayer)
			m_pPlayer->Draw();
		SetDepthTest(false);
		if (m_pGaugeUI)
			m_pGaugeUI->Draw();
		SetDepthTest(true);
	}
	else
	{
		// 参照用のインスタンスを取得
		TRAN_INS;

		if (m_pDice)
		{
			m_pDice->Draw();
			//m_pDice->TestDraw();
		}
		DirectX::XMFLOAT4 color = { 0.0f,0.0f,0.0f,0.0f };

		if (m_role)
		{
			m_role->Draw();
		}
		if (m_pScore)
		{
			m_pScore->Draw();
		}
		if (m_pRoleUI)
		{
			m_pRoleUI->Draw();
		}

	}
}
