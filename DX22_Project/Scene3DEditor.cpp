#include "Scene3DEditor.h"
#include "Geometory.h"
#include "Transfer.h"
#include "CameraDebug.h"
#include "Sprite.h"
#include <memory>

#define SAFE_UPDATE(p) if(p){p->Update();}

Scene3DEditor::Scene3DEditor()
{
    m_arm1.pos      = { 1,1,1 };
    m_arm1.gpos     = { 1,1,1 };
    m_arm1.opos     = { 1,1,1 };
    m_arm1.scale    = { 1,1,1 };
    m_arm1.rotate   = { 1,1,1 };

    m_pCamera = new CameraDebug();
    if (m_pCamera)
    {
        m_pCamera->LockPos(false);
        TRAN_INS;
        tran.camera.eye = { 0.0f, 6.0f, -6.0f };
        tran.camera.look = { 0.0f, 0.0f, 0.0f };
        m_pCamera->SetPos(tran.camera.eye);
        m_pCamera->SetLook(tran.camera.look);
    }
}

Scene3DEditor::~Scene3DEditor()
{
    SAFE_DELETE(m_pCamera);
}

void Scene3DEditor::Update()
{
    SAFE_UPDATE(m_pCamera);
}

void Scene3DEditor::Draw()
{
    if (!m_pCamera) return;

    DirectX::XMFLOAT4X4 view = m_pCamera->GetViewMatrix();
    DirectX::XMFLOAT4X4 proj = m_pCamera->GetProjectionMatrix();

    Geometory::SetView(view);
    Geometory::SetProjection(proj);
    Sprite::SetView(view);
    Sprite::SetProjection(proj);
}
