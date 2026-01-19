/*****************************************************************//**
 * \file   SceneGame.cpp
 * \brief  ・ｽQ・ｽ[・ｽ・ｽ・ｽV・ｽ[・ｽ・ｽ
 * 
 * \author ・ｽR・ｽ{・ｽ・ｽ・ｽ
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

	// ・ｽs・ｽ・ｽ・ｽ]・ｽ・ｽ
	if (d0 == 1 && d1 == 1 && d2 == 1)
		return { RoleType::Pinzoro, 100, 0 };

	// ・ｽ]・ｽ・ｽ・ｽ・ｽ
	if (d0 == d1 && d1 == d2)
		return { RoleType::Zorome, 50 + d0 * 10, 0 };

	// ・ｽV・ｽS・ｽ・ｽ
	if (d0 == 4 && d1 == 5 && d2 == 6)
		return { RoleType::Shigoro, 30, 0 };

	// ・ｽq・ｽt・ｽ~
	if (d0 == 1 && d1 == 2 && d2 == 3)
		return { RoleType::Hifumi, -20, 0 };

	// ・ｽﾊ擾ｿｽ・ｽ・ｽi2・ｽﾂ難ｿｽ・ｽ・ｽ + ・ｽc・ｽ・ｽ1・ｽﾂ）
	if (d0 == d1 && d1 != d2)
		return { RoleType::Me, d2, d2 };

	if (d0 != d1 && d1 == d2)
		return { RoleType::Me, d0, d0 };

	// ・ｽ・ｽ・ｽﾈゑｿｽ
	return { RoleType::None, 0, 0 };
}

static float StepSec60fps()
{
	return 1.0f / 60.0f;
}

void SceneGame::BeginTurn(TurnOwner owner)
{
	m_turnOwner = owner;
	m_turnPhase = TurnPhase::Betting;

	m_bet = 0;
	m_rollUsed = 0;
	m_betState = BetState::WaitingBet;
	m_damageThisTurn = 0;
	m_resultReady = false;
	m_roleFixedThisRoll = false;
	m_scoredThisRoll = false;

	if (m_pRoleUI) m_pRoleUI->SetTexture("Role/Role_None.png");

	if (m_pTurnUI)
	{
		const char* turnTex = (owner == TurnOwner::Player) ? "Character/Player.png" : "Character/Enemy.png";
		m_pTurnUI->SetTexture(turnTex);
	}

	m_enemyWaitSec = (owner == TurnOwner::Enemy) ? 1.0f : 0.0f;
}

void SceneGame::EndTurn()
{
	if (m_damageThisTurn > 0)
	{
		if (m_turnOwner == TurnOwner::Player)
		{
			m_enemyHP -= m_damageThisTurn;
			if (m_enemyHP < 0) m_enemyHP = 0;
		}
		else
		{
			m_playerHP -= m_damageThisTurn;
			if (m_playerHP < 0) m_playerHP = 0;
		}
	}

	if (m_pPlayerHp) m_pPlayerHp->SetScore(m_playerHP);
	if (m_pEnemyHp) m_pEnemyHp->SetScore(m_enemyHP);
	TurnOwner next = (m_turnOwner == TurnOwner::Player) ? TurnOwner::Enemy : TurnOwner::Player;
	BeginTurn(next);
}


namespace
{
	const float kRolePanelWidth = 375.0f;
	const float kRolePanelHeight = 520.0f;

	// ・ｽ\・ｽ・ｽ・ｽﾊ置・ｽi・ｽJ・ｽ・ｽ・ｽﾄゑｿｽﾆゑｿｽ・ｽﾍ右・ｽﾉ寄せて擾ｿｽ・ｽ・ｽ・ｽ]・ｽ・ｽ・ｽj
	const float kRolePanelOpenX = SCREEN_WIDTH - kRolePanelWidth * 0.5f - 20.0f;
	// ・ｽﾂゑｿｽ・ｽﾄゑｿｽﾆゑｿｽ・ｽﾍ会ｿｽﾊ外・ｽi・ｽE・ｽﾉ難ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽj
	const float kRolePanelCloseX = SCREEN_WIDTH + kRolePanelWidth * 0.5f + 20.0f;

	const float kRolePanelLerpDt = 1.0f / 120.0f;

	const float kTurnIndicatorSize = 96.0f;
	const float kTurnIndicatorMargin = 20.0f;

	const float kHpDigitW = 48.0f;
	const float kHpDigitH = 64.0f;
	const float kHpDigitSpacing = 56.0f;
	const float kHpIconSize = 64.0f;
	const float kHpIconGap = 12.0f;
	const float kPlayerHpX = 140.0f;
	const float kPlayerHpY = 60.0f;
	const float kEnemyHpX = SCREEN_WIDTH - 140.0f;
	const float kEnemyHpY = 160.0f;
	const float kHpIconOffsetX = kHpDigitW * 0.5f + kHpIconGap + kHpIconSize * 0.5f;
}

SceneGame::SceneGame()
	:OnlyDice(true)
{

	m_pCamera = new CameraDebug();


	m_pTyabu = new Model();

	if (!m_pTyabu->Load("Assets/Model/Object/tyabudai.fbx", 1.f, Model::ZFlip)) { // 倍率と反転は省略可
		MessageBox(NULL, "Branch_01", "Error", MB_OK); // エラーメッセージの表示
	}

	
	m_pTyawan = new Model();
	if (!m_pTyawan->Load("Assets/Model/Object/tyawan.fbx", 1.f, Model::ZFlip)) { // 倍率と反転は省略可
		MessageBox(NULL, "Branch_01", "Error", MB_OK); // エラーメッセージの表示
	}

	//--- ・ｽ・ｽ・ｽf・ｽ・ｽ・ｽﾌ描・ｽ・ｽ
	RenderTarget* pRTV = GetDefaultRTV(); // ・ｽf・ｽt・ｽH・ｽ・ｽ・ｽg・ｽ・ｽRenderTargetView・ｽ・ｽ・ｽ謫ｾ
	DepthStencil* pDSV = GetDefaultDSV(); // ・ｽf・ｽt・ｽH・ｽ・ｽ・ｽg・ｽ・ｽDepthStencilView・ｽ・ｽ・ｽ謫ｾ
	SetRenderTargets(1, &pRTV, pDSV); // ・ｽ・ｽ3・ｽ・ｽ・ｽ・ｽ・ｽ・ｽnull・ｽﾌ場合・ｽA2D・ｽ\・ｽ・ｽ・ｽﾆなゑｿｽ

	SetDepthTest(true);

	// ・ｽ・ｽﾂゑｿｽ・ｽ・ｽ
	m_pDice = new Dice();
	m_pDice->SetCamera(m_pCamera);
	TRAN_INS;



	m_pScore = new ScoreLite("Number/number.png", 360.0f, 40.0f, 48.0f, 64.0f, 56.0f);
	m_pScore->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_scoredThisRoll = false;

	m_pRoleUI = new UIObject(
		"Role/Role_None.png",   // ・ｽ・ｽ・ｽi・ｽ・ｽ・ｽﾆで搾ｿｽ・ｽ・ｽ・ｽﾖゑｿｽ・ｽ・ｽj
		360.0f, 120.0f,
		256.0f, 96.0f
	);

	m_pRoleUI->SetColor(1, 1, 1, 1);
	m_roleFixedThisRoll = false;

	const float turnX = SCREEN_WIDTH - kTurnIndicatorMargin - kTurnIndicatorSize * 0.5f - 50.0f;
	const float turnY = kTurnIndicatorMargin + 20.0f;
	m_pTurnUI = new UIObject("Character/Player.png", turnX, turnY, kTurnIndicatorSize, kTurnIndicatorSize);
	m_pTurnUI->SetColor(1, 1, 1, 1);

	// ・ｽ・ｽ・ｽ・ｽ・ｽﾍ好・ｽﾝ。・ｽ・ｽｹなゑｿｽ 140?200 ・ｽ・ｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽ・ｽ竄ｷ・ｽ・ｽ
	m_roleListY = SCREEN_HEIGHT - kRolePanelHeight * 0.5f;

	m_roleListOpen = false;
	m_roleListX = kRolePanelCloseX;
	m_roleListTargetX = kRolePanelCloseX;

	// ・ｽ・ｽ・ｽx・ｽi1・ｽb・ｽﾅほぼ難ｿｽ・ｽB・ｽ・ｽ・ｽ驍ｭ・ｽ轤｢・ｽj
	m_roleListSpeed = 14.0f;

	tran.diceui.role.pos = { m_roleListX ,m_roleListY};
	tran.diceui.role.size = {kRolePanelWidth,kRolePanelHeight};

	// ・ｽ・ｽ・ｽ・ｽ
	m_role = new UIObject("tintiro.png",
		tran.diceui.role.pos.x,
		tran.diceui.role.pos.y,
		tran.diceui.role.size.x,
		tran.diceui.role.size.y);
	// ・ｽ・ｽ・ｽ・ｽ・ｽﾊ置・ｽｽ映
	m_role->SetPosition(m_roleListX, m_roleListY);
	m_role->SetSize(kRolePanelWidth, kRolePanelHeight);

	// ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ\・ｽ・ｽ・ｽi・ｽﾊ置・ｽﾍ好・ｽﾝで抵ｿｽ・ｽ・ｽ・ｽj
	m_pMoneyUI = new ScoreLite("Number/number.png", 150.0f, 60.0f, 48.0f, 64.0f, 56.0f);
	m_pMoneyUI->SetScore(m_money);

	// HP・ｽ\・ｽ・ｽ
	m_pPlayerHp = new ScoreLite("Number/number.png", kPlayerHpX, kPlayerHpY, kHpDigitW, kHpDigitH, kHpDigitSpacing);
	m_pPlayerHp->SetScore(m_playerHP);
	m_pEnemyHp = new ScoreLite("Number/number.png", kEnemyHpX, kEnemyHpY, kHpDigitW, kHpDigitH, kHpDigitSpacing);
	m_pEnemyHp->SetScore(m_enemyHP);

	const float playerHpIconX = kPlayerHpX + kHpIconOffsetX;
	const float enemyHpIconX = kEnemyHpX + kHpIconOffsetX;
	m_pPlayerHpIcon = new UIObject("Character/Player.png", playerHpIconX, kPlayerHpY, kHpIconSize, kHpIconSize);
	m_pEnemyHpIcon = new UIObject("Character/Enemy.png", enemyHpIconX, kEnemyHpY, kHpIconSize, kHpIconSize);
	// ・ｽq・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
	m_money = 200;
	m_bet = 0;
	m_rollUsed = 0;
	m_betState = BetState::WaitingBet;

	m_pYukari = new Yukari();

	isUsedYukari = false;

	BeginTurn(TurnOwner::Player);
}

SceneGame::~SceneGame()
{
	if (m_pTyabu)
	{
		delete m_pTyabu;
		m_pTyabu = nullptr;
	}
	if (m_pTyawan)
	{
		delete m_pTyawan;
		m_pTyawan = nullptr;
	}
	if (m_pCamera) {
		delete m_pCamera;
		m_pCamera = nullptr;
	}
	if (m_pDice)
	{
		delete m_pDice;
		m_pDice = nullptr;
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
	if (m_pTurnUI)
	{
		delete m_pTurnUI;
		m_pTurnUI = nullptr;
	}
	if (m_pMoneyUI)
	{
		delete m_pMoneyUI;
		m_pMoneyUI = nullptr;
	}
	if (m_pPlayerHp)
	{
		delete m_pPlayerHp;
		m_pPlayerHp = nullptr;
	}
	if (m_pEnemyHp)
	{
		delete m_pEnemyHp;
		m_pEnemyHp = nullptr;
	}
	if (m_pPlayerHpIcon)
	{
	delete m_pPlayerHpIcon;
	m_pPlayerHpIcon = nullptr;
	}
	if (m_pEnemyHpIcon)
	{
	delete m_pEnemyHpIcon;
	m_pEnemyHpIcon = nullptr;
	}
	if (m_pYukari)
	{
		delete m_pYukari;
		m_pYukari = nullptr;
	}
}

void SceneGame::Update()
{
	m_pCamera->Update();
	if (!OnlyDice)
	{
		UpdatePlayerMode();
		return;
	}
	if (IsKeyTrigger('O'))
	{
		m_playerHP -= 50;
		m_pPlayerHp->SetScore(m_playerHP);
	}
	if (IsKeyTrigger('P'))
	{
		m_enemyHP -= 50;
		m_pEnemyHp->SetScore(m_enemyHP);
	}

	if (m_playerHP <= 0)
	{
		SceneManager::ChangeScene(SceneManager::SceneType::SCENE_RESULT);
		SceneManager::ChangeResult(SceneManager::ResultType::Lose);
	}

	else if (m_enemyHP <= 0)
	{
		SceneManager::ChangeScene(SceneManager::SceneType::SCENE_RESULT);
		SceneManager::ChangeResult(SceneManager::ResultType::Win);
	}

	UpdateDiceMode();
}

void SceneGame::UpdatePlayerMode()
{
}

void SceneGame::UpdateDiceMode()
{
	m_pDice->Update(1);
	m_pDice->SetCamera(m_pCamera);

	m_pYukari->Update();

	if (m_turnOwner == TurnOwner::Enemy)
	{
		UpdateEnemyTurn();
	}
	else
	{
		HandleDiceStop();
		HandlePlayerAutoBet();
		HandleBetInput();
		HandleRollInput();
	}

	if (m_resultReady)
	{
		ApplyBetResult(m_cachedRole);
		m_resultReady = false;
	}

	UpdateRoleListPanel(kRolePanelLerpDt);

	if (IsKeyTrigger(VK_ESCAPE))
	{
		SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
	}
}

void SceneGame::UpdateEnemyTurn()
{
	if (m_turnPhase == TurnPhase::Betting || m_turnPhase == TurnPhase::WaitingRoll)
	{
		if (m_rollUsed >= 3)
		{
			return;
		}

		m_enemyWaitSec -= StepSec60fps();
		if (m_enemyWaitSec > 0.0f)
			return;

		if (m_bet <= 0)
		{
			m_bet = 5;
		}

		m_turnPhase = TurnPhase::Rolling;
		m_betState = BetState::Rolling;
		m_roleFixedThisRoll = false;
		m_scoredThisRoll = false;
		m_rollUsed++;

		if (m_pRoleUI) m_pRoleUI->SetTexture("Role/Role_None.png");
		if (m_pDice)
		{
			m_pDice->RollRandom(0);
			m_pDice->RollRandom(2);
			m_pDice->RollRandom(3);
		}
		if (m_pYukari) m_pYukari->SetType(Yukari_Type::Think);
	}

	HandleDiceStop();
}
void SceneGame::HandleDiceStop()
{
	if (!m_pDice->IsStop())
	{
		m_pCamera->LockPos(false);
		return;
	}

	m_pCamera->LockPos(true);

	if (m_betState != BetState::Rolling)
		return;

	if (m_roleFixedThisRoll)
		return;

	TRAN_INS;
	const int a = tran.dice.currentFaceNumber[0];
	const int b = tran.dice.currentFaceNumber[2];
	const int c = tran.dice.currentFaceNumber[3];
	if (a < 1 || a > 6 || b < 1 || b > 6 || c < 1 || c > 6)
		return;

	RoleResult r = CalcRole(a, b, c);

	// ・ｽX・ｽR・ｽA・ｽ・ｽ・ｽZ
	m_pScore->AddScore(r.addScore);

	// ・ｽｼ表・ｽ・ｽ
	UpdateRoleUI(r);

	// ・ｽq・ｽ・ｽ・ｽ・ｽ・ｽﾊ（・ｽ~・ｽﾜゑｿｽ・ｽ・ｽ・ｽu・ｽﾔに確・ｽ・ｽj
	m_cachedRole = r;
	m_resultReady = true;

	m_roleFixedThisRoll = true;
}

void SceneGame::UpdateRoleUI(const RoleResult& r)
{
	switch (r.role)
	{
	case RoleType::None:
		m_pRoleUI->SetTexture("Role/Role_None.png");
		m_pYukari->SetType(Yukari_Type::UnHappy);
		break;
	case RoleType::Hifumi:
		m_pRoleUI->SetTexture("Role/Role_Hifumi.png");
		m_pYukari->SetType(Yukari_Type::UnHappy);
		break;
	case RoleType::Shigoro:
		m_pRoleUI->SetTexture("Role/Role_Shigoro.png");
		m_pYukari->SetType(Yukari_Type::Happy);
		break;
	case RoleType::Zorome:
		m_pRoleUI->SetTexture("Role/Role_Zorome.png");
		m_pYukari->SetType(Yukari_Type::Happy);
		break;
	case RoleType::Pinzoro:
		m_pRoleUI->SetTexture("Role/Role_Pinzoro.png");
		m_pYukari->SetType(Yukari_Type::Happy);
		break;
	case RoleType::Me:
	{
		char path[64];
		sprintf_s(path, "Role/Role_Me%d.png", r.me);
		m_pRoleUI->SetTexture(path);
		m_pYukari->SetType(Yukari_Type::Happy);
		break;
	}
	default:
		// ・ｽ・ｽ・ｽﾈゑｿｽ・ｽﾈゑｿｽ\・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ or --- ・ｽﾉゑｿｽ・ｽ・ｽ
		break;
	}
}

void SceneGame::ApplyBetResult(const RoleResult& r)
{
	bool roundEnded = false;

	auto win = [&](int mult, int damageMult)
	{
		const int damage = m_bet * damageMult;
		if (m_turnOwner == TurnOwner::Player)
		{
			m_money = m_bet * mult;
			m_enemyHP -= m_money;
			if (m_pEnemyHp)m_pEnemyHp->SetScore(m_enemyHP);
		}
		else
		{
			m_money = m_bet * mult;
			m_playerHP -= m_money;
			if (m_pPlayerHp)m_pPlayerHp->SetScore(m_playerHP);
		}

		// ・ｽ・ｽ・ｽE・ｽ・ｽ・ｽh・ｽI・ｽ・ｽ
		m_bet = 0;
		m_rollUsed = 0;
		m_betState = BetState::WaitingBet;
		m_turnPhase = TurnPhase::TurnEnd;
		m_damageThisTurn = damage;
		roundEnded = true;
	};

	auto continueRoll = [&]()
	{
		// ・ｽ・ｽ・ｽﾈゑｿｽ・ｽﾅ回数残・ｽ・ｽ・ｽﾄゑｿｽﾈら次・ｽﾌ・ｿｽ・ｽ[・ｽ・ｽ・ｽﾒゑｿｽ
		m_betState = BetState::WaitingRoll;
		m_turnPhase = TurnPhase::WaitingRoll;
	};

	auto loseRound = [&]()
	{
		// ・ｽ・ｽ・ｽﾅに先払・ｽ・ｽ・ｽﾏみなので、・ｽ・ｽ・ｽ・ｽ・ｽﾅは追会ｿｽ・ｽ・ｽ・ｽZ・ｽ・ｽ・ｽﾈゑｿｽ
		m_bet = 0;
		m_rollUsed = 0;
		m_betState = BetState::WaitingBet;
		m_turnPhase = TurnPhase::TurnEnd;
		m_damageThisTurn = 0;
		roundEnded = true;
	};

	if (r.role == RoleType::None)
	{
		if (m_rollUsed >= 3) loseRound();
		else continueRoll();
	}
	else if (r.role == RoleType::Hifumi)
	{
		// ・ｽq・ｽt・ｽ~・ｽﾍ托ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽi・ｽ謨･・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾅ終・ｽ・ｽ・ｽj
		loseRound();
	}
	else
	{
		int mult = 1;
		int damageMult = 0;
		switch (r.role)
		{
		case RoleType::Pinzoro: mult = 10; damageMult = 6; break;
		case RoleType::Zorome:  mult = 3;  damageMult = 4; break;
		case RoleType::Shigoro: mult = 2;  damageMult = 3; break;
		case RoleType::Me:      mult = 2;  damageMult = 2; break;
		default:                mult = 1;  damageMult = 1; break;
		}
		win(mult, damageMult);
	}

	if (roundEnded)
	{
		EndTurn();
	}
}

void SceneGame::HandlePlayerAutoBet()
{
	if (m_turnOwner != TurnOwner::Player)
		return;

	if (m_betState != BetState::WaitingBet)
		return;

	const int kMinBet = 5;
	if (m_money < kMinBet)
		return;

	m_bet = kMinBet;
	m_rollUsed = 0;
	m_betState = BetState::WaitingRoll;
	m_turnPhase = TurnPhase::WaitingRoll;

	if (m_pRoleUI) m_pRoleUI->SetTexture("Role/Role_None.png");
}

void SceneGame::HandleBetInput()
{
	// ・ｽx・ｽb・ｽg・ｽI・ｽ・ｽ・ｽiWaitingBet ・ｽﾌとゑｿｽ・ｽ・ｽ・ｽ・ｽ・ｽj
	if (m_betState != BetState::WaitingBet)
		return;

	int nextBet = 0;
	if (IsKeyTrigger('1')) nextBet = 5;
	if (IsKeyTrigger('2')) nextBet = 10;

	if (nextBet <= 0)
		return;

	// ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽs・ｽ・ｽ・ｽﾈら無・ｽ・ｽ・ｽiUI・ｽo・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾈゑｿｽ・ｽﾅ）
	if (m_money < nextBet)
		return;

	m_bet = nextBet;
	m_rollUsed = 0;
	m_betState = BetState::WaitingRoll;
	m_turnPhase = TurnPhase::WaitingRoll;

	// ・ｽ・ｽ\・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ None ・ｽ・ｽ
	if (m_pRoleUI) m_pRoleUI->SetTexture("Role/Role_None.png");
}

void SceneGame::HandleRollInput()
{
	if (m_turnOwner != TurnOwner::Player)
		return;

	if (!IsKeyTrigger('R'))
		return;

	// ・ｽ・ｽ・ｽE・ｽ・ｽ・ｽh・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽU・ｽ・ｽ・ｽ
	if (m_betState != BetState::WaitingRoll)
		return;

	if (m_rollUsed < 3 && m_pDice)
	{
		// ・ｽ謨･・ｽ・ｽ・ｽi・ｽU・ｽ・ｽ・ｽﾄゑｿｽﾅ抵ｿｽ・ｽﾉ鯉ｿｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ・ｽ・ｽﾔになゑｿｽj
		m_money -= m_bet;
		if (m_money < 0) m_money = 0;
		if (m_pMoneyUI) m_pMoneyUI->SetScore(m_money);

		m_roleFixedThisRoll = false;
		m_scoredThisRoll = false;

		m_rollUsed++;
		m_betState = BetState::Rolling;
		m_turnPhase = TurnPhase::Rolling;

		m_pDice->RollRandom(0);
		m_pDice->RollRandom(2);
		m_pDice->RollRandom(3);
	}
	m_pYukari->SetType(Yukari_Type::Think);
}

void SceneGame::UpdateRoleListPanel(float dt)
{
	// Shift・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾑに開・ｽ・ｽ
	if (IsKeyTrigger(VK_RSHIFT))
	{
		m_roleListOpen = !m_roleListOpen;

		m_roleListTargetX = m_roleListOpen ? kRolePanelOpenX : kRolePanelCloseX;
	}

	// Lerp・ｽﾅ奇ｿｽ・ｽ轤ｩ・ｽﾉ追従・ｽi・ｽw・ｽ・ｽ・ｽﾇ従・ｽj
	float t = 1.0f - expf(-m_roleListSpeed * dt);
	m_roleListX = m_roleListX + (m_roleListTargetX - m_roleListX) * t;

	m_role->SetPosition(m_roleListX, m_roleListY);
}

void SceneGame::Draw()
{
	// ・ｽ・ｽ・ｽ_・ｽV・ｽF・ｽ[・ｽ_・ｽ[・ｽﾉ渡・ｽ・ｽ・ｽﾏ奇ｿｽ・ｽs・ｽ・ｽﾌ変撰ｿｽ・ｽ・ｽ骭ｾ 
	DirectX::XMFLOAT4X4 fWVP[3];    // World,View,Projection・ｽﾌ暦ｿｽ  
	DirectX::XMMATRIX world, view, proj; // ・ｽe・ｽﾏ奇ｿｽ・ｽs・ｽ・ｽﾌ格・ｽ[・ｽ・ｽ 

	// ・ｽ・ｬ・ｽ・ｽ・ｽ・ｽ・ｽs・ｽ・ｽ・ｽ・ｽe・ｽﾏ撰ｿｽ・ｽﾖ格・ｽ[ 
	world = DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	view = DirectX::XMMatrixLookAtLH(
		DirectX::XMVectorSet(0.0f, 1.5f, -2.0f, 0.0f),
		DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
		DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	proj =
		DirectX::XMMatrixOrthographicOffCenterLH(
			-640, 640,  // ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽl
			-360, 360,  // ・ｽc・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽl
			0.001f,     // Near
			1000.0f);   // Far
	proj = DirectX::XMMatrixPerspectiveFovLH(
		// DirectXMath・ｽﾉ用・ｽﾓゑｿｽ・ｽ・ｽﾄゑｿｽ・ｽ・ｽp・ｽx・ｽ・ｽ・ｽ・ｽ・ｽW・ｽA・ｽ・ｽ・ｽp・ｽﾉ変奇ｿｽ・ｽ・ｽ・ｽ・ｽﾖ撰ｿｽ
		DirectX::XMConvertToRadians(70.0f), //・ｽp・ｽx
		16.0f / 9.0f,                      //・ｽA・ｽX・ｽ・ｽ
		0.1f,                              //・ｽﾅ擾ｿｽ・ｽ`・ｽ諡暦ｿｽ・ｽ
		100.0f);                           //・ｽﾅ抵ｿｽ・ｽ`・ｽ諡暦ｿｽ・ｽ



	// ・ｽv・ｽZ・ｽp・ｽﾌデ・ｽ[・ｽ^・ｽ・ｽ・ｽ・ｽﾇみ趣ｿｽ・ｽp・ｽﾌデ・ｽ[・ｽ^・ｽﾉ変奇ｿｽ 
	DirectX::XMStoreFloat4x4(&fWVP[0], DirectX::XMMatrixTranspose(world));
	DirectX::XMStoreFloat4x4(&fWVP[1], DirectX::XMMatrixTranspose(view));
	DirectX::XMStoreFloat4x4(&fWVP[2], DirectX::XMMatrixTranspose(proj));

	// ・ｽ・ｽ・ｽf・ｽ・ｽ・ｽﾉ変奇ｿｽ・ｽs・ｽ・ｽ・ｽﾝ抵ｿｽ 
	fWVP[1] = m_pCamera->GetViewMatrix();
	fWVP[2] = m_pCamera->GetProjectionMatrix();

	// ・ｽV・ｽF・ｽ[・ｽ_・ｽ[・ｽﾖ変奇ｿｽ・ｽs・ｽ・ｽ・ｽﾝ抵ｿｽ 
	ShaderList::SetWVP(fWVP); // SetWVP・ｽﾖ撰ｿｽ・ｽﾌ茨ｿｽ・ｽ・ｽ・ｽﾉゑｿｽXMFLOAT4X4・ｽ^・ｽﾅ要・ｽf・ｽ・ｽ・ｽR・ｽﾌ配・ｽ・ｽﾌア・ｽh・ｽ・ｽ・ｽX・ｽ・ｽn・ｽ・ｽ 


	// ・ｽ・ｽ・ｽf・ｽ・ｽ・ｽﾉ使・ｽp・ｽ・ｽ・ｽ髓ｸ・ｽ_・ｽV・ｽF・ｽ[・ｽ_・ｽ[・ｽA・ｽs・ｽN・ｽZ・ｽ・ｽ・ｽV・ｽF・ｽ[・ｽ_・ｽ[・ｽ・ｽﾝ抵ｿｽ 
	//m_pModel->SetVertexShader(ShaderList::GetVS(ShaderList::VS_WORLD));
	//m_pModel->SetPixelShader(ShaderList::GetPS(ShaderList::PS_LAMBERT));

	// ・ｽ・ｽ・ｽu・ｽ・ｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ・ｽ{・ｽb・ｽN・ｽX・ｽﾉカ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾝ抵ｿｽ
	Geometory::SetView(fWVP[1]);
	Geometory::SetProjection(fWVP[2]);

	// ・ｽ・ｽ・ｽu・ｽ・ｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ・ｽ{・ｽb・ｽN・ｽX・ｽﾉカ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾝ抵ｿｽ 
	Geometory::SetView(m_pCamera->GetViewMatrix());
	Geometory::SetProjection(m_pCamera->GetProjectionMatrix());

	// Sprite・ｽﾖカ・ｽ・ｽ・ｽ・ｽ・ｽﾌ行・ｽ・ｽ・ｽﾝ抵ｿｽ 
	Sprite::SetView(m_pCamera->GetViewMatrix());
	Sprite::SetProjection(m_pCamera->GetProjectionMatrix());
	using namespace DirectX;

	XMFLOAT3 pos;
	XMFLOAT3 size;


	DirectX::XMMATRIX T;
	DirectX::XMMATRIX S;

	pos = { 0,-2.7f,0 };
	size = { 15.0f,0.4f,15.0f };

	T = XMMatrixTranslation(pos.x,pos.y,pos.z);
	S = XMMatrixScaling(size.x,size.y,size.z);

	world = S * T;

	DirectX::XMStoreFloat4x4(&fWVP[0], DirectX::XMMatrixTranspose(world));

	Sprite::SetView(m_pCamera->GetViewMatrix());
	Sprite::SetProjection(m_pCamera->GetProjectionMatrix());
	// ・ｽ・ｽ・ｽf・ｽ・ｽ・ｽﾌ描・ｽ・ｽ ・ｽ・ｽ{・ｽ・ｽ・ｽ黷ｼ・ｽ・ｽ・ｽDraw・ｽﾅ出・ｽﾍゑｿｽ・ｽ・ｽ・ｽ・ｽﾌでゑｿｽ・ｽ・ｽﾈゑｿｽ・ｽ・ｽ・ｽT・ｽ・ｽ・ｽv・ｽ・ｽ・ｽﾆゑｿｽ・ｽﾄ残・ｽ・ｽ
	ShaderList::SetWVP(fWVP); // SetWVP・ｽﾖ撰ｿｽ・ｽﾌ茨ｿｽ・ｽ・ｽ・ｽﾉゑｿｽXMFLOAT4X4・ｽ^・ｽﾅ要・ｽf・ｽ・ｽ・ｽR・ｽﾌ配・ｽ・ｽﾌア・ｽh・ｽ・ｽ・ｽX・ｽ・ｽn・ｽ・ｽ 

	if(false)
	{
		// ・ｽ}・ｽe・ｽ・ｽ・ｽA・ｽ・ｽ・ｽﾊに・ｿｽ・ｽb・ｽV・ｽ・ｽ・ｽ・ｽ\・ｽ・ｽ 
		for (unsigned int i = 0; i < m_pTyawan->GetMeshNum(); ++i) {
			// ・ｽ・ｽ・ｽf・ｽ・ｽ・ｽﾌ・ｿｽ・ｽb・ｽV・ｽ・ｽ・ｽ・ｽ・ｽ謫ｾ 
			const Model::Mesh* mesh = m_pTyawan->GetMesh(i);
			// ・ｽ・ｽ・ｽb・ｽV・ｽ・ｽ・ｽﾉ奇ｿｽ・ｽ闢厄ｿｽﾄゑｿｽ・ｽﾄゑｿｽ・ｽ・ｽ}・ｽe・ｽ・ｽ・ｽA・ｽ・ｽ・ｽ・ｽ・ｽ謫ｾ 
			Model::Material material = *m_pTyawan->GetMaterial(mesh->materialID);
			// ・ｽV・ｽF・ｽ[・ｽ_・ｽ[・ｽﾖマ・ｽe・ｽ・ｽ・ｽA・ｽ・ｽ・ｽ・ｽﾝ抵ｿｽ 
			ShaderList::SetMaterial(material);
			// ・ｽ・ｽ・ｽf・ｽ・ｽ・ｽﾌ描・ｽ・ｽ 
			m_pTyawan->Draw(i);
		}
	}

	pos = { 0,0.5f,0 };
	size = { 4,1.5f,4 };

	T = XMMatrixTranslation(pos.x, pos.y, pos.z);
	S = XMMatrixScaling(size.x, size.y, size.z);

	world = S * T;

	DirectX::XMStoreFloat4x4(&fWVP[0], DirectX::XMMatrixTranspose(world));

	Sprite::SetView(m_pCamera->GetViewMatrix());
	Sprite::SetProjection(m_pCamera->GetProjectionMatrix());
	ShaderList::SetWVP(fWVP); // SetWVP・ｽﾖ撰ｿｽ・ｽﾌ茨ｿｽ・ｽ・ｽ・ｽﾉゑｿｽXMFLOAT4X4・ｽ^・ｽﾅ要・ｽf・ｽ・ｽ・ｽR・ｽﾌ配・ｽ・ｽﾌア・ｽh・ｽ・ｽ・ｽX・ｽ・ｽn・ｽ・ｽ 
	if(false)
	{
		// ・ｽ}・ｽe・ｽ・ｽ・ｽA・ｽ・ｽ・ｽﾊに・ｿｽ・ｽb・ｽV・ｽ・ｽ・ｽ・ｽ\・ｽ・ｽ 
		for (unsigned int i = 0; i < m_pTyabu->GetMeshNum(); ++i) {
			// ・ｽ・ｽ・ｽf・ｽ・ｽ・ｽﾌ・ｿｽ・ｽb・ｽV・ｽ・ｽ・ｽ・ｽ・ｽ謫ｾ 
			const Model::Mesh* mesh = m_pTyabu->GetMesh(i);
			// ・ｽ・ｽ・ｽb・ｽV・ｽ・ｽ・ｽﾉ奇ｿｽ・ｽ闢厄ｿｽﾄゑｿｽ・ｽﾄゑｿｽ・ｽ・ｽ}・ｽe・ｽ・ｽ・ｽA・ｽ・ｽ・ｽ・ｽ・ｽ謫ｾ 
			Model::Material material = *m_pTyabu->GetMaterial(mesh->materialID);
			// ・ｽV・ｽF・ｽ[・ｽ_・ｽ[・ｽﾖマ・ｽe・ｽ・ｽ・ｽA・ｽ・ｽ・ｽ・ｽﾝ抵ｿｽ 
			ShaderList::SetMaterial(material);
			// ・ｽ・ｽ・ｽf・ｽ・ｽ・ｽﾌ描・ｽ・ｽ 
			m_pTyabu->Draw(i);
		}
	}

	if(OnlyDice)
	{
		// ・ｽQ・ｽﾆ用・ｽﾌイ・ｽ・ｽ・ｽX・ｽ^・ｽ・ｽ・ｽX・ｽ・ｽ・ｽ謫ｾ
		TRAN_INS;

		if (m_pDice)
		{
			m_pDice->Draw();
		}

		if (m_role)
		{

			RoleResult r;
			int d1 = tran.dice.currentFaceNumber[0];
			int d2 = tran.dice.currentFaceNumber[2];
			int d3 = tran.dice.currentFaceNumber[3];
			r = CalcRole(d1, d2, d3);
			switch (r.role)
			{
			case RoleType::None:
				m_pRoleUI->SetTexture("Role/Role_None.png");
				m_pYukari->SetType(Yukari_Type::UnHappy);
				break;
			case RoleType::Hifumi:
				m_pRoleUI->SetTexture("Role/Role_Hifumi.png");
				m_pYukari->SetType(Yukari_Type::UnHappy);
				break;
			case RoleType::Shigoro:
				m_pRoleUI->SetTexture("Role/Role_Shigoro.png");
				m_pYukari->SetType(Yukari_Type::Happy);
				break;
			case RoleType::Zorome:
				m_pRoleUI->SetTexture("Role/Role_Zorome.png");
				m_pYukari->SetType(Yukari_Type::Happy);
				break;
			case RoleType::Pinzoro:
				m_pRoleUI->SetTexture("Role/Role_Pinzoro.png");
				m_pYukari->SetType(Yukari_Type::Happy);
				break;
			case RoleType::Me:
			{
				char path[64];
				sprintf_s(path, "Role/Role_Me%d.png", r.me);
				m_pRoleUI->SetTexture(path);
				m_pYukari->SetType(Yukari_Type::Happy);
				break;
			}
			default:
				// ・ｽ・ｽ・ｽﾈゑｿｽ・ｽﾈゑｿｽ\・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ or --- ・ｽﾉゑｿｽ・ｽ・ｽ
				break;
			}
			m_role->Draw();
		}
		if (m_pScore)
		{
			// m_pScore->Draw();
		}
		if (m_pRoleUI)
		{
			m_pRoleUI->Draw();
		}
		if (m_pTurnUI)
		{
			m_pTurnUI->Draw();
		}
		if (m_pMoneyUI)
		{
			//m_pMoneyUI->Draw();
		}
		if (m_pPlayerHp)
		{
			if (m_pPlayerHpIcon)
			{
				m_pPlayerHpIcon->Draw();
			}
			m_pPlayerHp->Draw();
		}
		if (m_pEnemyHp)
		{
			if (m_pEnemyHpIcon)
			{
				m_pEnemyHpIcon->Draw();
			}
			m_pEnemyHp->Draw();
		}
		if (m_pYukari && isUsedYukari)
		{
			m_pYukari->Draw();
		}
	}
}

























