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

#include <cmath>
#include <cstdlib>
#include <ctime>


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
static float StepSec60fps()
{
	return 1.0f / 120.0f;
}

void SceneGame::BeginTurn(TurnOwner owner)
{
	m_turnOwner = owner;

	m_bet = 0;
	m_rollUsed = 0;
	m_damageThisTurn = 0;

	if (m_pRoleUI) m_pRoleUI->SetTexture("Role/Role_None.png");

	m_enemyWaitSec = 1.0f;
	m_stopGuardFrames = 0;
	m_stopStableFrames = 0;
	m_rollElapsedSec = 0.0f;
}

void SceneGame::EndTurn()
{
	if (m_damageThisTurn > 0)
	{
		if (m_turnOwner == TurnOwner::Player)
		{
			m_enemyHP -= m_damageThisTurn;
			m_pEnemyHp->SetScore(m_enemyHP);
			if (m_enemyHP < 0) m_enemyHP = 0;
		}
		else
		{
			m_playerHP -= m_damageThisTurn;
			m_pPlayerHp->SetScore(m_playerHP);
			if (m_playerHP < 0) m_playerHP = 0;
		}
	}

	TurnOwner next = (m_turnOwner == TurnOwner::Player) ? TurnOwner::Enemy : TurnOwner::Player;
	BeginTurn(next);
}

// ---------------------------
// BetState handlers
// ---------------------------
void SceneGame::UpdateBetFlow(TurnOwner owner)
{
	switch (m_betState)
	{
	case BetState::WaitingBet:
		HandleWaitingBet(owner);
		break;
	case BetState::WaitingRoll:
		HandleWaitingRoll(owner);
		break;
	case BetState::Rolling:
		HandleRolling(owner);
		break;
	case BetState::Result:
	default:
		break;
	}
}

void SceneGame::HandleWaitingBet(TurnOwner owner)
{
	int nextBet = 0;

	if (owner == TurnOwner::Player)
	{
		nextBet = 5;
		// NOTE: if you want strict 5 only, keep only this line.
		if (IsKeyTrigger('1')) nextBet = 5;
		if (IsKeyTrigger('2')) nextBet = 10;
		if (nextBet == 0) return;
	}
	else
	{
		nextBet = 5;
	}

	if (nextBet <= 0) return;
	if (m_money < nextBet) return;

	m_bet = nextBet;
	m_rollUsed = 0;
	m_betState = BetState::WaitingRoll;
}

void SceneGame::HandleWaitingRoll(TurnOwner owner)
{
	if (owner == TurnOwner::Player)
	{
		if (IsKeyTrigger('R'))
		{
			StartRoll(owner);
		}
		return;
	}

	// Enemy: auto roll after a short delay
	m_enemyWaitSec -= StepSec60fps();
	if (m_enemyWaitSec <= 0.0f)
	{
		StartRoll(owner);
		m_enemyWaitSec = 3.0f;
	}
}

void SceneGame::StartRoll(TurnOwner owner)
{
	(void)owner;
	if (m_betState != BetState::WaitingRoll) return;
	if (m_rollUsed >= 3) return;
	if (m_bet <= 0) return;
	if (!m_pDice) return;

	// Pre-pay
	m_money -= m_bet;
	if (m_money < 0) m_money = 0;
	if (m_pMoneyUI) m_pMoneyUI->SetScore(m_money);

	m_roleFixedThisRoll = false;
	m_scoredThisRoll = false;

	m_rollUsed++;
	m_betState = BetState::Rolling;

	// Roll直後の「即IsStop」誤判定対策
	// 1) 直後は数フレーム停止判定を見ない
	// 2) IsStop()==true が数フレーム連続するまで確定しない
	// 3) Roll開始から最低時間(閾値)が経つまでは確定しない
	m_stopGuardFrames = 8;      // 120fps想定で約0.067s
	m_stopStableFrames = 0;
	m_rollElapsedSec = 0.0f;

	if (m_pYukari) m_pYukari->SetType(Yukari_Type::Think);
	if (m_pRoleUI) m_pRoleUI->SetTexture("Role/Role_None.png");

	// Roll
	m_pDice->RollRandom(0);
	m_pDice->RollRandom(2);
	m_pDice->RollRandom(3);
}

void SceneGame::HandleRolling(TurnOwner owner)
{
	(void)owner;
	if (!m_pDice) return;

	// Roll開始からの経過時間（固定120fps想定）
	m_rollElapsedSec += StepSec60fps();

	// Roll直後は数フレームだけ停止判定を見ない
	if (m_stopGuardFrames > 0)
	{
		m_stopGuardFrames--;
		return;
	}

	// 最低限の時間が経つまでは停止確定させない（敵の即停止対策）
	// ここは好みで調整していい（0.25〜0.5秒が無難）
	if (m_rollElapsedSec < 1.f)
	{
		return;
	}

	if (!m_pDice->IsStop())
	{
		m_stopStableFrames = 0;
		if (m_pCamera) m_pCamera->LockPos(false);
		return;
	}

	// IsStop()==true が連続したら「本当に止まった」とみなす
	// （1フレームだけtrueになるブレを吸収）
	{
		m_stopStableFrames++;
		if (m_stopStableFrames < 6)
		{
			return;
		}
	}

	if (m_pCamera) m_pCamera->LockPos(true);
	if (m_roleFixedThisRoll) return;

	RoleResult r{};
	if (!TryResolveStoppedRoll(r))
	{
		// Face values are not ready
		return;
	}

	ApplyRoleVisuals(r);
	ResolveBetOutcomeAndMaybeEndTurn(r);

	m_roleFixedThisRoll = true;
}

bool SceneGame::TryResolveStoppedRoll(RoleResult& outRole)
{
	TRAN_INS;
	const int a = tran.dice.currentFaceNumber[0];
	const int b = tran.dice.currentFaceNumber[2];
	const int c = tran.dice.currentFaceNumber[3];

	if (!(a >= 1 && a <= 6 && b >= 1 && b <= 6 && c >= 1 && c <= 6))
		return false;

	outRole = CalcRole(a, b, c);

	// Score
	if (m_pScore) m_pScore->AddScore(outRole.addScore);

	// Damage (avoid negative -> heal)
	m_damageThisTurn = (outRole.addScore > 0) ? outRole.addScore : 0;

	return true;
}

void SceneGame::ApplyRoleVisuals(const RoleResult& r)
{
	if (!m_pRoleUI) return;

	switch (r.role)
	{
	case RoleType::None:
		m_pRoleUI->SetTexture("Role/Role_None.png");
		if (m_pYukari) m_pYukari->SetType(Yukari_Type::UnHappy);
		break;
	case RoleType::Hifumi:
		m_pRoleUI->SetTexture("Role/Role_Hifumi.png");
		if (m_pYukari) m_pYukari->SetType(Yukari_Type::UnHappy);
		break;
	case RoleType::Shigoro:
		m_pRoleUI->SetTexture("Role/Role_Shigoro.png");
		if (m_pYukari) m_pYukari->SetType(Yukari_Type::Happy);
		break;
	case RoleType::Zorome:
		m_pRoleUI->SetTexture("Role/Role_Zorome.png");
		if (m_pYukari) m_pYukari->SetType(Yukari_Type::Happy);
		break;
	case RoleType::Pinzoro:
		m_pRoleUI->SetTexture("Role/Role_Pinzoro.png");
		if (m_pYukari) m_pYukari->SetType(Yukari_Type::Happy);
		break;
	case RoleType::Me:
	{
		char path[64];
		sprintf_s(path, "Role/Role_Me%d.png", r.me);
		m_pRoleUI->SetTexture(path);
		if (m_pYukari) m_pYukari->SetType(Yukari_Type::Happy);
		break;
	}
	default:
		break;
	}
}

void SceneGame::ResolveBetOutcomeAndMaybeEndTurn(const RoleResult& r)
{
	if (m_betState != BetState::Rolling) return;

	bool roundEnded = false;

	auto win = [&](int mult)
		{
			m_money += m_bet * mult;
			if (m_pMoneyUI) m_pMoneyUI->SetScore(m_money);

			m_bet = 0;
			m_rollUsed = 0;
			m_betState = BetState::WaitingBet;

			roundEnded = true;
		};

	auto continueRoll = [&]()
		{
			m_betState = BetState::WaitingRoll;
		};

	auto loseRound = [&]()
		{
			m_bet = 0;
			m_rollUsed = 0;
			m_betState = BetState::WaitingBet;

			roundEnded = true;
		};

	if (r.role == RoleType::None)
	{
		if (m_rollUsed >= 3) loseRound();
		else continueRoll();
	}
	else if (r.role == RoleType::Hifumi)
	{
		loseRound();
	}
	else
	{
		int mult = 1;
		switch (r.role)
		{
		case RoleType::Pinzoro: mult = 10; break;
		case RoleType::Zorome:  mult = 3;  break;
		case RoleType::Shigoro: mult = 2;  break;
		case RoleType::Me:      mult = 2;  break;
		default:                mult = 1;  break;
		}
		win(mult);
	}

	if (roundEnded)
	{
		EndTurn();
	}
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
	if (!m_pModel->Load("Assets/Model/Object/tyabudai.fbx", 1.f, Model::ZFlip)) { // 倍率と反転は省略可
		MessageBox(NULL, "Branch_01", "Error", MB_OK); // エラーメッセージの表示
	}
	m_pTyawan = new Model();
	if (!m_pTyawan->Load("Assets/Model/Object/tyawan.fbx", 1.f, Model::ZFlip)) { // 倍率と反転は省略可
		MessageBox(NULL, "Branch_01", "Error", MB_OK); // エラーメッセージの表示
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

	tran.tyabu.pos = { 0,-2.7f,0 };
	tran.tyabu.size = { 15.0f,0.4f,15.0f };

	tran.tyawan.pos = { 0,0.5f,0 };
	tran.tyawan.size = { 4,1.5f,4 };

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

	tran.diceui.role.pos = { m_roleListX ,m_roleListY };
	tran.diceui.role.size = { panelW,panelH };

	// 生成
	m_role = new UIObject("tintiro.png",
		tran.diceui.role.pos.x,
		tran.diceui.role.pos.y,
		tran.diceui.role.size.x,
		tran.diceui.role.size.y);
	// 初期位置を反映
	m_role->SetPosition(m_roleListX, m_roleListY);
	m_role->SetSize(panelW, panelH);

	// 所持金表示（位置は好みで調整）
	m_pMoneyUI = new ScoreLite("Number/number.png", 150.0f, 60.0f, 48.0f, 64.0f, 56.0f);
	m_pMoneyUI->SetScore(m_money);

	// 賭け初期化
	m_money = 200;
	m_bet = 0;
	m_rollUsed = 0;
	m_betState = BetState::WaitingBet;

	m_pYukari = new Yukari();

	isUsedYukari = false;

	m_pPlayerHp = new ScoreLite("Number/number.png", SCREEN_WIDTH - 200.0f, 60.0f, 48.0f, 64.0f, 56.0f);
	m_pPlayerHp->SetScore(m_playerHP);
	m_pEnemyHp = new ScoreLite("Number/number.png", SCREEN_WIDTH - 200.0f, 120.0f, 48.0f, 64.0f, 56.0f);
	m_pEnemyHp->SetScore(m_enemyHP);
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
	if (m_pScore)
	{
		delete m_pScore;
		m_pScore = nullptr;
	}
	if (m_pRoleUI)
	{
		delete m_pRoleUI;
		m_pRoleUI = nullptr;
	}
	if (m_pMoneyUI)
	{
		delete m_pMoneyUI;
		m_pMoneyUI = nullptr;
	}
	if (m_pYukari)
	{
		delete m_pYukari;
		m_pYukari = nullptr;
	}
}

void SceneGame::Update()
{
	if (m_playerHP <= 0)
	{
		SceneManager::ChangeScene(SceneManager::SCENE_RESULT);
		SceneManager::ChangeResult(SceneManager::ResultType::Lose);
	}
	if (m_enemyHP <= 0)
	{
		SceneManager::ChangeScene(SceneManager::SCENE_RESULT);
		SceneManager::ChangeResult(SceneManager::ResultType::Win);
	}

	if (m_pCamera) m_pCamera->Update();

	if (!OnlyDice) return;

	if (m_pDice)
	{
		m_pDice->Update(1);
		m_pDice->SetCamera(m_pCamera);
	}

	if (m_pYukari) m_pYukari->Update();

	// BetState per-turn handling
	UpdateBetFlow(m_turnOwner);

	// Shift toggle: role list panel
	if (IsKeyTrigger(VK_RSHIFT))
	{
		m_roleListOpen = !m_roleListOpen;
		m_roleListTargetX = m_roleListOpen ? openX : closeX;
	}

	const float dt = 1.0f / 120.0f;
	{
		float t = 1.0f - expf(-m_roleListSpeed * dt);
		m_roleListX = m_roleListX + (m_roleListTargetX - m_roleListX) * t;
		if (m_role) m_role->SetPosition(m_roleListX, m_roleListY);
	}

	if (IsKeyTrigger(VK_ESCAPE))
	{
		SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
	}

	// debug
	if (IsKeyTrigger('P')) m_playerHP = 0;
	if (IsKeyTrigger('O')) m_enemyHP = 0;
}



void SceneGame::Draw()
{
	using namespace DirectX;

	// 頂点シェーダーに渡す変換行列の変数を宣言 
	DirectX::XMFLOAT4X4 fWVP[3];    // World,View,Projectionの略  
	DirectX::XMMATRIX world, view, proj; // 各変換行列の格納先 
	TRAN_INS;
	// 作成した行列を各変数へ格納 
	world = DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f);

	XMFLOAT3 pos = tran.tyabu.pos;
	XMFLOAT3 size = tran.tyabu.size;
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(size.x, size.y, size.z);

	world = S * T;

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

	// 仮置きしているボックスにカメラを設定
	Geometory::SetView(fWVP[1]);
	Geometory::SetProjection(fWVP[2]);

	// 仮置きしているボックスにカメラを設定 
	Geometory::SetView(m_pCamera->GetViewMatrix());
	Geometory::SetProjection(m_pCamera->GetProjectionMatrix());

	// Spriteへカメラの行列を設定 
	Sprite::SetView(m_pCamera->GetViewMatrix());
	Sprite::SetProjection(m_pCamera->GetProjectionMatrix());


	SetDepthTest(true);


	if (OnlyDice && false)
	{
		// 参照用のインスタンスを取得
		TRAN_INS;

		if (m_pDice)
		{
			m_pDice->Draw();
			//m_pDice->TestDraw();
		}

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
		if (m_pMoneyUI)
		{
			m_pMoneyUI->Draw();
		}
		if (m_pYukari && isUsedYukari)
		{
			m_pYukari->Draw();
		}
		if (m_pPlayerHp)
		{
			m_pPlayerHp->Draw();
		}
		if (m_pEnemyHp)
		{
			m_pEnemyHp->Draw();
		}
	}
	// モデルの描画 基本それぞれのDrawで出力させるのでいらないがサンプルとして残す

	// シェーダーへ変換行列を設定 
	ShaderList::SetWVP(fWVP); // SetWVP関数の引数にはXMFLOAT4X4型で要素数３の配列のアドレスを渡す 

	// モデルに使用する頂点シェーダー、ピクセルシェーダーを設定 
	m_pModel->SetVertexShader(ShaderList::GetVS(ShaderList::VS_WORLD));
	m_pModel->SetPixelShader(ShaderList::GetPS(ShaderList::PS_LAMBERT));

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
	pos = tran.tyawan.pos;
	size = tran.tyawan.size;
	T = XMMatrixTranslation(pos.x, pos.y, pos.z);
	S = XMMatrixScaling(size.x, size.y, size.z);
	world = S * T;

	DirectX::XMStoreFloat4x4(&fWVP[0], DirectX::XMMatrixTranspose(world));

	ShaderList::SetWVP(fWVP); // SetWVP関数の引数にはXMFLOAT4X4型で要素数３の配列のアドレスを渡す 

	// モデルに使用する頂点シェーダー、ピクセルシェーダーを設定 
	m_pTyawan->SetVertexShader(ShaderList::GetVS(ShaderList::VS_WORLD));
	m_pTyawan->SetPixelShader(ShaderList::GetPS(ShaderList::PS_LAMBERT));

	// マテリアル別にメッシュを表示 
	for (unsigned int i = 0; i < m_pTyawan->GetMeshNum(); ++i) {
		// モデルのメッシュを取得 
		const Model::Mesh* mesh = m_pTyawan->GetMesh(i);
		// メッシュに割り当てられているマテリアルを取得 
		Model::Material material = *m_pTyawan->GetMaterial(mesh->materialID);
		// シェーダーへマテリアルを設定 
		ShaderList::SetMaterial(material);
		// モデルの描画 
		m_pTyawan->Draw(i);
	}
	if (OnlyDice)
	{
		// 参照用のインスタンスを取得
		TRAN_INS;

		if (m_pDice)
		{
			m_pDice->Draw();
			//m_pDice->TestDraw();
		}

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
		if (m_pMoneyUI)
		{
			m_pMoneyUI->Draw();
		}
		if (m_pYukari && isUsedYukari)
		{
			m_pYukari->Draw();
		}
		if (m_turnOwner == TurnOwner::Player)
		{
			m_pPlayerHp->SetColor(1, 0, 0, 1);
			m_pEnemyHp->SetColor(1, 1, 1, 1);
		}
		else
		{
			m_pPlayerHp->SetColor(1,1,1,1);
			m_pEnemyHp->SetColor(1,0,0,1);
		}
		if (m_pPlayerHp)
		{
			m_pPlayerHp->Draw();
		}
		if (m_pEnemyHp)
		{
			m_pEnemyHp->Draw();
		}
	}
}