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
	Me,        // �ʏ���i2���� + 1�Ⴄ�j
};

struct RoleResult
{
	RoleType role;
	int addScore;
	int me;   // RoleType::Me �̂Ƃ����� 1�`6�A����ȊO�� 0
};
// �q���V�X�e��
enum class BetState
{
	WaitingBet,     // �x�b�g�I��҂�
	WaitingRoll,    // ���̃��[�����͑҂��iR�j
	Rolling,        // �����œ]���蒆
	Result          // ���s�m��\�����i���̃x�b�g�ցj
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
	Model* m_pTyawan;
	Camera* m_pCamera;
	Player* m_pPlayer;
	Block* m_pBlock;
	GaugeUI* m_pGaugeUI;

	Dice* m_pDice;
	bool OnlyDice = true;

	UIObject* m_role;	// ����\��������

private:
	ScoreLite* m_pScore = nullptr;
	bool m_scoredThisRoll = false;

	UIObject* m_pRoleUI = nullptr;
	bool m_roleFixedThisRoll = false;

	// ���ꗗ�X���C�h�p
	bool  m_roleListOpen;
	float m_roleListX;        // ����X
	float m_roleListTargetX;  // �ڕWX
	float m_roleListY;        // �Œ�Y
	float m_roleListSpeed;    // �Ǐ]���x�i�傫���قǑ����j


	int m_money = 200;       // �����������i�Œ�200��OK�B��Œ萔��/JSON���j
	int m_bet = 0;           // ���݂̃x�b�g�z�i5 or 10�j
	int m_rollUsed = 0;      // �g�����񐔁i0..3�j
	BetState m_betState = BetState::WaitingBet;
	// �������\��
	ScoreLite* m_pMoneyUI = nullptr;

	Yukari* m_pYukari;

	bool isUsedYukari = false;
public:
	// �^�[����
	enum class TurnOwner
	{
		Player,
		Enemy
	};


	void EndTurn();
	void BeginTurn(TurnOwner owner);

	// BetState handlers
	void UpdateBetFlow(TurnOwner owner);
	void HandleWaitingBet(TurnOwner owner);
	void HandleWaitingRoll(TurnOwner owner);
	void HandleRolling(TurnOwner owner);

	// Helpers
	void StartRoll(TurnOwner owner);
	bool TryResolveStoppedRoll(RoleResult& outRole);
	void ApplyRoleVisuals(const RoleResult& r);
	void ResolveBetOutcomeAndMaybeEndTurn(const RoleResult& r);

private:

	TurnOwner m_turnOwner = TurnOwner::Player;

	int m_playerHP = 300;
	int m_enemyHP = 300;

	int m_damageThisTurn = 0;     // ���̃^�[���̃_���[�W�m��l�i����������������j

	// �GAI�p�̊ȒP�^�C�}�[�i�����Ńe���|�ǂ�����j
	float m_enemyWaitSec = 0.0f;

	// Rolling中の「即停止」誤判定対策
	int   m_stopGuardFrames = 0;   // Roll直後は停止判定を無視
	int   m_stopStableFrames = 0;  // IsStop()==true が連続したフレーム数
	float m_rollElapsedSec = 0.0f; // Roll開始からの経過秒


	ScoreLite* m_pPlayerHp;
	ScoreLite* m_pEnemyHp;
};

#endif // __SCENE_GAME_H__