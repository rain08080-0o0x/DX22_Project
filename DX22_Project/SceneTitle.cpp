#include "SceneTitle.h"
#include "SceneManager.h"
#include "Input.h"
#include "UIObject.h"

#include "Defines.h"

SceneTitle::SceneTitle()
    : m_pLogo(nullptr)
    , m_pStart(nullptr)
    , m_pHint(nullptr)
{
    // 画像は Assets/Texture/ を UIObject 側が付ける前提なら相対でOK
    // 例: Assets/Texture/Title/Title_Logo.png を置いた場合は "Title/Title_Logo.png"

    m_pLogo = new UIObject("Title/Title.png", SCREEN_WIDTH * 0.5f, 210.0f, 900.0f, 380.0f / 2);
    m_pStart = new UIObject("Title/Btn_Start.png", SCREEN_WIDTH * 0.5f, 550.0f, 380.0f, 110.0f);
    m_pHint = new UIObject("Title/Title_Hint.png", SCREEN_WIDTH * 0.5f, 670.0f, 300.0f, 120.0f);
}

SceneTitle::~SceneTitle()
{
    delete m_pLogo;  m_pLogo = nullptr;
    delete m_pStart; m_pStart = nullptr;
    delete m_pHint;  m_pHint = nullptr;
}

void SceneTitle::Update()
{
    // Enter でゲームへ
    if (IsKeyTrigger(VK_RETURN))
    {
        SceneManager::ChangeScene(SceneManager::SCENE_GAME);
    }
}

void SceneTitle::Draw()
{
    if (m_pLogo)  m_pLogo->Draw();
    if (m_pStart) m_pStart->Draw();
    if (m_pHint)  m_pHint->Draw();
}
