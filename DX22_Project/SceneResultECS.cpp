#include "SceneResult.h"
#include <memory>
#include "ECSComponents.h"
#include "ECSSystems.h"
#include "Defines.h"
#include "Input.h"

SceneResult::SceneResult()
{
	m_current = SceneManager::GetResultType();

	m_winner = m_world.CreateEntity();
	m_loser = m_world.CreateEntity();

	auto& win = m_world.Add<ECS::UIWidget>(m_winner, ECS::UIWidget{
		std::make_unique<UIObject>("result/winner.png", SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, SCREEN_WIDTH, SCREEN_HEIGHT)
		});
	auto& lose = m_world.Add<ECS::UIWidget>(m_loser, ECS::UIWidget{
		std::make_unique<UIObject>("result/loser.png", SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, SCREEN_WIDTH, SCREEN_HEIGHT)
		});

	win.visible = (m_current == SceneManager::Win);
	lose.visible = (m_current == SceneManager::Lose);
}

SceneResult::~SceneResult()
{
	m_world.Clear();
}

void SceneResult::Update()
{
	if (m_current == SceneManager::ResultType::None)
	{
		SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
		return;
	}

	if (IsKeyTrigger(VK_RETURN))
	{
		SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
	}
}

void SceneResult::Draw()
{
	ECSSystems::DrawUI(m_world);
}
