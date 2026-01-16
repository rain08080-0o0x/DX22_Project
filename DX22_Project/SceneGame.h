#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "Scene.h"
#include"Model.h"
#include"Camera.h"
#include "Player.h"
#include "Block.h"
#include "GaugeUI.h"
#include "Dice.h"
#include "Collision.h"
#include "UIObject.h"
#include "ScoreLite.h"
#include "Yukari.h"

enum class RoleType
{
	None,
	Hifumi,
	Shigoro,
	Zorome,
	Pinzoro,
	Me,        // 通常役（2個同じ + 1個違う）
};

struct RoleResult
{
	RoleType role;
	int addScore;
	int me;   // RoleType::Me のときだけ 1～6、それ以外は 0
};
// 賭けシステム
enum class BetState
{
	WaitingBet,     // ベット選択待ち
	WaitingRoll,    // 次のロール入力待ち（R）
	Rolling,        // 物理で転がり中
	Result          // 勝敗確定表示中（次のベットへ）
};

class SceneGame : public Scene
{
public:
	SceneGame();
	~SceneGame();
	void Update() final;
	void Draw() final;
public:

private:
	Model* m_pModel;
	Camera* m_pCamera;
	Player* m_pPlayer;
	Block* m_pBlock;
	GaugeUI* m_pGaugeUI;

	Dice* m_pDice;
	bool OnlyDice = true;

	UIObject* m_role;	// 役を表示するやつ

private:
	ScoreLite* m_pScore = nullptr;
	bool m_scoredThisRoll = false;

	UIObject* m_pRoleUI = nullptr;
	bool m_roleFixedThisRoll = false; // スコアの多重加算をさせないようにする変数

	// 役一覧スライド用
	bool  m_roleListOpen;
	float m_roleListX;        // 現在X
	float m_roleListTargetX;  // 目標X
	float m_roleListY;        // 固定Y
	float m_roleListSpeed;    // 追従速度（大きいほど速い）


	int m_money = 200;       // 初期所持金（固定200でOK。後で定数化/JSON化）
	int m_bet = 0;           // 現在のベット額（5 or 10）
	int m_rollUsed = 0;      // 使った回数（0..3）
	BetState m_betState = BetState::WaitingBet;
	// 所持金表示
	ScoreLite* m_pMoneyUI = nullptr;

	Yukari* m_pYukari;

	bool isUsedYukari = false;
public:
	// ターン制
	enum class TurnOwner
	{
		Player,
		Enemy
	};

	enum class TurnPhase
	{
		TurnStart,     // ターン開始（初期化）
		Betting,       // ベット選択
		WaitingRoll,   // ロール入力待ち（敵は自動で進む）
		Rolling,       // 転がり中
		Resolve,       // 出目確定後の処理
		TurnEnd        // 次のターンへ
	};

private:

	void EndTurn();
	void BeginTurn(TurnOwner owner);

	void TurnUpdate();

	void TurnStart();     // ターン開始（初期化）
	void Betting();       // ベット選択
	void WaitingRoll();   // ロール入力待ち（敵は自動で進む）
	void Rolling();       // 転がり中
	void Resolve();       // 出目確定後の処理
	void TurnEnd();        // 次のターンへ

private:

	TurnOwner m_turnOwner = TurnOwner::Player;
	TurnPhase m_turnPhase = TurnPhase::TurnStart;

	TurnOwner m_nowOwner;
	TurnOwner m_oldOwner;

	int m_playerHP = 300;
	int m_enemyHP = 300;

	int m_damageThisTurn = 0;     // このターンのダメージ確定値（勝った時だけ入る）
	bool m_resultReady = false;   // IsStop後に1回だけResolveするためのフラグ

	// 敵AI用の簡単タイマー（自動でテンポ良くする）
	float m_enemyWaitSec = 0.0f;

	// Resolve 用に保持
	RoleResult m_cachedRole;

	ScoreLite* m_pPlayerHp;
	ScoreLite* m_pEnemyHp;
	UIObject* m_pPlayerUI;
	UIObject* m_pEnemyUI;
};

#endif // __SCENE_GAME_H__