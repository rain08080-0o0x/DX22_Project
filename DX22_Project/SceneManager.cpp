#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneGame.h"

Scene* SceneManager::m_pScene = nullptr;
SceneManager::SceneType SceneManager::m_current = SceneManager::SCENE_TITLE;
SceneManager::SceneType SceneManager::m_next = SceneManager::SCENE_TITLE;
bool SceneManager::m_isChanging = false;

void SceneManager::Init()
{
    m_current = SCENE_TITLE;
    m_next = SCENE_TITLE;
    m_isChanging = false;

    CreateScene(m_current);
}

void SceneManager::Uninit()
{
    if (m_pScene)
    {
        delete m_pScene;
        m_pScene = nullptr;
    }
}

void SceneManager::CreateScene(SceneType type)
{
    if (m_pScene)
    {
        delete m_pScene;
        m_pScene = nullptr;
    }

    switch (type)
    {
    case SCENE_TITLE:
        m_pScene = new SceneTitle();
        break;
    case SCENE_GAME:
        m_pScene = new SceneGame();
        break;
    default:
        m_pScene = new SceneTitle();
        break;
    }
}

void SceneManager::ChangeScene(SceneType next)
{
    // 同じシーンに変えるのは無視
    if (next == m_current) return;

    m_next = next;
    m_isChanging = true;
}

void SceneManager::Update()
{
    if (m_isChanging)
    {
        // ここにフェード等を入れたいなら後で追加できる
        // 今は即切り替え
        m_current = m_next;
        CreateScene(m_current);
        m_isChanging = false;
    }

    if (m_pScene)
        m_pScene->RootUpdate();
}

void SceneManager::Draw()
{
    if (m_pScene)
        m_pScene->RootDraw();

    // フェード等を入れるなら、ここで上描き
}
