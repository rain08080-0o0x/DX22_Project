#include "Main.h"
#include <memory>
#include "DirectX.h"
#include "Geometory.h"
#include "Sprite.h"
#include "Input.h"
#include "SceneManager.h"
#include "Defines.h"
#include "ShaderList.h"
#include "Transfer.h"
#include "manager.h"
// rand初期化用
#include <cstdlib>
#include <ctime>

// ImGui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// デバッグ用
#include "DebugUtil.h"


HRESULT Init(HWND hWnd, UINT width, UINT height)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	HRESULT hr;
	// DirectX初期化
	hr = InitDirectX(hWnd, width, height, false);
	if (FAILED(hr)) { return hr; }

	std::srand(static_cast<unsigned int>(std::time(NULL)));


	// シーン
	Manager::Init();

	return hr;
}

void Uninit()
{
	double t0 = NowMS();
	DebugLog("App shutdown begin\n");

	Manager::Uninit();

	ShaderList::Uninit();
	UninitDirectX();

	DebugLog("App shutdown end : %.2f ms\n", NowMS() - t0);
}

void Update()
{
	Manager::Update();
}

void Draw()
{
	BeginDrawDirectX();

#ifdef _DEBUG
	TRAN_INS;

	// ImGuiの描画
	static bool show_main_window = false;
	static bool show_camera_window;
	static bool show_dice_window;
	static bool show_dice4x4_window;

	if(IsKeyTrigger(VK_LSHIFT))show_main_window = !show_main_window;

	using namespace ImGui;
	using namespace std;
	if (show_main_window)
	{
		Begin("Main Setting Window",&show_main_window);

		if (BeginTabBar("tabber"))
		{
			if (BeginTabItem("Camera"))
			{

				static float min = 20.0f;
				static float max = 80.0f;

				DragFloatRange2("Kari", &min, &max, 0.1f, 0.0f, 100.0f);

				float eye[3] = { tran.camera.eye.x,tran.camera.eye.y,tran.camera.eye.z };
				float look[3] = { tran.camera.look.x,tran.camera.look.y,tran.camera.look.z };
				DragFloat3("Camera Eye Position", eye);
				DragFloat3("Camera Look Position", look);

				tran.camera.eye = { eye[0],eye[1],eye[2] };
				tran.camera.look = { look[0],look[1],look[2] };

				EndTabItem();
			}
			if (BeginTabItem("Dice"))
			{
				DragFloat3("Position", reinterpret_cast<float*>(&tran.dice.pos), 0.1f);
				DragFloat3("Velocity", reinterpret_cast<float*>(&tran.dice.velocity));
				DragFloat4("Color", reinterpret_cast<float*>(&tran.dice.color), 0.01f, 0.0f, 1.0f);
				DragFloat4("Set Velocity", reinterpret_cast<float*>(&tran.dice.virtualVelocity), 0.01f);
				Text("rot %f:%f:%f:%f", tran.dice.rot.x, tran.dice.rot.y, tran.dice.rot.z, tran.dice.rot.w);
				if (Button("Set"))
				{
					tran.dice.velocity = tran.dice.virtualVelocity;
					tran.dice.virtualVelocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
				}
				EndTabItem();
			}
			if (BeginTabItem("Dice 4X4"))
			{

				float mat_1[4] = { tran.dice.world._11,tran.dice.world._12,tran.dice.world._13,tran.dice.world._14 };
				float mat_2[4] = { tran.dice.world._21,tran.dice.world._22,tran.dice.world._23,tran.dice.world._24 };
				float mat_3[4] = { tran.dice.world._31,tran.dice.world._32,tran.dice.world._33,tran.dice.world._34 };
				float mat_4[4] = { tran.dice.world._41,tran.dice.world._42,tran.dice.world._43,tran.dice.world._44 };

				DragFloat4("1", mat_1, 0.1f);
				DragFloat4("2", mat_2, 0.1f);
				DragFloat4("3", mat_3, 0.1f);
				DragFloat4("4\n\n", mat_4, 0.1f);
				tran.dice.world = {
					mat_1[0],mat_1[1],mat_1[2],mat_1[3],
					mat_2[0],mat_2[1],mat_2[2],mat_2[3],
					mat_3[0],mat_3[1],mat_3[2],mat_3[3],
					mat_4[0],mat_4[1],mat_4[2],mat_4[3]
				};

				DragFloat3("Object A", reinterpret_cast<float*>(&tran.obj.A), 0.01f);
				DragFloat3("Object B\n\n", reinterpret_cast<float*>(&tran.obj.B), 0.01f);

				DragFloat3("Object A Velocity", reinterpret_cast<float*>(&tran.obj.Avel));
				DragFloat3("Object B Velocity", reinterpret_cast<float*>(&tran.obj.Bvel));

				DragFloat3("Object A Angle Velocity", reinterpret_cast<float*>(&tran.obj.AangVel));
				DragFloat3("Object B Angle Velocity", reinterpret_cast<float*>(&tran.obj.BangVel));

				EndTabItem();
			}
			if (BeginTabItem("Dice drop"))
			{
				for (int i = 0; i < MAX_DICE; i++)
				{
					std::string faceText = "Current Up Face " + std::to_string(tran.dice.currentFaceNumber[i]);
					if (tran.dice.currentFaceNumber[i] == 0)continue;
					Text(faceText.c_str());
				}

				ImGui::SliderFloat("under", &tran.dice.underVel,0.0f,1.0f);

				EndTabItem();
			}
			if (BeginTabItem("UI"))
			{
				tran.diceui.role.pos;
				DragFloat2("role pos", reinterpret_cast<float*>(&tran.diceui.role.pos));
				DragFloat2("role size", reinterpret_cast<float*>(&tran.diceui.role.size));
				ColorEdit4("role color", reinterpret_cast<float*>(&tran.diceui.role.color));

				EndTabItem();
			}
			if (BeginTabItem("Mouse"))
			{
				std::string mouseInfoText;

				ImGuiIO& io = ImGui::GetIO();
				tran.mousePos.x = ImGui::GetCursorScreenPos().x;
				tran.mousePos.y = ImGui::GetCursorScreenPos().y;
				mouseInfoText = std::to_string(tran.mousePos.x) + " : " + std::to_string(tran.mousePos.y);
				ImGui::Text(mouseInfoText.c_str());

				if (ImGui::IsMousePosValid())
					ImGui::Text("Mouse pos: (%g, %g)", io.MousePos.x, io.MousePos.y);
				EndTabItem();
			}
			if (BeginTabItem("Yukari"))
			{
				DragFloat2("Yukari Pos", reinterpret_cast<float*>(&tran.yukari.pos));
				DragFloat2("Yukari size", reinterpret_cast<float*>(&tran.yukari.size));

				DragFloat2("Fukidasi pos", reinterpret_cast<float*>(&tran.fuki.pos));
				DragFloat2("Fukidasi size", reinterpret_cast<float*>(&tran.fuki.size));

				EndTabItem();
			}
			if (BeginTabItem("Tyabudai"))
			{
				tran.tyabu.pos;
				DragFloat3("Tyabudai pos", reinterpret_cast<float*>(&tran.tyabu.pos),0.1f);
				DragFloat3("Tyabudai size", reinterpret_cast<float*>(&tran.tyabu.size),0.1f);

				DragFloat3("Tyawan pos", reinterpret_cast<float*>(&tran.tyawan.pos),0.1f);
				DragFloat3("Tyawan size", reinterpret_cast<float*>(&tran.tyawan.size),0.1f);

				EndTabItem();
			}


			EndTabBar();
		}

		Text("FPS: %.1f", GetIO().Framerate);
		End();
	}


	// 軸線の表示
	// グリッド
	DirectX::XMFLOAT4 lineColor(0.5f, 0.5f, 0.5f, 1.0f);
	float size = DEBUG_GRID_NUM * DEBUG_GRID_MARGIN;
	for (int i = 1; i <= DEBUG_GRID_NUM; ++i)
	{
		float grid = i * DEBUG_GRID_MARGIN;
		DirectX::XMFLOAT3 pos[2] = {
			DirectX::XMFLOAT3(grid, 0.0f, size),
			DirectX::XMFLOAT3(grid, 0.0f,-size),
		};
		Geometory::AddLine(pos[0], pos[1], lineColor);
		pos[0].x = pos[1].x = -grid;
		Geometory::AddLine(pos[0], pos[1], lineColor);
		pos[0].x = size;
		pos[1].x = -size;
		pos[0].z = pos[1].z = grid;
		Geometory::AddLine(pos[0], pos[1], lineColor);
		pos[0].z = pos[1].z = -grid;
		Geometory::AddLine(pos[0], pos[1], lineColor);
	}
	// 軸
	Geometory::AddLine(DirectX::XMFLOAT3(0,0,0), DirectX::XMFLOAT3(size,0,0), DirectX::XMFLOAT4(1,0,0,1));
	Geometory::AddLine(DirectX::XMFLOAT3(0,0,0), DirectX::XMFLOAT3(0,size,0), DirectX::XMFLOAT4(0,1,0,1));
	Geometory::AddLine(DirectX::XMFLOAT3(0,0,0), DirectX::XMFLOAT3(0,0,size), DirectX::XMFLOAT4(0,0,1,1));
	Geometory::AddLine(DirectX::XMFLOAT3(0,0,0), DirectX::XMFLOAT3(-size,0,0),  DirectX::XMFLOAT4(0,0,0,1));
	Geometory::AddLine(DirectX::XMFLOAT3(0,0,0), DirectX::XMFLOAT3(0,0,-size),  DirectX::XMFLOAT4(0,0,0,1));

	Geometory::DrawLines();

	// カメラの値
	static bool camAutoSwitch = false;
	static bool camUpDownSwitch = true;
	static float camAutoRotate = 1.0f;
	if (IsKeyTrigger(VK_RETURN) && false) {
		camAutoSwitch ^= true;
	}
	if (IsKeyTrigger(VK_SPACE)) {
		camUpDownSwitch ^= true;
	}

	DirectX::XMVECTOR camPos;
	if (camAutoSwitch) {
		camAutoRotate += 0.01f;
	}
	camPos = DirectX::XMVectorSet(
		cosf(camAutoRotate) * 5.0f,
		3.5f * (camUpDownSwitch ? 1.0f : -1.0f),
		sinf(camAutoRotate) * 5.0f,
		0.0f);

	// ジオメトリ用カメラ初期化
	DirectX::XMFLOAT4X4 mat[2];
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixTranspose(
		DirectX::XMMatrixLookAtLH(
			camPos,
			DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
		)));
	DirectX::XMStoreFloat4x4(&mat[1], DirectX::XMMatrixTranspose(
		DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(60.0f), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 1000.0f)
	));
	Geometory::SetView(mat[0]);
	Geometory::SetProjection(mat[1]);
#endif

	Manager::Draw();
	EndDrawDirectX();
}

// EOF