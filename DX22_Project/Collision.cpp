#include "Collision.h"

Collision::Result Collision::Hit(Box a, Box b)
{
    Result out = {};

    // 計算用の方に変換
    DirectX::XMVECTOR vPosA = DirectX::XMLoadFloat3(&a.center);
    DirectX::XMVECTOR vPosB = DirectX::XMLoadFloat3(&b.center);
    DirectX::XMVECTOR vSizeA = DirectX::XMLoadFloat3(&a.size);
    DirectX::XMVECTOR vSizeB = DirectX::XMLoadFloat3(&b.size);

    // ボックスの半分のサイズを取得
    vSizeA = DirectX::XMVectorScale(vSizeA, 0.5f);
    vSizeB = DirectX::XMVectorScale(vSizeB, 0.5f);

    // ボックスの各軸の最大値、最小値を取得
    DirectX::XMVECTOR vMaxA = DirectX::XMVectorAdd(vPosA, vSizeA);
    DirectX::XMVECTOR vMinA = DirectX::XMVectorSubtract(vPosA, vSizeA);
    DirectX::XMVECTOR vMaxB = DirectX::XMVectorAdd(vPosB, vSizeB);
    DirectX::XMVECTOR vMinB = DirectX::XMVectorSubtract(vPosB, vSizeB);
    DirectX::XMFLOAT3 maxA, minA, maxB, minB;
    //maxA vMaxA minA, maxB, minB
    DirectX::XMStoreFloat3(&maxA,vMaxA);
    DirectX::XMStoreFloat3(&minA,vMinA);
    DirectX::XMStoreFloat3(&maxB,vMaxB);
    DirectX::XMStoreFloat3(&minB,vMinB);
        //  
    out.isHit = false;

    //  
    if (maxA.x  >= minB.x && minA.x <= maxB.x) {
        if (maxA.y >= minB.y && minA.y <= maxB.y) {
            if ((maxA.z >= minB.z && minA.z <= maxB.z)) {
                //  
                out.isHit = true;

                DirectX::XMVECTOR vDist =
                    DirectX::XMVectorSubtract(vPosA, vPosB);
                vDist = DirectX::XMVectorAbs(vDist);

                DirectX::XMVECTOR vSumSize =
                    DirectX::XMVectorAdd(vSizeA, vSizeB);
                DirectX::XMVECTOR vOverlap =
                    DirectX::XMVectorSubtract(vSumSize, vDist);
                DirectX::XMFLOAT3 overlap;
                DirectX::XMStoreFloat3(&overlap, vOverlap);

                // 各軸のめり込み量のうち、最小のめり込み量の方向へ跳ね返す 
                if (overlap.x < overlap.y) {
                    if (overlap.x < overlap.z)
                        out.dir = { a.center.x < b.center.x ? -1.0f : 1.0f, 0.0f, 0.0f };
                    else
                        out.dir = { 0.0f, 0.0f, a.center.z < b.center.z ? -1.0f : 1.0f };
                }
                else {
                    if (overlap.y < overlap.z)
                        out.dir = { 0.0f,  a.center.y < b.center.y ? -1.0f : 1.0f, 0.0f };
                    else
                        out.dir = { 0.0f, 0.0f, a.center.z < b.center.z ? -1.0f : 1.0f };
                }
            }
        }
    }

    return out;
}

Collision::Result Collision::Hit(Sphere a, Sphere b)
{
    Result out = {};

    //  
    DirectX::XMVECTOR vPosA = DirectX::XMLoadFloat3(&a.center);
    DirectX::XMVECTOR vPosB = DirectX::XMLoadFloat3(&b.center);

    //  
    DirectX::XMVECTOR vDist = DirectX::XMVectorSubtract(vPosA, vPosB);

    //  
    DirectX::XMVECTOR vLen = vDist;
    float length;
    DirectX::XMStoreFloat(&length,vLen);

    //  
    out.isHit = (a.radius + b.radius) >= length;
    // if( a.radius + b.radius)>= lenths

    return out;
}
