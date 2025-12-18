#include "Player.h"
#include "Geometory.h"
#include "Input.h"
#include "Transfer.h"


Player::Player()
    : m_pCamera(nullptr)
    , m_move()
    , m_isStop(true)
    , m_isGround(true)
    , m_shotStep(0)
    , m_shotPower(0.0f)
    , m_shotstep(SHOT_WAIT)
{
    m_pos.x = 0.0f;
    m_pos.y = 0.0f;
    m_pos.z = 0.0f;
    m_collision.size = DirectX::XMFLOAT3(1.0f,1.0f,1.0f);
    m_collision.center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

    m_pShadowTex = new Texture();
    if (FAILED(m_pShadowTex->Create("Assets/Texture/shadow.png")))
    {
        MessageBox(NULL, "Texture load failed.¥nPlayer.cpp", "Error", MB_OK);
    }
}

Player::~Player()
{
    if (m_pShadowTex) {
        delete m_pShadowTex;
        m_pShadowTex = nullptr;
    }
}

void Player::Update()
{
    // カメラが設定されてない場合は処理しない 
    if (!m_pCamera) { return; }
    // ボールが停止しているかどうかによって処理を変える 
    if (m_isStop)
        UpdateShot(); // 球を打つ処理 
    else
        UpdateMove(); // 打った球の移動処理 
    m_collision.center = m_pos;
}

void Player::Draw() 
{
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(m_pos.x,m_pos.y,m_pos.z);
    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(
        m_collision.size.x,
        m_collision.size.y,
        m_collision.size.z
    );
    DirectX::XMFLOAT4X4 mat;
    DirectX::XMStoreFloat4x4(&mat, DirectX::XMMatrixTranspose(S * T));
    Geometory::SetWorld(mat);
    Geometory::DrawBox();

    // 影の大きさを計算 
    float rate = (m_pos.y - m_shadowPos.y) / 4.0f; // 距離が近ければ0,遠ければ1 
    float scale = (1.0f - rate);      // rateを0なら1、1なら0になるよう反転 

    // 影を表示するための行列計算 
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(90));
    S = DirectX::XMMatrixScaling(scale,scale,scale);
    T = DirectX::XMMatrixTranslation(m_shadowPos.x,m_shadowPos.y,m_shadowPos.z);
    DirectX::XMMATRIX mWorld = S * R * T;
    DirectX::XMFLOAT4X4 fMat;
    DirectX::XMStoreFloat4x4(&fMat, DirectX::XMMatrixTranspose(mWorld));

    // 影の表示     
    Sprite::SetWorld(fMat);
    Sprite::SetSize({ 3.0f,3.0f });
    Sprite::SetColor({ 0.0f, 0.0f, 0.0f, scale * 0.8f }); // 地面との距離に応じて影の透明度を設定 
    Sprite::SetTexture(m_pShadowTex);
    Sprite::Draw();
}

void Player::SetCamera(Camera* pCamera)
{ 
    m_pCamera = pCamera; 
}

Collision::Box Player::GetCollision()
{
    return m_collision;
}

void Player::SetShadowPos(DirectX::XMFLOAT3 pos)
{
    m_shadowPos = pos;
}

Collision::Box Player::GetShadowCollision()
{
    return m_shadowCollision;
}

float Player::GetPower()
{
    return m_shotPower;
}

void Player::Bound(BoundAxis axis)
{
    // 接触方向に応じてめり込み解消 
    switch (axis) {
    case BoundX: m_pos.x -= m_move.x; break;
    case BoundY: m_pos.y -= m_move.y; break;
    case BoundZ: m_pos.z -= m_move.z; break;
    }
    // 接触方向に応じた摩擦を設定（数値は適当） 
    DirectX::XMFLOAT3 friction; // 摩擦 
    switch (axis)
    {
    case BoundX: friction = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f); break;
    case BoundY: friction = DirectX::XMFLOAT3(1.0f, 0.7f, 0.95f); break;
    case BoundZ: friction = DirectX::XMFLOAT3(1.0f, 1.0f, 0.95f); break;
    }

    // 接触時の減速 
    m_move.x *= friction.x;
    m_move.y *= friction.y;
    m_move.z *= friction.z;

    // 移動方向の反転 
    switch (axis) {
    case BoundX: m_move.x = -m_move.x; break;
    case BoundY: m_move.y = -m_move.y; break;
    case BoundZ: m_move.z = -m_move.z; break;
    }

    // 地面接地判定 
    if (0.0f < m_move.y && m_move.y < 0.05f) 
    {
        m_move.y = 0.0f; // 転がっているはずなのでYの移動を0にする 
        m_isGround = true; // 地面にいる状態とみなす 
    }
}

bool Player::CheckStop()
{
    float speed;
    DirectX::XMVECTOR vMove = DirectX::XMLoadFloat3(&m_move);
    DirectX::XMVECTOR vLen = DirectX::XMVector3Length(vMove);
    DirectX::XMStoreFloat(&speed, vLen);
    // 地面にいて移動スピードが一定値を下回ったら停止とみなす 
    return m_isGround && speed < 0.5f;
}

void Player::UpdateShot()
{
    TRAN_INS;
    switch (m_shotstep)
    {
    case Player::SHOT_WAIT:
        if (IsKeyTrigger('Z'))
        {
            m_shotPower = 0.0f;   // パワーを０にリセット z
            m_shotstep = SHOT_KEEP;  // パワーを溜める手順に変更 
        }
        break;
    case Player::SHOT_KEEP:
        m_shotPower += 0.02f; // パワーを溜め続ける 

        // パワーが上限を超えないように判定 
        if (m_shotPower > 1.0f)
            m_shotPower = 1.0f;

        // キー入力を止めたら球を打つ手順に変更 
        if (IsKeyRelease('Z')) {
            m_shotstep = SHOT_RELEASE;
        }
        break;
    case Player::SHOT_RELEASE:
		// 打ち出す計算
        DirectX::XMFLOAT3 camPos = m_pCamera->GetPos();						//カメラの位置を取得
        DirectX::XMVECTOR vCamPos = DirectX::XMLoadFloat3(&camPos);			//カメラの位置を計算用の型に変換
        DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(&m_pos);				//自分の位置を計算用の型に変換
        DirectX::XMVECTOR vec = DirectX::XMVectorSubtract(vPos, vCamPos);	//カメラから自分の位置に向かうベクトルを計算
        vec = DirectX::XMVector3Normalize(vec);		//ベクトルの正規化
        vec = DirectX::XMVectorScale(vec, m_shotPower);		//正規化したベクトルを、溜めた力に応じて伸ばす
        DirectX::XMStoreFloat3(&m_move, vec);//移動のデータm_moveに計算したvecを設定する（計算用の型から保存用の型に変換;

        // 打ち出し後の情報を設定
        m_isStop = false;			// 移動するので停止にしない 
        m_isGround = false;         // 飛ぶので一旦地面から離れたとみなす
        m_shotstep = SHOT_WAIT;		// キー入力待ちの手順に戻す
        m_shotPower = 0.0f;
        break;
    }
}

void Player::UpdateMove()
{
    // 重力 
    m_move.y -= 0.02f;

    // 減速処理(空気抵抗 
    m_move.x *= 0.99f;
    m_move.y *= 0.99f;
    m_move.z *= 0.99f;

    // 移動処理 
    m_pos.x += m_move.x;
    m_pos.y += m_move.y;
    m_pos.z += m_move.z;

    // 地面接触判定 
    if (m_pos.y < 0.0f) {
        m_pos.y = 0.0f;
        Bound(BoundY);
    }

    // 停止判定 
    if (CheckStop()) {
        m_isStop = true;
        m_shotStep = SHOT_WAIT;
    }
}
