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
// rand初期化用
#include <cstdlib>
#include <ctime>

// ImGui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "Easing.h"

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

	// 他機能初期化
	Geometory::Init();
	Sprite::Init();
	InitInput();
	ShaderList::Init();

	// シーン
	SceneManager::Init();

	return hr;
}

void Uninit()
{
	double t0 = NowMS();
	DebugLog("App shutdown begin\n");

	SceneManager::Uninit();

	ShaderList::Uninit();
	UninitInput();
	Sprite::Uninit();
	Geometory::Uninit();
	UninitDirectX();

	DebugLog("App shutdown end : %.2f ms\n", NowMS() - t0);
}

void Update()
{
	UpdateInput();
	SceneManager::Update();
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

	if(IsKeyTrigger(VK_TAB))show_main_window = !show_main_window;

	using namespace ImGui;
	using namespace std;
	if (show_main_window)
	{
		Begin("Main Setting Window",&show_main_window);

		string sceneTxt;
		switch (SceneManager::GetCurrent())
		{
		case SceneManager::SceneType::SCENE_TITLE:
			sceneTxt = "Title";
			break;
		case SceneManager::SceneType::SCENE_GAME:
			sceneTxt = "Game";
			break;
		case SceneManager::SceneType::SCENE_RESULT:
			sceneTxt = "Result";
			break;
		case SceneManager::SceneType::SCENE_3DEDITOR:
			sceneTxt = "3DEditor";
			break;
		default:
			sceneTxt = "Unknown";
			break;
		}
		sceneTxt = "Current Scene : " + sceneTxt;
		ImGui::Text(sceneTxt.c_str());

		if (BeginTabBar("TabBar"))
		{
			if (BeginTabItem("Player"))
			{
				DragFloat3("Position", reinterpret_cast<float*>(&tran.player.pos), 0.05f);
				DragFloat3("Size", reinterpret_cast<float*>(&tran.player.size), 0.05f);
				DragFloat3("Velocity", reinterpret_cast<float*>(&tran.player.velocity), 0.05f);
				DragFloat("Move Speed", &tran.player.moveSpeed, 0.01f, 0.0f, 10.0f);
				DragFloat("Dash Distance", &tran.player.dashDistance, 0.05f, 0.0f, 10.0f);
				DragFloat("Dash Cooldown", &tran.player.dashCooldown, 0.01f, 0.0f, 5.0f);
				DragFloat("Dash Duration", &tran.player.dashDuration, 0.01f, 0.01f, 1.0f);
				DragFloat("Stage Size", &tran.player.stageSize, 0.1f, 1.0f, 20.0f);
				ColorEdit4("Color", reinterpret_cast<float*>(&tran.player.color));
				EndTabItem();
			}
			if (BeginTabItem("ModelEditer"))
			{
				ImGui::SeparatorText(u8"腕");
				if(TreeNode("Arm"))
				{
					PushID(0);
					if(TreeNode("Right"))
					{
						ImGui::DragFloat3("Right Arm1 Angle", reinterpret_cast<float*>(&tran.modelediter.armRight1.subAngle), 0.1f);
						ImGui::DragFloat3("Right Arm1 Pos", reinterpret_cast<float*>(&tran.modelediter.armRight1.pos), 0.1f);
						ImGui::DragFloat3("Right Arm1 Size", reinterpret_cast<float*>(&tran.modelediter.armRight1.size), 0.1f);
						ImGui::DragFloat3("Right Arm1 Rotate", reinterpret_cast<float*>(&tran.modelediter.armRight1.rotate), 0.1f);
						ImGui::SeparatorText("Right Arm2");
						ImGui::DragFloat3("Right Arm2 Angle", reinterpret_cast<float*>(&tran.modelediter.armRight2.subAngle), 0.1f);
						ImGui::DragFloat3("Right Arm2 Pos", reinterpret_cast<float*>(&tran.modelediter.armRight2.pos), 0.1f);
						ImGui::DragFloat3("Right Arm2 Size", reinterpret_cast<float*>(&tran.modelediter.armRight2.size), 0.1f);
						ImGui::DragFloat3("Right Arm2 Rotate", reinterpret_cast<float*>(&tran.modelediter.armRight2.rotate), 0.1f);
						TreePop();
					}
					if(TreeNode("Left"))
					{
						ImGui::SeparatorText("Left Arm1");
						ImGui::DragFloat3("Left Arm1 Angle", reinterpret_cast<float*>(&tran.modelediter.armLeft1.subAngle), 0.1f);
						ImGui::DragFloat3("Left Arm1 Pos", reinterpret_cast<float*>(&tran.modelediter.armLeft1.pos), 0.1f);
						ImGui::DragFloat3("Left Arm1 Size", reinterpret_cast<float*>(&tran.modelediter.armLeft1.size), 0.1f);
						ImGui::DragFloat3("Left Arm1 Rotate", reinterpret_cast<float*>(&tran.modelediter.armLeft1.rotate), 0.1f);
						ImGui::SeparatorText("Left Arm2");
						ImGui::DragFloat3("Left Arm2 Angle", reinterpret_cast<float*>(&tran.modelediter.armLeft2.subAngle), 0.1f);
						ImGui::DragFloat3("Left Arm2 Pos", reinterpret_cast<float*>(&tran.modelediter.armLeft2.pos), 0.1f);
						ImGui::DragFloat3("Left Arm2 Size", reinterpret_cast<float*>(&tran.modelediter.armLeft2.size), 0.1f);
						ImGui::DragFloat3("Left Arm2 Rotate", reinterpret_cast<float*>(&tran.modelediter.armLeft2.rotate), 0.1f);
						TreePop();
					}
					TreePop();
					PopID();
				}
				ImGui::SeparatorText("Leg ");
				if(TreeNode("Leg "))
				{
					PushID(1);
					if(TreeNode("Right"))
					{ 
						ImGui::DragFloat3("Right Leg 1 Angle", reinterpret_cast<float*>(&tran.modelediter.legRight1.subAngle), 0.1f);
						ImGui::DragFloat3("Right Leg 1 Pos", reinterpret_cast<float*>(&tran.modelediter.legRight1.pos), 0.1f);
						ImGui::DragFloat3("Right Leg 1 Size", reinterpret_cast<float*>(&tran.modelediter.legRight1.size), 0.1f);
						ImGui::DragFloat3("Right Leg 1 Rotate", reinterpret_cast<float*>(&tran.modelediter.legRight1.rotate), 0.1f);
						ImGui::SeparatorText("Right Leg 2");
						ImGui::DragFloat3("Right Leg 2 Angle", reinterpret_cast<float*>(&tran.modelediter.legRight2.subAngle), 0.1f);
						ImGui::DragFloat3("Right Leg 2 Pos", reinterpret_cast<float*>(&tran.modelediter.legRight2.pos), 0.1f);
						ImGui::DragFloat3("Right Leg 2 Size", reinterpret_cast<float*>(&tran.modelediter.legRight2.size), 0.1f);
						ImGui::DragFloat3("Right Leg 2 Rotate", reinterpret_cast<float*>(&tran.modelediter.legRight2.rotate), 0.1f);
						TreePop();
					}
					if(TreeNode("Left"))
					{ 
						ImGui::SeparatorText("Left Leg 1");
						ImGui::DragFloat3("Left Leg 1 Angle", reinterpret_cast<float*>(&tran.modelediter.legLeft1.subAngle), 0.1f);
						ImGui::DragFloat3("Left Leg 1 Pos", reinterpret_cast<float*>(&tran.modelediter.legLeft1.pos), 0.1f);
						ImGui::DragFloat3("Left Leg 1 Size", reinterpret_cast<float*>(&tran.modelediter.legLeft1.size), 0.1f);
						ImGui::DragFloat3("Left Leg 1 Rotate", reinterpret_cast<float*>(&tran.modelediter.legLeft1.rotate), 0.1f);
						ImGui::SeparatorText("Left Leg 2");
						ImGui::DragFloat3("Left Leg 2 Angle", reinterpret_cast<float*>(&tran.modelediter.legLeft2.subAngle), 0.1f);
						ImGui::DragFloat3("Left Leg 2 Pos", reinterpret_cast<float*>(&tran.modelediter.legLeft2.pos), 0.1f);
						ImGui::DragFloat3("Left Leg 2 Size", reinterpret_cast<float*>(&tran.modelediter.legLeft2.size), 0.1f);
						ImGui::DragFloat3("Left Leg 2 Rotate", reinterpret_cast<float*>(&tran.modelediter.legLeft2.rotate), 0.1f);
						TreePop();
					}
					TreePop();
					PopID();
				}

				ImGui::SeparatorText("Body");
				ImGui::DragFloat3("Body Pos", reinterpret_cast<float*>(&tran.modelediter.body.pos), 0.1f);
				ImGui::DragFloat3("Body Size", reinterpret_cast<float*>(&tran.modelediter.body.size), 0.1f);
				ImGui::DragFloat3("Body Rotate", reinterpret_cast<float*>(&tran.modelediter.body.angle), 0.1f);
				ImGui::DragFloat3("Body Joint Right Arm", reinterpret_cast<float*>(&tran.modelediter.body.jointRightArmPos),0.1f);
				ImGui::DragFloat3("Body Joint Left Arm", reinterpret_cast<float*>(&tran.modelediter.body.jointLeftArmPos),0.1f);
				ImGui::DragFloat3("Body Joint Right Leg", reinterpret_cast<float*>(&tran.modelediter.body.jointRightLegPos),0.1f);
				ImGui::DragFloat3("Body Joint Left Leg", reinterpret_cast<float*>(&tran.modelediter.body.jointLeftLegPos),0.1f);

				EndTabItem();
			}
			if (BeginTabItem("Camera"))
            {

                static float min = 20.0f;
                static float max = 80.0f;

                DragFloatRange2("Kari", &min, &max, 0.1f, 0.0f, 100.0f);

                const char* cameraModes[] = { "Game", "Debug" };
                int mode = tran.cameraMode;
                if (Combo("Camera Mode", &mode, cameraModes, IM_ARRAYSIZE(cameraModes)))
                {
                    tran.cameraMode = mode;
                }

                const char* activeLabel = (tran.cameraMode == 1) ? "Debug" : "Game";
                Text("Active: %s", activeLabel);

                SeparatorText("Game Camera");
                float gameEye[3] = { tran.cameraGame.eye.x, tran.cameraGame.eye.y, tran.cameraGame.eye.z };
                float gameLook[3] = { tran.cameraGame.look.x, tran.cameraGame.look.y, tran.cameraGame.look.z };
                if (DragFloat3("Game Eye", gameEye, 0.05f))
                {
                    tran.cameraGame.eye = { gameEye[0], gameEye[1], gameEye[2] };
                }
                if (DragFloat3("Game Look", gameLook, 0.05f))
                {
                    tran.cameraGame.look = { gameLook[0], gameLook[1], gameLook[2] };
                }

                SeparatorText("Debug Camera");
                float debugEye[3] = { tran.cameraDebug.eye.x, tran.cameraDebug.eye.y, tran.cameraDebug.eye.z };
                float debugLook[3] = { tran.cameraDebug.look.x, tran.cameraDebug.look.y, tran.cameraDebug.look.z };
                if (DragFloat3("Debug Eye", debugEye, 0.05f))
                {
                    tran.cameraDebug.eye = { debugEye[0], debugEye[1], debugEye[2] };
                }
                if (DragFloat3("Debug Look", debugLook, 0.05f))
                {
                    tran.cameraDebug.look = { debugLook[0], debugLook[1], debugLook[2] };
                }

                EndTabItem();
            }
			if (BeginTabItem("Graph"))
			{
				static float easing = 1;

				DragFloat("Expf easing", &easing,0.1f);
				Separator();

				float target = 1000;
				float current = 10;
				float frame[100];
				for (int i = 0; i < 100; i++)
				{
					current = current + (target - current) * expf(-easing);
					frame[i] = current;
				}

				PlotLines("expf", frame, 100, 0, 0, 3.4028235E38F, 3.4028235E38F, { 100,100 }, 4);
				Separator();
				current = 0;
				float PlanA[100];
				for (int i = 0; i < 100; i++)
				{
					current += 0.01f;
					PlanA[i] = current * current * current * current;
				}

				PlotLines("Plan A",PlanA,100, 0, 0, 3.4028235E38F, 3.4028235E38F, { 100,100 }, 4);
				Separator();
				float c4 = (2 * PI) / 3;

				current = 0;
				float  PlanB[100];
				for (int i = 0; i < 100; i++)
				{
					current += 0.01f;
					PlanB[i] = current == 0.0f ? 0.0f : current == 1.0f ? 1.0f : powf(2, -10 * current) * sinf((current * 10 - 0.75f) * c4) + 1.0f;
				}
				PlotLines("Plan B", PlanB, 100, 0, 0, 3.4028235E38F, 3.4028235E38F, { 100,100 }, 4);

				Separator();

				current = 0;

				float n1 = 7.5625;
				float d1 = 2.75;
				float PlanC[100];
				for(int i = 0; i < 100;i++)
				{
					current += 0.01f;
					PlanC[i] = 1.0f - EaseOutBounce(current);
				}

				PlotLines("Plan C", PlanC, 100, 0, 0, 3.4028235E38F, 3.4028235E38F, { 100,100 }, 4);

				EndTabItem();
			}
			if (BeginTabItem("Change Scene"))
			{
				static int sceneNum = 0;
				if (ArrowButton("上", ImGuiDir_Up))
					sceneNum++;
				SameLine();
				if (ArrowButton("下", ImGuiDir_Down))
					sceneNum--;
				if (sceneNum < 0)sceneNum = static_cast<int>(SceneManager::SceneType::SCENE_MAX) - 1;
				sceneNum = sceneNum % static_cast<int>(SceneManager::SceneType::SCENE_MAX);
				SceneManager::SceneType changeScene;
				switch (sceneNum)
				{
				case 0:
					sceneTxt = "Title";
					changeScene = SceneManager::SceneType::SCENE_TITLE;
					break;
				case 1:
					sceneTxt = "Game";
					changeScene = SceneManager::SceneType::SCENE_GAME;
					break;
				case 2:
					sceneTxt = "Result";
					changeScene = SceneManager::SceneType::SCENE_RESULT;
					break;
				case 3:
					sceneTxt = "3DEditor";
					changeScene = SceneManager::SceneType::SCENE_3DEDITOR;
					break;
				default:
					sceneTxt = "Unknown";
					break;
				}
				sceneTxt = "Change Scene : " + sceneTxt;
				SameLine();
				Text(sceneTxt.c_str());
				if (Button("Accept"))
					SceneManager::ChangeScene(changeScene);

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
		Separator();
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

	SceneManager::Draw();
	EndDrawDirectX();
}

// EOF

