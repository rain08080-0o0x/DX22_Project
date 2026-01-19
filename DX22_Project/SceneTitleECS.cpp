#include "SceneTitle.h"
#include <memory>
#include "ECSComponents.h"
#include "ECSSystems.h"
#include "SceneManager.h"
#include "Input.h"
#include "Defines.h"

SceneTitle::SceneTitle()
{
    m_logo = m_world.CreateEntity();
    m_start = m_world.CreateEntity();
    m_hint = m_world.CreateEntity();

    m_world.Add<ECS::UIWidget>(m_logo, ECS::UIWidget{
        std::make_unique<UIObject>("Title/Title.png", SCREEN_WIDTH * 0.5f, 210.0f, 900.0f, 380.0f / 2)
        });
    m_world.Add<ECS::UIWidget>(m_start, ECS::UIWidget{
        std::make_unique<UIObject>("Title/Btn_Start.png", SCREEN_WIDTH * 0.5f, 550.0f, 380.0f, 110.0f)
        });
    m_world.Add<ECS::UIWidget>(m_hint, ECS::UIWidget{
        std::make_unique<UIObject>("Title/Title_Hint.png", SCREEN_WIDTH * 0.5f, 670.0f, 300.0f, 120.0f)
        });
}

SceneTitle::~SceneTitle()
{
    m_world.Clear();
}

void SceneTitle::Update()
{
    if (IsKeyTrigger(VK_RETURN))
    {
        SceneManager::ChangeScene(SceneManager::SCENE_GAME);
    }
}

void SceneTitle::Draw()
{
    ECSSystems::DrawUI(m_world);
}
