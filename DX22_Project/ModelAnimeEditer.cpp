#include "ModelAnimeEditer.h"
#include "Transfer.h"
#include "Geometory.h"

ModelAnimeEditer::ModelAnimeEditer()
{
	TRAN_INS;
	pos = tran.modelediter.arm.pos;
	size = tran.modelediter.arm.size = { 1,1,1 };
	rotate = tran.modelediter.arm.rotate;
}

ModelAnimeEditer::~ModelAnimeEditer()
{

}

void ModelAnimeEditer::Update()
{
	TRAN_INS;
	subAngle = tran.modelediter.arm.subAngle;
	pos = tran.modelediter.arm.pos;
	size = tran.modelediter.arm.size;
	rotate = tran.modelediter.arm.rotate;

	pos.x = cosf(DirectX::XMConvertToRadians(subAngle.x)) * (size.x / 2.0f);
	pos.z = sinf(DirectX::XMConvertToRadians(subAngle.x)) * (size.x / 2.0f);

	rotate.y = -subAngle.x;

	tran.modelediter.arm.subAngle = subAngle;
	tran.modelediter.arm.pos = pos;
	tran.modelediter.arm.size = size;
	tran.modelediter.arm.rotate = rotate;
}

void ModelAnimeEditer::Draw()
{
	Geometory::SetView(m_pCamera->GetViewMatrix());
	Geometory::SetProjection(m_pCamera->GetProjectionMatrix());

	DirectX::XMMATRIX R =
		DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(rotate.x)) *
		DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(rotate.y)) *
		DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(rotate.z));

	DirectX::XMMATRIX world = DirectX::XMMatrixScaling(size.x,size.y,size.z) * R * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);

	DirectX::XMFLOAT4X4 World;

	DirectX::XMStoreFloat4x4(&World, DirectX::XMMatrixTranspose(world));

	Geometory::SetWorld(World);
	Geometory::DrawBox();
}

void ModelAnimeEditer::SetCamera(Camera* camera)
{
	m_pCamera = camera;
}
