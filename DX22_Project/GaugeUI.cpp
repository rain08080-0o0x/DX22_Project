#include "GaugeUI.h"
#include "DirectXMath.h"
#include "Defines.h"
#include "ShaderList.h"

GaugeUI::GaugeUI()
	: m_pFrameTex(nullptr)
	, m_pGaugeTex(nullptr)
	, m_rate(0.1f)
{
	//  
	m_pFrameTex = new Texture();
	if (FAILED(m_pFrameTex->Create("Assets/Texture/UIFrame.png"))) {
		MessageBox(NULL, "Texture load failed.¥nUI.cpp", "Error", MB_OK);
	}
	//  
	m_pGaugeTex = new Texture();
	if (FAILED(m_pGaugeTex->Create("Assets/Texture/UIGauge.png"))) {
		MessageBox(NULL, "Texture load failed.¥nUI.cpp", "Error", MB_OK);
	}
}

GaugeUI::~GaugeUI()
{
	if (m_pFrameTex) {
		delete m_pFrameTex;
		m_pFrameTex = nullptr;
	} if (m_pGaugeTex) {
		delete m_pGaugeTex;
		m_pGaugeTex = nullptr;
	}
}

void GaugeUI::Update()
{

}

void GaugeUI::Draw()
{
	// 2D 
	DirectX::XMFLOAT4X4 world, view, proj;
	DirectX::XMMATRIX mView = DirectX::XMMatrixLookAtLH(
		DirectX::XMVectorSet(0,0,-10,0),
		DirectX::XMVectorSet(0,0,0,0),
		DirectX::XMVectorSet(0,1,0,0)
	);
	DirectX::XMMATRIX mProj = DirectX::XMMatrixOrthographicOffCenterLH(
		0.0f,SCREEN_WIDTH,
		SCREEN_HEIGHT,0.0,
		1,
		100
	);
	DirectX::XMStoreFloat4x4(&view, mView);
	DirectX::XMStoreFloat4x4(&proj, mProj);

	//ShaderList::SetWVP(fWVP); // SetWVP関数の引数にはXMFLOAT4X4型で要素数３の配列のアドレスを渡す 
	//  
	Sprite::SetView(view);
	Sprite::SetProjection(proj);

	// (0) (1) 
	DirectX::XMFLOAT2 pos = { SCREEN_WIDTH * 0.5f,SCREEN_HEIGHT * 0.5f };
	DirectX::XMFLOAT2 size[] = { {256,66},{258,64} };
	Texture* pTexture[] = { m_pFrameTex,m_pGaugeTex };

	// (0) (1) 
	for (int i = 0; i < 2; ++i) {
		//  
		DirectX::XMMATRIX T =
			DirectX::XMMatrixTranslation(pos.x - size[i].x * 0.5f, pos.y, 0.0f);
		DirectX::XMMATRIX S;
		if (i == 0) //  
			S = DirectX::XMMatrixScaling(1.0f, -1.0f, 1.0f);
		else   // m_rate 
			S = DirectX::XMMatrixScaling(m_rate, -1.0f, 1.0f);
		DirectX::XMMATRIX mWorld = T * S;
		DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(mWorld));

			//  
			Sprite::SetWorld(world);      //  
		Sprite::SetSize(size[i]);      //  
		Sprite::SetOffset({ size[i].x * 0.5f, 0.0f }); //  
		Sprite::SetColor({ 1.0f,1.0f,1.0f,1.0f });
		Sprite::SetTexture(pTexture[i]);    //  
		Sprite::Draw();
	}
}

void GaugeUI::SetGauge(float rate)
{
	m_rate = rate;
}
