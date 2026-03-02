#include "SceneTitle.h"
#include "SceneManager.h"
#include "Input.h"
#include "Transfer.h"
#include "Main.h"
#include "UIObject.h"

#include "Defines.h"

namespace
{
    constexpr int kTitleMenuStart = 0;
    constexpr int kTitleMenuOption = 1;
    constexpr int kTitleMenuExit = 2;

    constexpr int kOptionRowMaster = 0;
    constexpr int kOptionRowBgm = 1;
    constexpr int kOptionRowSe = 2;
    constexpr int kOptionRowDisplay = 3;
    constexpr int kOptionRowBack = 4;
    constexpr int kOptionRowCount = 5;
    constexpr float kVolumeStep = 0.05f;
    constexpr int kDifficultyChoiceCount = 3;
    constexpr float kDifficultyFrameWidth = 760.0f;
    constexpr float kDifficultyFrameHeight = 500.0f;
    constexpr float kDifficultyButtonWidth = 420.0f;
    constexpr float kDifficultyButtonHeight = 92.0f;
    constexpr float kDifficultyBackWidth = 240.0f;
    constexpr float kDifficultyBackHeight = 68.0f;
    constexpr float kHoveredScale = 1.12f;
    constexpr float kDifficultyPanelOffsetY = kDifficultyFrameHeight * 0.5f;

    bool IsTitleConfirmTriggered()
    {
        return IsKeyTrigger(VK_RETURN) || IsKeyTrigger('F') || IsKeyTrigger(VK_SPACE);
    }

    int WrapIndex(int value, int count)
    {
        if (count <= 0) return 0;
        int wrapped = value % count;
        if (wrapped < 0) wrapped += count;
        return wrapped;
    }

    float ClampVolume(float value)
    {
        if (value < 0.0f) return 0.0f;
        if (value > 2.0f) return 2.0f;
        return value;
    }

    bool IsMouseOverUI(UIObject* pUI)
    {
        if (!pUI) return false;

        const POINT mousePos = GetMousePosition();
        const DirectX::XMFLOAT2 pos = pUI->GetPosition();
        const DirectX::XMFLOAT2 size = pUI->GetSize();
        const float left = pos.x - size.x * 0.5f;
        const float right = pos.x + size.x * 0.5f;
        const float top = pos.y;
        const float bottom = pos.y + size.y;
        return mousePos.x >= left && mousePos.x <= right && mousePos.y >= top && mousePos.y <= bottom;
    }

    void ApplyButtonVisual(UIObject* pUI, float baseWidth, float baseHeight, bool highlighted)
    {
        if (!pUI) return;

        const float scale = highlighted ? kHoveredScale : 1.0f;
        const float color = highlighted ? 1.0f : 0.8f;
        pUI->SetSize(baseWidth * scale, baseHeight * scale);
        pUI->SetColor(color, color, color, 1.0f);
    }
}

SceneTitle::SceneTitle()
    : m_pLogo(nullptr)
    , m_pStart(nullptr)
    , m_pOption(nullptr)
    , m_pHint(nullptr)
    , m_pDifficultyFrame(nullptr)
    , m_pDifficultyEasy(nullptr)
    , m_pDifficultyNormal(nullptr)
    , m_pDifficultyHard(nullptr)
    , m_pDifficultyBack(nullptr)
    , m_menuSelection(kTitleMenuStart)
    , m_isOptionOpen(false)
    , m_optionSelection(kOptionRowMaster)
    , m_isDifficultyOpen(false)
    , m_difficultySelection(1)
{
    // 画像は Assets/Texture/ を UIObject 側が付ける前提なら相対でOK
    // 例: Assets/Texture/Title/Title_Logo.png を置いた場合は "Title/Title_Logo.png"

    m_pLogo = new UIObject("Title/Title_Logo.png", SCREEN_WIDTH * 0.5f, 210.0f, 900.0f, 380.0f);
    m_pStart = new UIObject("Title/Btn_Start.png", SCREEN_WIDTH * 0.5f, 500.0f, 360.0f, 96.0f);
    m_pOption = new UIObject("Title/Option.png", SCREEN_WIDTH * 0.5f, 600.0f, 360.0f, 96.0f);
    m_pHint = new UIObject("Title/Title_Hint.png", SCREEN_WIDTH * 0.5f, 680.0f, 300.0f, 90.0f);
    m_pDifficultyFrame = new UIObject("Game/Frame.png", SCREEN_WIDTH * 0.5f, 200.0f + kDifficultyPanelOffsetY, kDifficultyFrameWidth, kDifficultyFrameHeight);
    m_pDifficultyEasy = new UIObject("Game/Easy.png", SCREEN_WIDTH * 0.5f, 290.0f, kDifficultyButtonWidth, kDifficultyButtonHeight);
    m_pDifficultyNormal = new UIObject("Game/Normal.png", SCREEN_WIDTH * 0.5f, 395.0f, kDifficultyButtonWidth, kDifficultyButtonHeight);
    m_pDifficultyHard = new UIObject("Game/Hard.png", SCREEN_WIDTH * 0.5f, 500.0f, kDifficultyButtonWidth, kDifficultyButtonHeight);
    m_pDifficultyBack = new UIObject("Game/Back.png", SCREEN_WIDTH * 0.5f, 622.0f, kDifficultyBackWidth, kDifficultyBackHeight);
    TRAN_INS;
    tran.gameplayDebug.titleOptionOpen = 0;
    tran.gameplayDebug.titleOptionSelection = 0;
    tran.gameplayDebug.titleOptionRequestClose = 0;
    m_difficultySelection = tran.NormalizeDifficultyPreset(tran.gameplayDebug.difficultyPreset);
    tran.gameplayDebug.titleDifficultyOpen = 0;
    tran.gameplayDebug.titleDifficultySelection = m_difficultySelection;
}

SceneTitle::~SceneTitle()
{
    delete m_pLogo;  m_pLogo = nullptr;
    delete m_pStart; m_pStart = nullptr;
    delete m_pOption; m_pOption = nullptr;
    delete m_pHint;  m_pHint = nullptr;
    delete m_pDifficultyFrame; m_pDifficultyFrame = nullptr;
    delete m_pDifficultyEasy; m_pDifficultyEasy = nullptr;
    delete m_pDifficultyNormal; m_pDifficultyNormal = nullptr;
    delete m_pDifficultyHard; m_pDifficultyHard = nullptr;
    delete m_pDifficultyBack; m_pDifficultyBack = nullptr;
}

void SceneTitle::Update()
{
    TRAN_INS;
    if (tran.gameplayDebug.titleOptionRequestClose != 0)
    {
        tran.gameplayDebug.titleOptionRequestClose = 0;
        if (m_isOptionOpen)
        {
            m_isOptionOpen = false;
            return;
        }
    }
    if (m_isDifficultyOpen)
    {
        tran.gameplayDebug.titleOptionOpen = 0;
        tran.gameplayDebug.titleOptionSelection = 0;
        tran.gameplayDebug.titleOptionRequestClose = 0;
        tran.gameplayDebug.titleDifficultyOpen = 1;
        tran.gameplayDebug.titleDifficultySelection = m_difficultySelection;

        auto closeDifficultyOverlay = [&]()
        {
            m_isDifficultyOpen = false;
            tran.gameplayDebug.titleDifficultyOpen = 0;
            tran.gameplayDebug.titleDifficultySelection = m_difficultySelection;
        };

        auto startGameWithDifficulty = [&](int difficulty)
        {
            const int selectedDifficulty = tran.NormalizeDifficultyPreset(difficulty);
            tran.ApplyDifficultyPreset(selectedDifficulty);
            tran.ResetRoguelikeUpgrade();
            tran.gameplayDebug.requestBossBattle = 0;
            tran.gameplayDebug.bossBattleActive = 0;
            tran.gameplayDebug.showBossResultTimer = 0;
            tran.gameplayDebug.runElapsedSec = 0.0f;
            tran.gameplayDebug.runRecordedSec = 0.0f;
            tran.gameplayDebug.runTimerRunning = 0;
            tran.gameplayDebug.titleDifficultyOpen = 0;
            tran.gameplayDebug.titleDifficultySelection = selectedDifficulty;
            m_isDifficultyOpen = false;
            SceneManager::ChangeScene(SceneManager::SCENE_GAME);
        };

        if (IsKeyTrigger(VK_ESCAPE))
        {
            closeDifficultyOverlay();
            return;
        }

        if (IsKeyTrigger(VK_UP) || IsKeyTrigger('W') || IsKeyTrigger(VK_LEFT) || IsKeyTrigger('A'))
        {
            m_difficultySelection = WrapIndex(m_difficultySelection - 1, kDifficultyChoiceCount);
        }
        if (IsKeyTrigger(VK_DOWN) || IsKeyTrigger('S') || IsKeyTrigger(VK_RIGHT) || IsKeyTrigger('D'))
        {
            m_difficultySelection = WrapIndex(m_difficultySelection + 1, kDifficultyChoiceCount);
        }

        const bool easyHovered = IsMouseOverUI(m_pDifficultyEasy);
        const bool normalHovered = IsMouseOverUI(m_pDifficultyNormal);
        const bool hardHovered = IsMouseOverUI(m_pDifficultyHard);
        const bool backHovered = IsMouseOverUI(m_pDifficultyBack);

        if (easyHovered)
        {
            m_difficultySelection = 0;
        }
        else if (normalHovered)
        {
            m_difficultySelection = 1;
        }
        else if (hardHovered)
        {
            m_difficultySelection = 2;
        }

        tran.gameplayDebug.titleDifficultySelection = m_difficultySelection;

        ApplyButtonVisual(m_pDifficultyEasy, kDifficultyButtonWidth, kDifficultyButtonHeight, m_difficultySelection == 0 || easyHovered);
        ApplyButtonVisual(m_pDifficultyNormal, kDifficultyButtonWidth, kDifficultyButtonHeight, m_difficultySelection == 1 || normalHovered);
        ApplyButtonVisual(m_pDifficultyHard, kDifficultyButtonWidth, kDifficultyButtonHeight, m_difficultySelection == 2 || hardHovered);
        ApplyButtonVisual(m_pDifficultyBack, kDifficultyBackWidth, kDifficultyBackHeight, backHovered);

        if (IsMouseLeftTrigger())
        {
            if (backHovered)
            {
                closeDifficultyOverlay();
                return;
            }
            if (easyHovered)
            {
                startGameWithDifficulty(0);
                return;
            }
            if (normalHovered)
            {
                startGameWithDifficulty(1);
                return;
            }
            if (hardHovered)
            {
                startGameWithDifficulty(2);
                return;
            }
        }

        if (IsTitleConfirmTriggered())
        {
            startGameWithDifficulty(m_difficultySelection);
            return;
        }
        return;
    }
    tran.gameplayDebug.titleDifficultyOpen = 0;
    tran.gameplayDebug.titleDifficultySelection = m_difficultySelection;
    if (m_isOptionOpen)
    {
        tran.gameplayDebug.titleOptionOpen = 1;
        tran.gameplayDebug.titleOptionSelection = m_optionSelection;

        if (IsKeyTrigger(VK_ESCAPE))
        {
            m_isOptionOpen = false;
            return;
        }

        if (IsKeyTrigger(VK_UP) || IsKeyTrigger('W'))
        {
            m_optionSelection = WrapIndex(m_optionSelection - 1, kOptionRowCount);
        }
        if (IsKeyTrigger(VK_DOWN) || IsKeyTrigger('S'))
        {
            m_optionSelection = WrapIndex(m_optionSelection + 1, kOptionRowCount);
        }

        const bool decrease = IsKeyTrigger(VK_LEFT) || IsKeyTrigger('A');
        const bool increase = IsKeyTrigger(VK_RIGHT) || IsKeyTrigger('D');
        if (decrease || increase)
        {
            const float delta = decrease ? -kVolumeStep : kVolumeStep;
            switch (m_optionSelection)
            {
            case kOptionRowMaster:
                tran.gameplay.volumeMaster = ClampVolume(tran.gameplay.volumeMaster + delta);
                break;
            case kOptionRowBgm:
                tran.gameplay.volumeBgm = ClampVolume(tran.gameplay.volumeBgm + delta);
                break;
            case kOptionRowSe:
                tran.gameplay.volumeSe = ClampVolume(tran.gameplay.volumeSe + delta);
                break;
            case kOptionRowDisplay:
                SetAppFullscreen(increase);
                break;
            default:
                break;
            }
        }

        if (IsTitleConfirmTriggered())
        {
            if (m_optionSelection == kOptionRowDisplay)
            {
                ToggleAppFullscreen();
            }
            else if (m_optionSelection == kOptionRowBack)
            {
                m_isOptionOpen = false;
            }
        }
        return;
    }
    tran.gameplayDebug.titleOptionOpen = 0;
    tran.gameplayDebug.titleOptionSelection = 0;
    tran.gameplayDebug.titleOptionRequestClose = 0;

    if (IsKeyTrigger(VK_ESCAPE))
    {
        PostQuitMessage(0);
        return;
    }

    if (IsKeyTrigger(VK_UP) || IsKeyTrigger('W') || IsKeyTrigger(VK_LEFT) || IsKeyTrigger('A'))
    {
        m_menuSelection = WrapIndex(m_menuSelection - 1, 3);
    }
    if (IsKeyTrigger(VK_DOWN) || IsKeyTrigger('S') || IsKeyTrigger(VK_RIGHT) || IsKeyTrigger('D'))
    {
        m_menuSelection = WrapIndex(m_menuSelection + 1, 3);
    }

    const bool startSelected = (m_menuSelection == kTitleMenuStart);
    const bool optionSelected = (m_menuSelection == kTitleMenuOption);
    const bool exitSelected = (m_menuSelection == kTitleMenuExit);
    const float selectedScale = 1.18f;

    if (m_pStart)
    {
        m_pStart->SetSize(startSelected ? 360.0f * selectedScale : 360.0f,
                          startSelected ? 96.0f * selectedScale : 96.0f);
        m_pStart->SetColor(startSelected ? 1.0f : 0.65f, startSelected ? 1.0f : 0.65f, startSelected ? 1.0f : 0.65f, 1.0f);
    }
    if (m_pOption)
    {
        m_pOption->SetSize(optionSelected ? 360.0f * selectedScale : 360.0f,
                           optionSelected ? 96.0f * selectedScale : 96.0f);
        m_pOption->SetColor(optionSelected ? 1.0f : 0.65f, optionSelected ? 1.0f : 0.65f, optionSelected ? 1.0f : 0.65f, 1.0f);
    }
    if (m_pHint)
    {
        m_pHint->SetSize(exitSelected ? 300.0f * selectedScale : 300.0f,
                         exitSelected ? 90.0f * selectedScale : 90.0f);
        m_pHint->SetColor(exitSelected ? 1.0f : 0.65f, exitSelected ? 1.0f : 0.65f, exitSelected ? 1.0f : 0.65f, 1.0f);
    }

    if (IsTitleConfirmTriggered())
    {
        if (startSelected)
        {
            m_isDifficultyOpen = true;
            m_difficultySelection = tran.NormalizeDifficultyPreset(tran.gameplayDebug.difficultyPreset);
            tran.gameplayDebug.titleDifficultyOpen = 1;
            tran.gameplayDebug.titleDifficultySelection = m_difficultySelection;
            return;
        }
        if (optionSelected)
        {
            m_isOptionOpen = true;
            m_optionSelection = kOptionRowMaster;
            tran.gameplayDebug.titleOptionOpen = 1;
            tran.gameplayDebug.titleOptionSelection = m_optionSelection;
            tran.gameplayDebug.titleOptionRequestClose = 0;
            return;
        }
        if (exitSelected)
        {
            PostQuitMessage(0);
            return;
        }
    }
}

void SceneTitle::Draw()
{
    UIObject::Begin2D();
    if (m_pLogo)  m_pLogo->Draw();
    if (m_pStart) m_pStart->Draw();
    if (m_pOption) m_pOption->Draw();
    if (m_pHint)  m_pHint->Draw();
    if (m_isDifficultyOpen)
    {
        if (m_pDifficultyFrame) m_pDifficultyFrame->Draw();
        if (m_pDifficultyEasy) m_pDifficultyEasy->Draw();
        if (m_pDifficultyNormal) m_pDifficultyNormal->Draw();
        if (m_pDifficultyHard) m_pDifficultyHard->Draw();
        if (m_pDifficultyBack) m_pDifficultyBack->Draw();
    }
    UIObject::End2D();
}
