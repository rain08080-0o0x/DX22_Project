#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneGameECS.h"
#include "SceneResult.h"

Scene* SceneManager::m_pScene = nullptr;
SceneManager::SceneType SceneManager::m_current = SceneManager::SCENE_TITLE;
SceneManager::SceneType SceneManager::m_next = SceneManager::SCENE_TITLE;
SceneManager::ResultType SceneManager::m_result = SceneManager::None;
bool SceneManager::m_isChanging = false;

void SceneManager::Init()
{
    m_current = SCENE_TITLE;
    m_next = SCENE_TITLE;
    m_isChanging = false;
    m_result = None;
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
    case SCENE_RESULT:
        m_pScene = new SceneResult();
        break;
    default:
        m_pScene = new SceneTitle();
        break;
    }
}

void SceneManager::ChangeScene(SceneType next)
{
    // Ignore switching to the same scene.
    if (next == m_current) return;

    m_next = next;
    m_isChanging = true;
}

SceneManager::ResultType SceneManager::GetResultType()
{
    return m_result;
}

void SceneManager::ChangeResult(ResultType set)
{
    m_result = set;
}

void SceneManager::Update()
{
    if (m_isChanging)
    {
        // Immediate switch; add fades later if needed.
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
}
