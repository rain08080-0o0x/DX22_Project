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
    UIObject* m_pOption;
    UIObject* m_pHint;
    UIObject* m_pDifficultyFrame;
    UIObject* m_pDifficultyEasy;
    UIObject* m_pDifficultyNormal;
    UIObject* m_pDifficultyHard;
    UIObject* m_pDifficultyBack;
    int m_menuSelection;
    bool m_isOptionOpen;
    int m_optionSelection;
    bool m_isDifficultyOpen;
    int m_difficultySelection;
};
