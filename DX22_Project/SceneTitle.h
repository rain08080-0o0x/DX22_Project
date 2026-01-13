#pragma once
#include "Scene.h"

class UIObject;

class SceneTitle : public Scene
{
public:
    SceneTitle();
    ~SceneTitle();

    void Update() override;
    void Draw() override;

private:
    UIObject* m_pLogo;
    UIObject* m_pStart;
    UIObject* m_pHint;
};
