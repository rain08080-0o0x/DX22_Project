#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "Scene.h"
#include"Model.h"
#include"Camera.h"
#include "Player.h"
#include "Block.h"
#include "Dice.h"

class SceneGame : public Scene
{
public:
	SceneGame();
	~SceneGame();
	void Update() final;
	void Draw() final;

	void MoveAllDice(); 
private:
	void ResolveDiceCollisions();
	void ResolveDicePair(Dice& a, Dice& b);

	void ResolveDicePosition(Dice& a, Dice& b, const Collision::Manifold& m);
	void ResolveDiceVelocity(Dice& a, Dice& b, const Collision::Manifold& m);
	void ResolveDiceAngular(Dice& a, Dice& b, const Collision::Manifold& m);


private:
	Model* m_pModel;
	Camera* m_pCamera;
	Player* m_pPlayer;
	Block* m_pBlock;
	int m_diceCount;
	Dice *m_dice;
};

#endif // __SCENE_GAME_H__