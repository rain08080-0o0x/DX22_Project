#include "SceneResult.h"
#include "Defines.h"
#include "Input.h"
#include "Sound.h"
#include "Transfer.h"

SceneResult::SceneResult()
{
	m_pWinner = new UIObject("result/winner.png", SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
	m_pLoser = new UIObject("result/loser.png", SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
	m_pResultBgm = LoadSound("Assets/Sound/BGM/ResultBGM.mp3", true);
	m_pResultBgmVoice = nullptr;
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
}

void SceneResult::Update()
{
	TRAN_INS;
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
	if (IsKeyTrigger(VK_RETURN))
	{
		SceneManager::ChangeResult(SceneManager::ResultType::None);
		SceneManager::ChangeScene(SceneManager::SCENE_GAME);
		return;
	}
	if (IsKeyTrigger('T'))
	{
		SceneManager::ChangeResult(SceneManager::ResultType::None);
		SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
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
}
