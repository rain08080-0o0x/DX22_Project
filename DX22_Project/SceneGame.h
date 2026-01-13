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
	bool OnlyDice;

	UIObject* m_role;	// 役を表示するやつ

private:
	ScoreLite* m_pScore = nullptr;
	bool m_scoredThisRoll = false;

	UIObject* m_pRoleUI = nullptr;
	bool m_roleFixedThisRoll = false;

	// 役一覧スライド用
	bool  m_roleListOpen;
	float m_roleListX;        // 現在X
	float m_roleListTargetX;  // 目標X
	float m_roleListY;        // 固定Y
	float m_roleListSpeed;    // 追従速度（大きいほど速い）


};

#endif // __SCENE_GAME_H__