#include "SceneGameECS.h"
#include <cmath>
#include <cstdio>
#include <memory>
#include <utility>
#include "CameraDebug.h"
#include "DirectX.h"
#include "Defines.h"
#include "Dice.h"
#include "ECSComponents.h"
#include "Geometory.h"
#include "Input.h"
#include "SceneManager.h"
#include "ShaderList.h"
#include "Sprite.h"
#include "Transfer.h"
#include "Yukari.h"

namespace
{
    const float kRolePanelWidth = 375.0f;
    const float kRolePanelHeight = 520.0f;
    const float kRolePanelOpenX = SCREEN_WIDTH - kRolePanelWidth * 0.5f - 20.0f;
    const float kRolePanelCloseX = SCREEN_WIDTH + kRolePanelWidth * 0.5f + 20.0f;
    const float kRolePanelLerpDt = 1.0f / 120.0f;
    const float kTurnIndicatorSize = 96.0f;
    const float kTurnIndicatorMargin = 20.0f;
    const float kHpDigitW = 48.0f;
    const float kHpDigitH = 64.0f;
    const float kHpDigitSpacing = 56.0f;
    const float kHpIconSize = 64.0f;
    const float kHpIconGap = 12.0f;
    const float kPlayerHpX = 140.0f;
    const float kPlayerHpY = 60.0f;
    const float kEnemyHpX = SCREEN_WIDTH - 140.0f;
    const float kEnemyHpY = 160.0f;
    const float kHpIconOffsetX = kHpDigitW * 0.5f + kHpIconGap + kHpIconSize * 0.5f;
}

enum class RoleType
{
    None,
    Hifumi,
    Shigoro,
    Zorome,
    Pinzoro,
    Me
};

struct RoleResult
{
    RoleType role;
    int addScore;
    int me;
};

enum class BetState
{
    WaitingBet,
    WaitingRoll,
    Rolling,
    Result
};

enum class TurnOwner
{
    Player,
    Enemy
};

enum class TurnPhase
{
    TurnStart,
    Betting,
    WaitingRoll,
    Rolling,
    Resolve,
    TurnEnd
};

struct RolePanelState
{
    bool open = false;
    float x = 0.0f;
    float targetX = 0.0f;
    float y = 0.0f;
    float speed = 14.0f;
};

struct GameState
{
    bool onlyDice = true;
    bool scoredThisRoll = false;
    bool roleFixedThisRoll = false;
    bool resultReady = false;
    bool isUsedYukari = false;

    int money = 200;
    int bet = 0;
    int rollUsed = 0;

    BetState betState = BetState::WaitingBet;
    TurnOwner turnOwner = TurnOwner::Player;
    TurnPhase turnPhase = TurnPhase::TurnStart;

    int playerHP = 300;
    int enemyHP = 300;

    int damageThisTurn = 0;
    float enemyWaitSec = 0.0f;

    RoleResult cachedRole{ RoleType::None, 0, 0 };

    ECS::Entity cameraEntity = ECS::kInvalidEntity;
    ECS::Entity diceEntity = ECS::kInvalidEntity;
    ECS::Entity rolePanelEntity = ECS::kInvalidEntity;
    ECS::Entity roleUIEntity = ECS::kInvalidEntity;
    ECS::Entity turnUIEntity = ECS::kInvalidEntity;
    ECS::Entity scoreEntity = ECS::kInvalidEntity;
    ECS::Entity moneyEntity = ECS::kInvalidEntity;
    ECS::Entity playerHpEntity = ECS::kInvalidEntity;
    ECS::Entity enemyHpEntity = ECS::kInvalidEntity;
    ECS::Entity playerHpIconEntity = ECS::kInvalidEntity;
    ECS::Entity enemyHpIconEntity = ECS::kInvalidEntity;
    ECS::Entity yukariEntity = ECS::kInvalidEntity;
};

static void Sort3(int& a, int& b, int& c)
{
    if (a > b) std::swap(a, b);
    if (b > c) std::swap(b, c);
    if (a > b) std::swap(a, b);
}

static RoleResult CalcRole(int d0, int d1, int d2)
{
    Sort3(d0, d1, d2);

    if (d0 == 1 && d1 == 1 && d2 == 1)
        return { RoleType::Pinzoro, 100, 0 };

    if (d0 == d1 && d1 == d2)
        return { RoleType::Zorome, 50 + d0 * 10, 0 };

    if (d0 == 4 && d1 == 5 && d2 == 6)
        return { RoleType::Shigoro, 30, 0 };

    if (d0 == 1 && d1 == 2 && d2 == 3)
        return { RoleType::Hifumi, -20, 0 };

    if (d0 == d1 && d1 != d2)
        return { RoleType::Me, d2, d2 };

    if (d0 != d1 && d1 == d2)
        return { RoleType::Me, d0, d0 };

    return { RoleType::None, 0, 0 };
}

static float StepSec60fps()
{
    return 1.0f / 60.0f;
}

static Camera* GetCamera(ECS::World& world, const GameState& state)
{
    auto* comp = world.TryGet<ECS::CameraComponent>(state.cameraEntity);
    return comp ? comp->camera.get() : nullptr;
}

static Dice* GetDice(ECS::World& world, const GameState& state)
{
    auto* comp = world.TryGet<ECS::DiceComponent>(state.diceEntity);
    return comp ? comp->dice.get() : nullptr;
}

static UIObject* GetUI(ECS::World& world, ECS::Entity entity)
{
    auto* comp = world.TryGet<ECS::UIWidget>(entity);
    return comp ? comp->ui.get() : nullptr;
}

static ScoreLite* GetScore(ECS::World& world, ECS::Entity entity)
{
    auto* comp = world.TryGet<ECS::ScoreWidget>(entity);
    return comp ? comp->score.get() : nullptr;
}

static Yukari* GetYukari(ECS::World& world, const GameState& state)
{
    auto* comp = world.TryGet<ECS::YukariComponent>(state.yukariEntity);
    return comp ? comp->yukari.get() : nullptr;
}

static void SetScore(ECS::World& world, ECS::Entity entity, int value)
{
    auto* score = GetScore(world, entity);
    if (score)
    {
        score->SetScore(value);
    }
}

static void BeginTurn(ECS::World& world, GameState& state, TurnOwner owner)
{
    state.turnOwner = owner;
    state.turnPhase = TurnPhase::Betting;

    state.bet = 0;
    state.rollUsed = 0;
    state.betState = BetState::WaitingBet;
    state.damageThisTurn = 0;
    state.resultReady = false;
    state.roleFixedThisRoll = false;
    state.scoredThisRoll = false;

    if (auto* roleUI = GetUI(world, state.roleUIEntity))
        roleUI->SetTexture("Role/Role_None.png");

    if (auto* turnUI = GetUI(world, state.turnUIEntity))
    {
        const char* turnTex = (owner == TurnOwner::Player) ? "Character/Player.png" : "Character/Enemy.png";
        turnUI->SetTexture(turnTex);
    }

    state.enemyWaitSec = (owner == TurnOwner::Enemy) ? 1.0f : 0.0f;
}

static void EndTurn(ECS::World& world, GameState& state)
{
    if (state.damageThisTurn > 0)
    {
        if (state.turnOwner == TurnOwner::Player)
        {
            state.enemyHP -= state.damageThisTurn;
            if (state.enemyHP < 0) state.enemyHP = 0;
        }
        else
        {
            state.playerHP -= state.damageThisTurn;
            if (state.playerHP < 0) state.playerHP = 0;
        }
    }

    SetScore(world, state.playerHpEntity, state.playerHP);
    SetScore(world, state.enemyHpEntity, state.enemyHP);

    TurnOwner next = (state.turnOwner == TurnOwner::Player) ? TurnOwner::Enemy : TurnOwner::Player;
    BeginTurn(world, state, next);
}

static void UpdateRoleUI(ECS::World& world, GameState& state, const RoleResult& result)
{
    auto* roleUI = GetUI(world, state.roleUIEntity);
    auto* yukari = GetYukari(world, state);

    switch (result.role)
    {
    case RoleType::None:
        if (roleUI) roleUI->SetTexture("Role/Role_None.png");
        if (yukari) yukari->SetType(Yukari_Type::UnHappy);
        break;
    case RoleType::Hifumi:
        if (roleUI) roleUI->SetTexture("Role/Role_Hifumi.png");
        if (yukari) yukari->SetType(Yukari_Type::UnHappy);
        break;
    case RoleType::Shigoro:
        if (roleUI) roleUI->SetTexture("Role/Role_Shigoro.png");
        if (yukari) yukari->SetType(Yukari_Type::Happy);
        break;
    case RoleType::Zorome:
        if (roleUI) roleUI->SetTexture("Role/Role_Zorome.png");
        if (yukari) yukari->SetType(Yukari_Type::Happy);
        break;
    case RoleType::Pinzoro:
        if (roleUI) roleUI->SetTexture("Role/Role_Pinzoro.png");
        if (yukari) yukari->SetType(Yukari_Type::Happy);
        break;
    case RoleType::Me:
    {
        if (roleUI)
        {
            char path[64];
            sprintf_s(path, "Role/Role_Me%d.png", result.me);
            roleUI->SetTexture(path);
        }
        if (yukari) yukari->SetType(Yukari_Type::Happy);
        break;
    }
    default:
        break;
    }
}

static void HandleDiceStop(ECS::World& world, GameState& state)
{
    auto* dice = GetDice(world, state);
    auto* camera = GetCamera(world, state);
    if (!dice)
        return;

    if (!dice->IsStop())
    {
        if (camera) camera->LockPos(false);
        return;
    }

    if (camera) camera->LockPos(true);

    if (state.betState != BetState::Rolling)
        return;

    if (state.roleFixedThisRoll)
        return;

    TRAN_INS;
    const int a = tran.dice.currentFaceNumber[0];
    const int b = tran.dice.currentFaceNumber[2];
    const int c = tran.dice.currentFaceNumber[3];
    if (a < 1 || a > 6 || b < 1 || b > 6 || c < 1 || c > 6)
        return;

    RoleResult result = CalcRole(a, b, c);

    if (auto* score = GetScore(world, state.scoreEntity))
        score->AddScore(result.addScore);

    UpdateRoleUI(world, state, result);

    state.cachedRole = result;
    state.resultReady = true;
    state.roleFixedThisRoll = true;
}

static void ApplyBetResult(ECS::World& world, GameState& state, const RoleResult& result)
{
    bool roundEnded = false;

    auto win = [&](int mult, int damageMult)
    {
        const int damage = state.bet * damageMult;
        if (state.turnOwner == TurnOwner::Player)
        {
            state.money = state.bet * mult;
            state.enemyHP -= state.money;
            if (state.enemyHP < 0) state.enemyHP = 0;
            SetScore(world, state.enemyHpEntity, state.enemyHP);
        }
        else
        {
            state.money = state.bet * mult;
            state.playerHP -= state.money;
            if (state.playerHP < 0) state.playerHP = 0;
            SetScore(world, state.playerHpEntity, state.playerHP);
        }

        state.bet = 0;
        state.rollUsed = 0;
        state.betState = BetState::WaitingBet;
        state.turnPhase = TurnPhase::TurnEnd;
        state.damageThisTurn = damage;
        roundEnded = true;
    };

    auto continueRoll = [&]()
    {
        state.betState = BetState::WaitingRoll;
        state.turnPhase = TurnPhase::WaitingRoll;
    };

    auto loseRound = [&]()
    {
        state.bet = 0;
        state.rollUsed = 0;
        state.betState = BetState::WaitingBet;
        state.turnPhase = TurnPhase::TurnEnd;
        state.damageThisTurn = 0;
        roundEnded = true;
    };

    if (result.role == RoleType::None)
    {
        if (state.rollUsed >= 3) loseRound();
        else continueRoll();
    }
    else if (result.role == RoleType::Hifumi)
    {
        loseRound();
    }
    else
    {
        int mult = 1;
        int damageMult = 0;
        switch (result.role)
        {
        case RoleType::Pinzoro: mult = 10; damageMult = 6; break;
        case RoleType::Zorome:  mult = 3;  damageMult = 4; break;
        case RoleType::Shigoro: mult = 2;  damageMult = 3; break;
        case RoleType::Me:      mult = 2;  damageMult = 2; break;
        default:                mult = 1;  damageMult = 1; break;
        }
        win(mult, damageMult);
    }

    if (roundEnded)
    {
        EndTurn(world, state);
    }
}

static void HandlePlayerAutoBet(ECS::World& world, GameState& state)
{
    if (state.turnOwner != TurnOwner::Player)
        return;

    if (state.betState != BetState::WaitingBet)
        return;

    const int kMinBet = 5;
    if (state.money < kMinBet)
        return;

    state.bet = kMinBet;
    state.rollUsed = 0;
    state.betState = BetState::WaitingRoll;
    state.turnPhase = TurnPhase::WaitingRoll;

    if (auto* roleUI = GetUI(world, state.roleUIEntity))
        roleUI->SetTexture("Role/Role_None.png");
}

static void HandleBetInput(ECS::World& world, GameState& state)
{
    if (state.betState != BetState::WaitingBet)
        return;

    int nextBet = 0;
    if (IsKeyTrigger('1')) nextBet = 5;
    if (IsKeyTrigger('2')) nextBet = 10;

    if (nextBet <= 0)
        return;

    if (state.money < nextBet)
        return;

    state.bet = nextBet;
    state.rollUsed = 0;
    state.betState = BetState::WaitingRoll;
    state.turnPhase = TurnPhase::WaitingRoll;

    if (auto* roleUI = GetUI(world, state.roleUIEntity))
        roleUI->SetTexture("Role/Role_None.png");
}

static void HandleRollInput(ECS::World& world, GameState& state)
{
    if (state.turnOwner != TurnOwner::Player)
        return;

    if (!IsKeyTrigger('R'))
        return;

    if (state.betState != BetState::WaitingRoll)
        return;

    auto* dice = GetDice(world, state);
    if (state.rollUsed < 3 && dice)
    {
        state.money -= state.bet;
        if (state.money < 0) state.money = 0;
        SetScore(world, state.moneyEntity, state.money);

        state.roleFixedThisRoll = false;
        state.scoredThisRoll = false;

        state.rollUsed++;
        state.betState = BetState::Rolling;
        state.turnPhase = TurnPhase::Rolling;

        dice->RollRandom(0);
        dice->RollRandom(2);
        dice->RollRandom(3);
    }

    if (auto* yukari = GetYukari(world, state))
        yukari->SetType(Yukari_Type::Think);
}

static void UpdateEnemyTurn(ECS::World& world, GameState& state)
{
    if (state.turnPhase == TurnPhase::Betting || state.turnPhase == TurnPhase::WaitingRoll)
    {
        if (state.rollUsed >= 3)
        {
            return;
        }

        state.enemyWaitSec -= StepSec60fps();
        if (state.enemyWaitSec > 0.0f)
            return;

        if (state.bet <= 0)
        {
            state.bet = 5;
        }

        state.turnPhase = TurnPhase::Rolling;
        state.betState = BetState::Rolling;
        state.roleFixedThisRoll = false;
        state.scoredThisRoll = false;
        state.rollUsed++;

        if (auto* roleUI = GetUI(world, state.roleUIEntity))
            roleUI->SetTexture("Role/Role_None.png");

        if (auto* dice = GetDice(world, state))
        {
            dice->RollRandom(0);
            dice->RollRandom(2);
            dice->RollRandom(3);
        }

        if (auto* yukari = GetYukari(world, state))
            yukari->SetType(Yukari_Type::Think);
    }

    HandleDiceStop(world, state);
}

static void UpdateRoleListPanel(ECS::World& world, GameState& state, float dt)
{
    auto* panelState = world.TryGet<RolePanelState>(state.rolePanelEntity);
    auto* panelUI = GetUI(world, state.rolePanelEntity);
    if (!panelState || !panelUI)
        return;

    if (IsKeyTrigger(VK_RSHIFT))
    {
        panelState->open = !panelState->open;
        panelState->targetX = panelState->open ? kRolePanelOpenX : kRolePanelCloseX;
    }

    const float t = 1.0f - expf(-panelState->speed * dt);
    panelState->x = panelState->x + (panelState->targetX - panelState->x) * t;

    panelUI->SetPosition(panelState->x, panelState->y);

    TRAN_INS;
    tran.diceui.role.pos = { panelState->x, panelState->y };
}

static void UpdateRolePreview(ECS::World& world, GameState& state)
{
    TRAN_INS;
    const int a = tran.dice.currentFaceNumber[0];
    const int b = tran.dice.currentFaceNumber[2];
    const int c = tran.dice.currentFaceNumber[3];
    if (a < 1 || a > 6 || b < 1 || b > 6 || c < 1 || c > 6)
        return;

    RoleResult result = CalcRole(a, b, c);
    UpdateRoleUI(world, state, result);
}

SceneGame::SceneGame()
{
    m_gameEntity = m_world.CreateEntity();
    auto& state = m_world.Add<GameState>(m_gameEntity);

    state.onlyDice = true;

    RenderTarget* pRTV = GetDefaultRTV();
    DepthStencil* pDSV = GetDefaultDSV();
    SetRenderTargets(1, &pRTV, pDSV);
    SetDepthTest(true);

    state.cameraEntity = m_world.CreateEntity();
    auto& cameraComp = m_world.Add<ECS::CameraComponent>(
        state.cameraEntity,
        ECS::CameraComponent{ std::make_unique<CameraDebug>() });

    state.diceEntity = m_world.CreateEntity();
    auto& diceComp = m_world.Add<ECS::DiceComponent>(
        state.diceEntity,
        ECS::DiceComponent{ std::make_unique<Dice>() });
    diceComp.dice->SetCamera(cameraComp.camera.get());

    state.scoreEntity = m_world.CreateEntity();
    auto& scoreComp = m_world.Add<ECS::ScoreWidget>(
        state.scoreEntity,
        ECS::ScoreWidget{ std::make_unique<ScoreLite>("Number/number.png", 360.0f, 40.0f, 48.0f, 64.0f, 56.0f) });
    if (scoreComp.score)
        scoreComp.score->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    scoreComp.visible = false;

    state.roleUIEntity = m_world.CreateEntity();
    auto& roleUI = m_world.Add<ECS::UIWidget>(
        state.roleUIEntity,
        ECS::UIWidget{ std::make_unique<UIObject>("Role/Role_None.png", 360.0f, 120.0f, 256.0f, 96.0f) });
    if (roleUI.ui)
        roleUI.ui->SetColor(1, 1, 1, 1);

    const float turnX = SCREEN_WIDTH - kTurnIndicatorMargin - kTurnIndicatorSize * 0.5f - 50.0f;
    const float turnY = kTurnIndicatorMargin + 20.0f;
    state.turnUIEntity = m_world.CreateEntity();
    auto& turnUI = m_world.Add<ECS::UIWidget>(
        state.turnUIEntity,
        ECS::UIWidget{ std::make_unique<UIObject>("Character/Player.png", turnX, turnY, kTurnIndicatorSize, kTurnIndicatorSize) });
    if (turnUI.ui)
        turnUI.ui->SetColor(1, 1, 1, 1);

    RolePanelState panelState;
    panelState.open = false;
    panelState.x = kRolePanelCloseX;
    panelState.targetX = kRolePanelCloseX;
    panelState.y = SCREEN_HEIGHT - kRolePanelHeight * 0.5f;
    panelState.speed = 14.0f;

    state.rolePanelEntity = m_world.CreateEntity();
    auto& rolePanelUI = m_world.Add<ECS::UIWidget>(
        state.rolePanelEntity,
        ECS::UIWidget{ std::make_unique<UIObject>("tintiro.png", panelState.x, panelState.y, kRolePanelWidth, kRolePanelHeight) });
    if (rolePanelUI.ui)
    {
        rolePanelUI.ui->SetPosition(panelState.x, panelState.y);
        rolePanelUI.ui->SetSize(kRolePanelWidth, kRolePanelHeight);
    }
    m_world.Add<RolePanelState>(state.rolePanelEntity, panelState);

    TRAN_INS;
    tran.diceui.role.pos = { panelState.x, panelState.y };
    tran.diceui.role.size = { kRolePanelWidth, kRolePanelHeight };

    state.money = 200;
    state.moneyEntity = m_world.CreateEntity();
    auto& moneyUI = m_world.Add<ECS::ScoreWidget>(
        state.moneyEntity,
        ECS::ScoreWidget{ std::make_unique<ScoreLite>("Number/number.png", 150.0f, 60.0f, 48.0f, 64.0f, 56.0f) });
    if (moneyUI.score)
        moneyUI.score->SetScore(state.money);
    moneyUI.visible = false;

    state.playerHpEntity = m_world.CreateEntity();
    auto& playerHp = m_world.Add<ECS::ScoreWidget>(
        state.playerHpEntity,
        ECS::ScoreWidget{ std::make_unique<ScoreLite>("Number/number.png", kPlayerHpX, kPlayerHpY, kHpDigitW, kHpDigitH, kHpDigitSpacing) });
    if (playerHp.score)
        playerHp.score->SetScore(state.playerHP);

    state.enemyHpEntity = m_world.CreateEntity();
    auto& enemyHp = m_world.Add<ECS::ScoreWidget>(
        state.enemyHpEntity,
        ECS::ScoreWidget{ std::make_unique<ScoreLite>("Number/number.png", kEnemyHpX, kEnemyHpY, kHpDigitW, kHpDigitH, kHpDigitSpacing) });
    if (enemyHp.score)
        enemyHp.score->SetScore(state.enemyHP);

    const float playerHpIconX = kPlayerHpX + kHpIconOffsetX;
    const float enemyHpIconX = kEnemyHpX + kHpIconOffsetX;
    state.playerHpIconEntity = m_world.CreateEntity();
    m_world.Add<ECS::UIWidget>(
        state.playerHpIconEntity,
        ECS::UIWidget{ std::make_unique<UIObject>("Character/Player.png", playerHpIconX, kPlayerHpY, kHpIconSize, kHpIconSize) });
    state.enemyHpIconEntity = m_world.CreateEntity();
    m_world.Add<ECS::UIWidget>(
        state.enemyHpIconEntity,
        ECS::UIWidget{ std::make_unique<UIObject>("Character/Enemy.png", enemyHpIconX, kEnemyHpY, kHpIconSize, kHpIconSize) });

    state.yukariEntity = m_world.CreateEntity();
    auto& yukariComp = m_world.Add<ECS::YukariComponent>(
        state.yukariEntity,
        ECS::YukariComponent{ std::make_unique<Yukari>() });
    yukariComp.visible = false;
    state.isUsedYukari = false;

    BeginTurn(m_world, state, TurnOwner::Player);
}

SceneGame::~SceneGame()
{
    m_world.Clear();
}

void SceneGame::Update()
{
    auto* state = m_world.TryGet<GameState>(m_gameEntity);
    if (!state)
        return;

    if (auto* camera = GetCamera(m_world, *state))
        camera->Update();

    if (!state->onlyDice)
    {
        UpdatePlayerMode();
        return;
    }

    if (IsKeyTrigger('O'))
    {
        state->playerHP -= 50;
        SetScore(m_world, state->playerHpEntity, state->playerHP);
    }
    if (IsKeyTrigger('P'))
    {
        state->enemyHP -= 50;
        SetScore(m_world, state->enemyHpEntity, state->enemyHP);
    }

    if (state->playerHP <= 0)
    {
        SceneManager::ChangeScene(SceneManager::SceneType::SCENE_RESULT);
        SceneManager::ChangeResult(SceneManager::ResultType::Lose);
        return;
    }

    if (state->enemyHP <= 0)
    {
        SceneManager::ChangeScene(SceneManager::SceneType::SCENE_RESULT);
        SceneManager::ChangeResult(SceneManager::ResultType::Win);
        return;
    }

    UpdateDiceMode();
}

void SceneGame::UpdatePlayerMode()
{
}

void SceneGame::UpdateDiceMode()
{
    auto* state = m_world.TryGet<GameState>(m_gameEntity);
    if (!state)
        return;

    if (auto* dice = GetDice(m_world, *state))
    {
        dice->Update(1);
        if (auto* camera = GetCamera(m_world, *state))
            dice->SetCamera(camera);
    }

    if (auto* yukari = GetYukari(m_world, *state))
        yukari->Update();

    if (state->turnOwner == TurnOwner::Enemy)
    {
        UpdateEnemyTurn(m_world, *state);
    }
    else
    {
        HandleDiceStop(m_world, *state);
        HandlePlayerAutoBet(m_world, *state);
        HandleBetInput(m_world, *state);
        HandleRollInput(m_world, *state);
    }

    if (state->resultReady)
    {
        ApplyBetResult(m_world, *state, state->cachedRole);
        state->resultReady = false;
    }

    UpdateRolePreview(m_world, *state);
    UpdateRoleListPanel(m_world, *state, kRolePanelLerpDt);

    if (IsKeyTrigger(VK_ESCAPE))
    {
        SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
    }
}

void SceneGame::Draw()
{
    auto* state = m_world.TryGet<GameState>(m_gameEntity);
    if (!state)
        return;

    DirectX::XMFLOAT4X4 fWVP[3];
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX proj;

    world = DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f);
    view = DirectX::XMMatrixLookAtLH(
        DirectX::XMVectorSet(0.0f, 1.5f, -2.0f, 0.0f),
        DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    proj = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(70.0f),
        16.0f / 9.0f,
        0.1f,
        100.0f);

    DirectX::XMStoreFloat4x4(&fWVP[0], DirectX::XMMatrixTranspose(world));
    DirectX::XMStoreFloat4x4(&fWVP[1], DirectX::XMMatrixTranspose(view));
    DirectX::XMStoreFloat4x4(&fWVP[2], DirectX::XMMatrixTranspose(proj));

    if (auto* camera = GetCamera(m_world, *state))
    {
        fWVP[1] = camera->GetViewMatrix();
        fWVP[2] = camera->GetProjectionMatrix();
        Geometory::SetView(fWVP[1]);
        Geometory::SetProjection(fWVP[2]);
        Sprite::SetView(camera->GetViewMatrix());
        Sprite::SetProjection(camera->GetProjectionMatrix());
    }

    ShaderList::SetWVP(fWVP);

    if (state->onlyDice)
    {
        if (auto* dice = GetDice(m_world, *state))
            dice->Draw();

        UIObject::Begin2D();

        if (auto* rolePanel = GetUI(m_world, state->rolePanelEntity))
            rolePanel->Draw();
        if (auto* roleUI = GetUI(m_world, state->roleUIEntity))
            roleUI->Draw();
        if (auto* turnUI = GetUI(m_world, state->turnUIEntity))
            turnUI->Draw();

        if (auto* playerIcon = GetUI(m_world, state->playerHpIconEntity))
            playerIcon->Draw();
        if (auto* playerHp = GetScore(m_world, state->playerHpEntity))
            playerHp->Draw();

        if (auto* enemyIcon = GetUI(m_world, state->enemyHpIconEntity))
            enemyIcon->Draw();
        if (auto* enemyHp = GetScore(m_world, state->enemyHpEntity))
            enemyHp->Draw();

        if (state->isUsedYukari)
        {
            if (auto* yukari = GetYukari(m_world, *state))
                yukari->Draw();
        }

        UIObject::End2D();
    }
}
