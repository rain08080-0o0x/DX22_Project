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
public:
    //  立方体同士の当たり判定
    static Result Hit(Box a, Box b);

    //  
    static Result Hit(Sphere a, Sphere b);
};
