// SceneNewLastBossEditor.h

#pragma once
#include "Scene.h"
#include "Texture.h"
#include "Sprite.h"
#include "ShaderList.h"
#include "Camera.h"
#include <DirectXMath.h>

struct CharInfo
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 size;
    bool useBillboard;
    float yawDeg;
};

/// <summary>
/// 
/// </summary>
/// @param None     : 攻撃なし状態
/// @param UnUsed   : 攻撃未発動状態
/// @param Using    : 攻撃発動中
/// @param Used     : 攻撃発動後
enum AttackState
{
    None,
    UnUsed,
    Using,
    Used,
};

class NewLastBoss : public Scene
{
public:
    NewLastBoss();
    ~NewLastBoss();

    void Init();
    void Uninit();
    void Update();
    void Draw();

private:

    void UpdatePlayer();
    void UpdateBoss();
    void DrawField();
    // ランダムスラッシュ攻撃を追加する関数。最大が10。
    // それ以上は追加できず初期化しないと再発動はできない。
    void AddAttack();
    // ランダムスラッシュの更新
    // プレイヤーの位置を中心にランダムな角度で攻撃が発生、
    // その場に一定時間の範囲を描画の後、攻撃が発生する。
    void RandomSlashUpdate();
    // ランダムスラッシュの描画
    // 当たり判定と、攻撃範囲を描画
    // テクスチャを出しているものの処理内容は当たり判定依存。
    void RandomSlashDraw();
    // クロス攻撃の更新
    // ステージ全体を斜めに切るように攻撃が発生。
    // 攻撃の向きは45度で、格子状
    void CrossUpdate();
    // クロス攻撃の描画
    // 当たり判定と、攻撃範囲を描画
    // テクスチャを出しているものの処理内容は当たり判定依存。
    void CrossDraw();
    // プレイヤー依存のクロス攻撃の更新
    // 最初にステージの中心からみたプレイヤーの位置を求め、
    // ステージの中心を通る直線を求めた角度で攻撃が発生。
    // その求めた角度に対して45度ずらした攻撃を追加で発生させる格子攻撃。
    // 攻撃は基の角度の方と45度ずらした方の両方で、
    // 攻撃が終わるたびにもう一度発動し段々攻撃が外側に増える。
    // 同心円状ならぬ同心平行状。
    void CircleCrossUpdate();
    // プレイヤー依存のクロス攻撃の描画
    void CircleCrossDraw();
    // デバッグ用のGUI描画関数。様々な情報を表示するための関数。
    // 随時更新
    void DrawDebugGUI();
    Camera* camera;
    Texture* m_pPlayer;
    Texture* m_pBoss;
    CharInfo player;
    CharInfo boss;

    float playerSpeed = 0.01f;

    Texture* m_pFieldtex;   // フィールドのテクスチャ
    Texture* m_pAtkTex;     // 攻撃のテクスチャ

    // ランダムスラッシュの攻撃状態を表す列挙型。
    // デフォはUnUsed
    // 攻撃最中はUseing
    // 攻撃後はUsed
    //
    enum class SlashState
    {
        UnUsed,
        Useing,
        Used
    };
    /// <summary>
    /// 攻撃の各情報をまとめた構造体。
    /// </summary>
    struct RandomSlash
    {
        SlashState isUsed;                // 最大同時攻撃数10を上限。攻撃が発生しているかどうか
        int frame;                  // 攻撃のフレームカウンタ。攻撃開始から何フレーム経過したかをカウントする。
        float angle;                // 攻撃角度。PI*2が一周。攻撃の向きを表す。
        DirectX::XMFLOAT3 start;    // 攻撃の中間
        DirectX::XMFLOAT3 end;      // 
        DirectX::XMFLOAT3 middle;   // 
    };

    RandomSlash rs_attack[10];
    int count = 0;

    enum CrossState
    {
        First,
        Second,
        Final,
        Max
    };

    CrossState crossState = CrossState::Max;

    struct CircleCross
    {
        AttackState state = AttackState::Used;
		float firstAngle;   // 最初の攻撃の角度。プレイヤーの位置から求める。
        float secondAngle;  // もう一つの攻撃角度。45度ずれたもの。
        float width;        // 攻撃の幅。長さはステージを切るように長いのでほぼ固定。
        const float length = 10.0f * sqrtf(2.0f); // 攻撃の長さ。ステージを丁度斜めに切っても届くように長めに定義。
        // 攻撃間のインターバル。
        // 二種類目(45度ずれ攻撃)の発生までの時間。
    };
    CircleCross cc; // CircleCross
};
