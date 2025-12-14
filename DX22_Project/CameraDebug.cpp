#include "CameraDebug.h"

const float CameraSpeed = 0.1f;
const float CameraDebugRotate = 0.1f;
const float CameraDefaultDistance = -2.0f;

CameraDebug::CameraDebug()
	: m_radXZ(0.0f)
	, m_radY(0.50f)
	, m_radius(5.0f)
{

}

CameraDebug::~CameraDebug()
{

}

void CameraDebug::Update()
{
	if(true)
	{
		// 先生のやつ
		//--- 注視点の移動 
		// ↑(+z)に移動 
		if (IsKeyPress(VK_UP)) { m_look.z += sinf(atan2f(m_look.z - m_pos.z, m_look.x - m_pos.x)) * CameraSpeed; ; }
		if (IsKeyPress(VK_DOWN)) { m_look.z -= sinf(atan2f(m_look.z - m_pos.z, m_look.x - m_pos.x)) * CameraSpeed; }
		if (IsKeyPress(VK_RIGHT)) { m_look.x += cosf(atan2f(m_look.z - m_pos.z, m_look.x - m_pos.x)) * CameraSpeed;}
		if (IsKeyPress(VK_LEFT)) { m_look.x -= cosf(atan2f(m_look.z - m_pos.z, m_look.x - m_pos.x)) * CameraSpeed; }
		if (IsKeyPress(VK_SHIFT)) { m_look.y += CameraSpeed; }
		if (IsKeyPress(VK_CONTROL)) { m_look.y -= CameraSpeed; }
		//--- カメラ位置の移動 
		// 回り込み
		if (IsKeyPress('A')) { m_radXZ += CameraDebugRotate; }
		if (IsKeyPress('D')) { m_radXZ -= CameraDebugRotate; }
		if (IsKeyPress('W')) { m_radY -= CameraDebugRotate; }
		if (IsKeyPress('S')) { m_radY += CameraDebugRotate; }

		// --- カメラの距離
		if (IsKeyPress('E')) { m_radius += CameraSpeed; }
		if (IsKeyPress('Q')) { m_radius -= CameraSpeed; }

		// カメラの位置の計算
		m_pos.x = m_look.x + m_radius * cosf(m_radY) * sinf(m_radXZ);
		m_pos.y = m_look.y + m_radius * sinf(m_radY);
		m_pos.z = m_look.z + m_radius * cosf(m_radY) * cosf(m_radXZ);

	}
}

void CameraDebug::SetLook(DirectX::XMFLOAT3 set)
{
	m_look = set;
}
