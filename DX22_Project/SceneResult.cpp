#include "SceneResult.h"
#include "Defines.h"
#include "Input.h"

SceneResult::SceneResult()
{
	m_pWinner = new UIObject("result/winner.png",SCREEN_WIDTH / 2.0f,SCREEN_HEIGHT / 2.0f,SCREEN_WIDTH,SCREEN_HEIGHT);
	m_pLoser = new UIObject("result/loser.png",SCREEN_WIDTH / 2.0f,SCREEN_HEIGHT / 2.0f,SCREEN_WIDTH,SCREEN_HEIGHT);
	m_current = SceneManager::GetResultType();
}

SceneResult::~SceneResult()
{
	if (m_pWinner)
	{
		delete m_pWinner;
		m_pWinner = nullptr;
	}
	if (m_pLoser)
	{
		delete m_pLoser;
		m_pLoser = nullptr;
	}
}

void SceneResult::Update()
{
	if(m_current == SceneManager::ResultType::None)
		SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
	if (IsKeyTrigger(VK_RETURN))
		SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
}


void SceneResult::Draw()
{
	switch (m_current)
	{
	case SceneManager::Win:
		m_pWinner->Draw();
		break;
	case SceneManager::Lose:
		m_pLoser->Draw();
		break;
	default:
		return;
	}
}
