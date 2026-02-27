#include "SceneResult.h"
#include "Defines.h"
#include "Input.h"
#include "Sound.h"
#include "Transfer.h"

namespace
{
	constexpr int kMenuRestart = 0;
	constexpr int kMenuTitle = 1;

	bool IsResultConfirmTriggered()
	{
		return IsKeyTrigger(VK_RETURN) || IsKeyTrigger('F') || IsKeyTrigger(VK_SPACE);
	}

	bool IsBossStartTriggered()
	{
		return IsKeyTrigger('B');
	}
}

SceneResult::SceneResult()
{
	m_pWinner = new UIObject("result/winner.png", SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
	m_pLoser = new UIObject("result/loser.png", SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
	m_pMenuRestart = new UIObject("result/Restart.png", SCREEN_WIDTH * 0.33f, SCREEN_HEIGHT * 0.80f, 380.0f, 120.0f);
	m_pMenuTitle = new UIObject("result/Title.png", SCREEN_WIDTH * 0.67f, SCREEN_HEIGHT * 0.80f, 380.0f, 120.0f);
	m_pResultBgm = LoadSound("Assets/Sound/BGM/ResultBGM.mp3", true);
	m_pResultBgmVoice = nullptr;
	m_menuSelection = kMenuRestart;
	if (m_pResultBgm)
	{
		m_pResultBgmVoice = PlaySound(m_pResultBgm);
	}
	m_current = SceneManager::GetResultType();
}

SceneResult::~SceneResult()
{
	if (m_pResultBgmVoice)
	{
		m_pResultBgmVoice->Stop();
		m_pResultBgmVoice->DestroyVoice();
		m_pResultBgmVoice = nullptr;
	}
	m_pResultBgm = nullptr;

	if (m_pWinner)
	{
		delete m_pWinner;
		m_pWinner = nullptr;
	}
	if (m_pLoser)
	{
		delete m_pLoser;
		m_pLoser = nullptr;
	}
	if (m_pMenuRestart)
	{
		delete m_pMenuRestart;
		m_pMenuRestart = nullptr;
	}
	if (m_pMenuTitle)
	{
		delete m_pMenuTitle;
		m_pMenuTitle = nullptr;
	}
}

void SceneResult::Update()
{
	TRAN_INS;
	tran.gameplayDebug.runTimerRunning = 0;
	if (m_pResultBgmVoice)
	{
		float masterVolume = tran.gameplay.volumeMaster;
		if (masterVolume < 0.0f) masterVolume = 0.0f;
		if (masterVolume > 2.0f) masterVolume = 2.0f;
		float bgmVolume = tran.gameplay.volumeBgm;
		if (bgmVolume < 0.0f) bgmVolume = 0.0f;
		if (bgmVolume > 2.0f) bgmVolume = 2.0f;
		m_pResultBgmVoice->SetVolume(masterVolume * bgmVolume);
	}

	if (m_current == SceneManager::ResultType::None)
	{
		SceneManager::ChangeResult(SceneManager::ResultType::None);
		SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
		return;
	}
	if (m_current == SceneManager::ResultType::Win && tran.roguelike.selectionPending != 0)
	{
		if (IsBossStartTriggered())
		{
			tran.roguelike.selectionPending = 0;
			tran.roguelike.rerollRemain = 0;
			tran.gameplayDebug.upgradeSelectionPending = 0;
			tran.gameplayDebug.upgradeRerollRemain = 0;
			tran.gameplayDebug.upgradeOffer0 = -1;
			tran.gameplayDebug.upgradeOffer1 = -1;
			tran.gameplayDebug.upgradeOffer2 = -1;
			tran.gameplayDebug.requestBossBattle = 1;
			tran.gameplayDebug.bossBattleActive = 0;
			tran.gameplayDebug.showBossResultTimer = 0;
			SceneManager::ChangeResult(SceneManager::ResultType::None);
			SceneManager::ChangeScene(SceneManager::SCENE_GAME);
			return;
		}

		const bool hasAnyOffer =
			(tran.roguelike.offers[0] >= 0) ||
			(tran.roguelike.offers[1] >= 0) ||
			(tran.roguelike.offers[2] >= 0);
		if (!hasAnyOffer)
		{
			if (IsResultConfirmTriggered())
			{
				tran.roguelike.selectionPending = 0;
				tran.roguelike.rerollRemain = 0;
				tran.gameplayDebug.upgradeSelectionPending = 0;
				tran.gameplayDebug.upgradeRerollRemain = 0;
				tran.gameplayDebug.upgradeOffer0 = -1;
				tran.gameplayDebug.upgradeOffer1 = -1;
				tran.gameplayDebug.upgradeOffer2 = -1;
			}
			return;
		}
		if (IsKeyTrigger('R'))
		{
			tran.RerollUpgradeSelection();
		}

		int selectedIndex = -1;
		if (IsKeyTrigger('1')) selectedIndex = 0;
		else if (IsKeyTrigger('2')) selectedIndex = 1;
		else if (IsKeyTrigger('3')) selectedIndex = 2;

		if (selectedIndex >= 0 && tran.ApplyUpgradeSelection(selectedIndex))
		{
			SceneManager::ChangeResult(SceneManager::ResultType::None);
			SceneManager::ChangeScene(SceneManager::SCENE_GAME);
			return;
		}
		return;
	}

	if (IsKeyTrigger(VK_LEFT) || IsKeyTrigger('A') || IsKeyTrigger(VK_UP) || IsKeyTrigger('W'))
	{
		m_menuSelection = kMenuRestart;
	}
	if (IsKeyTrigger(VK_RIGHT) || IsKeyTrigger('D') || IsKeyTrigger(VK_DOWN) || IsKeyTrigger('S'))
	{
		m_menuSelection = kMenuTitle;
	}

	const bool restartSelected = (m_menuSelection == kMenuRestart);
	const float selectedScale = 1.18f;
	if (m_pMenuRestart)
	{
		m_pMenuRestart->SetSize(restartSelected ? 380.0f * selectedScale : 380.0f,
			restartSelected ? 120.0f * selectedScale : 120.0f);
		m_pMenuRestart->SetColor(restartSelected ? 1.0f : 0.65f, restartSelected ? 1.0f : 0.65f, restartSelected ? 1.0f : 0.65f, 1.0f);
	}
	if (m_pMenuTitle)
	{
		m_pMenuTitle->SetSize(!restartSelected ? 380.0f * selectedScale : 380.0f,
			!restartSelected ? 120.0f * selectedScale : 120.0f);
		m_pMenuTitle->SetColor(!restartSelected ? 1.0f : 0.65f, !restartSelected ? 1.0f : 0.65f, !restartSelected ? 1.0f : 0.65f, 1.0f);
	}

	if (IsResultConfirmTriggered())
	{
		if (restartSelected)
		{
			tran.ResetRoguelikeUpgrade();
			tran.gameplayDebug.requestBossBattle = 0;
			tran.gameplayDebug.bossBattleActive = 0;
			tran.gameplayDebug.showBossResultTimer = 0;
			tran.gameplayDebug.runElapsedSec = 0.0f;
			tran.gameplayDebug.runRecordedSec = 0.0f;
			tran.gameplayDebug.runTimerRunning = 0;
			SceneManager::ChangeResult(SceneManager::ResultType::None);
			SceneManager::ChangeScene(SceneManager::SCENE_GAME);
		}
		else
		{
			tran.gameplayDebug.requestBossBattle = 0;
			tran.gameplayDebug.bossBattleActive = 0;
			tran.gameplayDebug.showBossResultTimer = 0;
			tran.gameplayDebug.runTimerRunning = 0;
			SceneManager::ChangeResult(SceneManager::ResultType::None);
			SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
		}
		return;
	}
}

void SceneResult::Draw()
{
	switch (m_current)
	{
	case SceneManager::Win:
		m_pWinner->Draw();
		break;
	case SceneManager::Lose:
		m_pLoser->Draw();
		break;
	default:
		return;
	}

	TRAN_INS;
	if (m_current == SceneManager::ResultType::Win && tran.roguelike.selectionPending != 0)
	{
		return;
	}

	if (m_pMenuRestart) m_pMenuRestart->Draw();
	if (m_pMenuTitle) m_pMenuTitle->Draw();
}
