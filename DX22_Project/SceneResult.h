#pragma once
#include "SceneManager.h"
#include "UIObject.h"

struct XAUDIO2_BUFFER;
struct IXAudio2SourceVoice;

class SceneResult :
    public Scene
{
public:
	SceneResult();
	~SceneResult();
	void Update() final;
	void Draw() final;
private:
	UIObject* m_pWinner;
	UIObject* m_pLoser;
	XAUDIO2_BUFFER* m_pResultBgm;
	IXAudio2SourceVoice* m_pResultBgmVoice;
	SceneManager::ResultType m_current;
};

