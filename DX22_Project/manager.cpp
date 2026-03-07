#include "manager.h"
#include "Geometory.h"
#include "Sprite.h"
#include "Input.h"
#include "ShaderList.h"
#include "SceneGame.h"

Scene* Manager::m_pScene{};

void Manager::Init()
{
	// ‘¼‹@”\‰Šú‰»
	Geometory::Init();
	Sprite::Init();
	InitInput();
	ShaderList::Init();

	m_pScene = new SceneGame();
	m_pScene->InitBase();
}

void Manager::Uninit()
{
	m_pScene->UninitBase();
	delete m_pScene;

	UninitInput();
	Sprite::Uninit();
	ShaderList::Uninit();
	Geometory::Uninit();
}

void Manager::Update()
{
	UpdateInput();

	m_pScene->UpdateBase();
}

void Manager::Draw()
{
	m_pScene->DrawBase();
}
