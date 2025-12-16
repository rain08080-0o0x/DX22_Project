#include "Collision.h"

#include <DirectXMath.h>
#include <cmath>
using namespace DirectX;

// 軸 axis に OBB を投影した半径を求める
static float ProjectRadius(const Collision::OBB& o, FXMVECTOR axis)
{
    XMVECTOR ax0 = XMLoadFloat3(&o.axis[0]);
    XMVECTOR ax1 = XMLoadFloat3(&o.axis[1]);
    XMVECTOR ax2 = XMLoadFloat3(&o.axis[2]);

    float r =
        fabsf(XMVectorGetX(XMVector3Dot(axis, ax0))) * o.halfSize.x +
        fabsf(XMVectorGetX(XMVector3Dot(axis, ax1))) * o.halfSize.y +
        fabsf(XMVectorGetX(XMVector3Dot(axis, ax2))) * o.halfSize.z;

    return r;
}

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

    DirectX::XMVECTOR vPosA = DirectX::XMLoadFloat3(&a.center);
    DirectX::XMVECTOR vPosB = DirectX::XMLoadFloat3(&b.center);

    DirectX::XMVECTOR vDist = DirectX::XMVectorSubtract(vPosA, vPosB);

    DirectX::XMVECTOR vLen = vDist;
    float length;
    DirectX::XMStoreFloat(&length,vLen);

    out.isHit = (a.radius + b.radius) >= length;

    return out;
}

bool Collision::HitOBB(const OBB& a, const OBB& b)
{
    using namespace DirectX;

    // 中心差ベクトル
    XMVECTOR d = XMVectorSubtract(
        XMLoadFloat3(&b.center),
        XMLoadFloat3(&a.center)
    );

    // 判定に使う軸（Aの3軸 + Bの3軸）
    XMVECTOR axes[6] =
    {
        XMLoadFloat3(&a.axis[0]),
        XMLoadFloat3(&a.axis[1]),
        XMLoadFloat3(&a.axis[2]),
        XMLoadFloat3(&b.axis[0]),
        XMLoadFloat3(&b.axis[1]),
        XMLoadFloat3(&b.axis[2]),
    };

    for (int i = 0; i < 6; ++i)
    {
        XMVECTOR L = axes[i];

        // 念のため正規化
        L = XMVector3Normalize(L);

        // 中心距離を軸に投影
        float dist =
            fabsf(XMVectorGetX(XMVector3Dot(d, L)));

        // 各 OBB の投影半径
        float ra = ProjectRadius(a, L);
        float rb = ProjectRadius(b, L);

        // 分離していたら非衝突
        if (dist > ra + rb)
            return false;
    }

    // 全軸で分離していなければ衝突
    return true;
}

bool Collision::HitOBB_Full(const OBB& a, const OBB& b, Manifold& m)
{
    using namespace DirectX;

    m.hit = false;
    m.penetration = FLT_MAX;

    // 中心差
    XMVECTOR d =
        XMVectorSubtract(XMLoadFloat3(&b.center),
            XMLoadFloat3(&a.center));

    // 軸リスト（最大15）
    XMVECTOR axes[15];
    int axisCount = 0;

    // A の軸
    for (int i = 0; i < 3; ++i)
        axes[axisCount++] = XMLoadFloat3(&a.axis[i]);

    // B の軸
    for (int i = 0; i < 3; ++i)
        axes[axisCount++] = XMLoadFloat3(&b.axis[i]);

    // 交差軸
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            XMVECTOR c =
                XMVector3Cross(
                    XMLoadFloat3(&a.axis[i]),
                    XMLoadFloat3(&b.axis[j])
                );

            // ほぼゼロ軸は無視
            if (XMVectorGetX(XMVector3LengthSq(c)) > 1e-6f)
            {
                axes[axisCount++] = c;
            }
        }
    }

    // SAT 本体
    for (int i = 0; i < axisCount; ++i)
    {
        XMVECTOR L = XMVector3Normalize(axes[i]);

        float dist =
            fabsf(XMVectorGetX(XMVector3Dot(d, L)));

        float ra = ProjectRadius(a, L);
        float rb = ProjectRadius(b, L);

        float overlap = ra + rb - dist;

        // 分離していたら非衝突
        if (overlap < 0.0f)
            return false;

        // 最小貫通軸を記録
        if (overlap < m.penetration)
        {
            m.penetration = overlap;

            // 押し戻し方向（A → B）
            if (XMVectorGetX(XMVector3Dot(d, L)) < 0.0f)
                L = XMVectorNegate(L);

            XMStoreFloat3(&m.normal, L);
        }
    }

    m.hit = true;
    return true;
}
