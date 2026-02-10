#pragma once
#include "Scene.h"

class SceneManager
{
public:
    enum SceneType
    {
        SCENE_TITLE = 0,
        SCENE_GAME,
        SCENE_RESULT,
        SCENE_3DEDITOR,
        SCENE_MAX
    };

    enum ResultType
    {
        None,
        Win,
        Lose,
    };

public:
    static void Init();
    static void Uninit();
    static void Update();
    static void Draw();

    static void ChangeScene(SceneType next);
    static SceneType GetCurrent() { return m_current; }

    static ResultType GetResultType();
    static void ChangeResult(ResultType set);

private:
    static void CreateScene(SceneType type);

private:
    static ResultType m_result;
    static Scene* m_pScene;
    static SceneType m_current;
    static SceneType m_next;
    static bool m_isChanging;
};
