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
}

SceneTitle::SceneTitle()
    : m_pLogo(nullptr)
    , m_pStart(nullptr)
    , m_pOption(nullptr)
    , m_pHint(nullptr)
    , m_menuSelection(kTitleMenuStart)
    , m_isOptionOpen(false)
    , m_optionSelection(kOptionRowMaster)
{
    // 画像は Assets/Texture/ を UIObject 側が付ける前提なら相対でOK
    // 例: Assets/Texture/Title/Title_Logo.png を置いた場合は "Title/Title_Logo.png"

    m_pLogo = new UIObject("Title/Title_Logo.png", SCREEN_WIDTH * 0.5f, 210.0f, 900.0f, 380.0f);
    m_pStart = new UIObject("Title/Btn_Start.png", SCREEN_WIDTH * 0.5f, 500.0f, 360.0f, 96.0f);
    m_pOption = new UIObject("Title/Option.png", SCREEN_WIDTH * 0.5f, 600.0f, 360.0f, 96.0f);
    m_pHint = new UIObject("Title/Title_Hint.png", SCREEN_WIDTH * 0.5f, 680.0f, 300.0f, 90.0f);
    TRAN_INS;
    tran.gameplayDebug.titleOptionOpen = 0;
    tran.gameplayDebug.titleOptionSelection = 0;
    tran.gameplayDebug.titleOptionRequestClose = 0;
}

SceneTitle::~SceneTitle()
{
    delete m_pLogo;  m_pLogo = nullptr;
    delete m_pStart; m_pStart = nullptr;
    delete m_pOption; m_pOption = nullptr;
    delete m_pHint;  m_pHint = nullptr;
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
            TRAN_INS;
            tran.ResetRoguelikeUpgrade();
            tran.gameplayDebug.requestBossBattle = 0;
            tran.gameplayDebug.bossBattleActive = 0;
            tran.gameplayDebug.showBossResultTimer = 0;
            tran.gameplayDebug.runElapsedSec = 0.0f;
            tran.gameplayDebug.runRecordedSec = 0.0f;
            tran.gameplayDebug.runTimerRunning = 0;
            SceneManager::ChangeScene(SceneManager::SCENE_GAME);
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
    if (m_pLogo)  m_pLogo->Draw();
    if (m_pStart) m_pStart->Draw();
    if (m_pOption) m_pOption->Draw();
    if (m_pHint)  m_pHint->Draw();
}
