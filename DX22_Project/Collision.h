#pragma once
#include<DirectXMath.h>

class Collision {
public:
    //--- 当たり判定
    // 立方体
    struct Box
    {
        DirectX::XMFLOAT3 center;   // 中心座標
        DirectX::XMFLOAT3 size;     // サイズ
    };
    // 球
    struct Sphere 
    {
        DirectX::XMFLOAT3 center;   // 中心座標
        float radius;               // 半径
    };
    //--- 当たり判定の結果
    struct Result 
    {
        bool isHit;             // 当たったかどうか
        DirectX::XMFLOAT3 dir;  // ヒット方向
        //      
    };
    // 回転付きボックス（Oriented Bounding Box）
    struct OBB
    {
        DirectX::XMFLOAT3 center;     // 中心位置
        DirectX::XMFLOAT3 axis[3];    // 回転したローカル軸（正規化済み）
        DirectX::XMFLOAT3 halfSize;   // 半サイズ
    };

    struct Manifold
    {
        bool hit = false;

        DirectX::XMFLOAT3 normal; // 押し戻し方向（A → B）
        float penetration = 0.0f; // めり込み量
    };

public:
    //  立方体同士の当たり判定
    static Result Hit(Box a, Box b);

    //  
    static Result Hit(Sphere a, Sphere b);

    // OBB 同士の衝突判定（最小版）
    static bool HitOBB(const OBB& a, const OBB& b);


    static bool HitOBB_Full(const OBB& a, const OBB& b, Manifold& m);
};
