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
        using namespace DirectX;
		// 先生のやつ
		//--- 注視点の移動
		// ↑(+z)に移動 
        XMVECTOR pos = XMLoadFloat3(&m_pos);
        XMVECTOR look = XMLoadFloat3(&m_look);

        // 向いている方向（look - pos）
        XMVECTOR forward = look - pos;

        // 地面移動にしたいならY成分を潰す（XZ平面）
        forward = XMVectorSetY(forward, 0.0f);

        // ほぼゼロ長さ対策
        if (XMVectorGetX(XMVector3LengthSq(forward)) < 1e-8f)
            return;

        forward = XMVector3Normalize(forward);

        // ワールド上方向
        const XMVECTOR up = XMVectorSet(0, 1, 0, 0);

        // 右方向（左右が逆なら cross の順序を変える）
        XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));

        XMVECTOR delta = XMVectorZero();

        if (IsKeyPress(VK_UP))    delta += forward * CameraSpeed;
        if (IsKeyPress(VK_DOWN))  delta -= forward * CameraSpeed;
        if (IsKeyPress(VK_RIGHT)) delta += right * CameraSpeed;
        if (IsKeyPress(VK_LEFT))  delta -= right * CameraSpeed;

        // 上下移動（ワールドY）
        if (IsKeyPress(VK_SHIFT))   delta += up * CameraSpeed;
        if (IsKeyPress(VK_CONTROL)) delta -= up * CameraSpeed;

        // 視線を維持したまま平行移動：pos と look を同じだけ動かす
        pos += delta;
        look += delta;

        // m_posについては後でlook基準に動かすので消す
        //XMStoreFloat3(&m_pos, pos);
        XMStoreFloat3(&m_look, look);
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
