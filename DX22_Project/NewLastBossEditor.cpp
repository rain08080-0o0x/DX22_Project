#include "NewLastBossEditor.h"
#include "Geometory.h"
#include "Input.h"
#include "CameraDebug.h"
#include "ShaderList.h"
#include "imgui.h"
#include <math.h>
#include <string>

using namespace std;

#define PI (3.141592635f)

namespace
{
	// 2点間の距離を求める関数
	DirectX::XMFLOAT2 Squared(DirectX::XMFLOAT2 x1, DirectX::XMFLOAT2 x2)
	{
		return DirectX::XMFLOAT2(x2.x - x1.x, x2.y - x1.y);
	}
}

void DrawCharSprite(Texture* texture, Camera* camera, const CharInfo& info, const DirectX::XMFLOAT4& color)
{
	if (!texture)
	{
		return;
	}

	using namespace DirectX;

	XMMATRIX facing = XMMatrixIdentity();
	if (info.useBillboard)
	{
		if (camera)
		{
			const XMFLOAT4X4 viewFloat = camera->GetViewMatrix(false);
			const XMMATRIX viewMatrix = XMLoadFloat4x4(&viewFloat);
			const XMMATRIX inverseView = XMMatrixInverse(nullptr, viewMatrix);

			XMFLOAT4X4 inverseViewFloat{};
			XMStoreFloat4x4(&inverseViewFloat, inverseView);
			inverseViewFloat._41 = 0.0f;
			inverseViewFloat._42 = 0.0f;
			inverseViewFloat._43 = 0.0f;
			facing = XMLoadFloat4x4(&inverseViewFloat);
		}
	}
	else
	{
		facing = XMMatrixRotationY(XMConvertToRadians(info.yawDeg));
	}

	const XMMATRIX worldMatrix = facing * XMMatrixTranslation(
		info.pos.x,
		info.pos.y + (info.size.y * 0.5f),
		info.pos.z);
	XMFLOAT4X4 world{};
	XMStoreFloat4x4(&world, XMMatrixTranspose(worldMatrix));

	Sprite::SetWorld(world);
	Sprite::SetSize({ info.size.x, info.size.y });
	Sprite::SetOffset({ 0.0f, 0.0f });
	Sprite::SetUVPos({ 0.0f, 0.0f });
	Sprite::SetUVScale({ 1.0f, 1.0f });
	Sprite::SetColor(color);
	Sprite::SetTexture(texture);
	SetCullingMode(D3D11_CULL_NONE);
	Sprite::Draw();
	SetCullingMode(D3D11_CULL_BACK);
}

void DrawCharCollision(CharInfo set,DirectX::XMFLOAT4 color)
{
	set.size.x /= 2.0f;
	set.size.y /= 2.0f;
	set.size.z /= 2.0f;
	Geometory::AddLine(
		{set.pos.x - set.size.x,set.pos.y,set.pos.z - set.size.z},
		{set.pos.x + set.size.x,set.pos.y,set.pos.z - set.size.z}, 
		color);
	Geometory::AddLine(
		{set.pos.x - set.size.x,set.pos.y,set.pos.z + set.size.z},
		{set.pos.x + set.size.x,set.pos.y,set.pos.z + set.size.z}, 
		color);
	Geometory::AddLine(
		{set.pos.x - set.size.x,set.pos.y,set.pos.z - set.size.z},
		{set.pos.x - set.size.x,set.pos.y,set.pos.z + set.size.z}, 
		color);
	Geometory::AddLine(
		{set.pos.x + set.size.x,set.pos.y,set.pos.z - set.size.z},
		{set.pos.x + set.size.x,set.pos.y,set.pos.z + set.size.z}, 
		color);
}

void DrawAttackRange(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end, float width, DirectX::XMFLOAT4 color)
{
	DirectX::XMFLOAT3 vertex[4];
	float angle = atan2f(end.z - start.z,end.x - start.x);
	float
		startX = width * cosf(angle + (PI / 2.0f)),
		startZ = width * sinf(angle + (PI / 2.0f)),
		endX = width * cosf(angle - (PI / 2.0f)),
		endZ = width * sinf(angle - (PI / 2.0f));
	vertex[0] = { start.x + startX,0.1f,start.z + startZ };
	vertex[1] = { start.x - startX,0.1f,start.z - startZ };
	vertex[2] = { end.x + endX,0.1f,end.z + endZ };
	vertex[3] = { end.x - endX,0.1f,end.z - endZ };
	Geometory::AddLine(vertex[0], vertex[1], color);
	Geometory::AddLine(vertex[1], vertex[2], color);
	Geometory::AddLine(vertex[2], vertex[3], color);
	Geometory::AddLine(vertex[3], vertex[0], color);
}

/// <summary>
/// 攻撃範囲を描画する関数
/// </summary>
/// <param name="start">始点座標</param>
/// <param name="end">終点座標</param>
/// <param name="width">幅</param>
/// <param name="tex">テクスチャ</param>
/// <param name="progress">0.0f～1.0fで攻撃発生までの予兆</param>
void DrawAttackRange(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end, float width, Texture* tex,float progress)
{
	using namespace DirectX;
	float angle = atan2f(end.z - start.z, end.x - start.x);
	DirectX::XMFLOAT2 start2D, end2D;
	start2D = { start.x,start.z };
	end2D = { end.z,end.y };
	XMVECTOR a = XMLoadFloat2(&start2D);
	XMVECTOR b = XMLoadFloat2(&end2D);

	XMVECTOR diff = XMVectorSubtract(b, a);
	XMVECTOR len = XMVector2Length(diff);

	float distance = XMVectorGetX(len);

	DirectX::XMFLOAT4X4 world;
	DirectX::XMMATRIX mat =
		DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(90)) *
		DirectX::XMMatrixRotationY(-angle) *
		DirectX::XMMatrixTranslation(
			start.x + (end.x - start.x) * progress * 0.5f ,
			0.11f,
			start.z + (end.z - start.z) * progress * 0.5f
		);
	mat = DirectX::XMMatrixTranspose(mat);
	DirectX::XMStoreFloat4x4(&world, mat);

	Sprite::SetWorld(world);
	Sprite::SetSize({ distance * progress,width });
	Sprite::SetOffset({0,0});
	Sprite::SetUVPos({ 0,0 });
	Sprite::SetUVScale({ 1,1 });
	Sprite::SetColor({ 1.0f,0,0,0.5f });
	Sprite::SetTexture(tex);
	Sprite::Draw();
}
void DrawAttackRange(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end, float width, Texture* tex)
{
	using namespace DirectX;
	float angle = atan2f(end.z - start.z, end.x - start.x);
	DirectX::XMFLOAT2 start2D, end2D;
	start2D = { start.x,start.z };
	end2D = { end.z,end.y };
	XMVECTOR a = XMLoadFloat2(&start2D);
	XMVECTOR b = XMLoadFloat2(&end2D);

	XMVECTOR diff = XMVectorSubtract(b, a);
	XMVECTOR len = XMVector2Length(diff);

	float distance = XMVectorGetX(len);

	DirectX::XMFLOAT4X4 world;
	DirectX::XMMATRIX mat =
		DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(90)) *
		DirectX::XMMatrixRotationY(-angle) *
		DirectX::XMMatrixTranslation(
			(start.x + end.x) * 0.5f,
			0.1f,
			(start.z + end.z) * 0.5f
		);
	mat = DirectX::XMMatrixTranspose(mat);
	DirectX::XMStoreFloat4x4(&world, mat);

	Sprite::SetWorld(world);
	Sprite::SetSize({distance,width});
	Sprite::SetOffset({0,0});
	Sprite::SetUVPos({ 0,0 });
	Sprite::SetUVScale({ 1,1 });
	Sprite::SetColor({ 0.25f,0,0,0.5f });
	Sprite::SetTexture(tex);
	Sprite::Draw();
}

/// <summary>
/// 攻撃範囲を描画する関数
/// </summary>
/// <param name="start">始点座標</param>
/// <param name="end">終点座標</param>
/// <param name="width">幅</param>
/// <param name="height">長さ</param>
/// <param name="tex">テクスチャ</param>
/// <param name="progress">0.0f～1.0fで攻撃発生までの予兆</param>
void DrawAttackRange(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end, float width, float height,Texture* tex,float progress)
{
	using namespace DirectX;
	float angle = atan2f(end.z - start.z, end.x - start.x);
	DirectX::XMFLOAT2 start2D, end2D;
	start2D = { start.x,start.z };
	end2D = { end.z,end.y };
	XMVECTOR a = XMLoadFloat2(&start2D);
	XMVECTOR b = XMLoadFloat2(&end2D);

	XMVECTOR diff = XMVectorSubtract(b, a);
	XMVECTOR len = XMVector2Length(diff);

	float distance = height;

	DirectX::XMFLOAT4X4 world;
	DirectX::XMMATRIX mat =
		DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(90)) *
		DirectX::XMMatrixRotationY(-angle) *
		DirectX::XMMatrixTranslation(
			start.x + (end.x - start.x) * progress * 0.5f ,
			0.11f,
			start.z + (end.z - start.z) * progress * 0.5f
		);
	mat = DirectX::XMMatrixTranspose(mat);
	DirectX::XMStoreFloat4x4(&world, mat);

	Sprite::SetWorld(world);
	Sprite::SetSize({ distance * progress,width });
	Sprite::SetOffset({0,0});
	Sprite::SetUVPos({ 0,0 });
	Sprite::SetUVScale({ 1,1 });
	Sprite::SetColor({ 1.0f,0,0,0.5f });
	Sprite::SetTexture(tex);
	Sprite::Draw();
}
void DrawAttackRange(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end, float width, float height,Texture* tex)
{
	using namespace DirectX;
	float angle = atan2f(end.z - start.z, end.x - start.x);
	DirectX::XMFLOAT2 start2D, end2D;
	start2D = { start.x,start.z };
	end2D = { end.z,end.y };
	XMVECTOR a = XMLoadFloat2(&start2D);
	XMVECTOR b = XMLoadFloat2(&end2D);

	XMVECTOR diff = XMVectorSubtract(b, a);
	XMVECTOR len = XMVector2Length(diff);

	float distance = XMVectorGetX(len);
	XMFLOAT2 diff2D = Squared(start2D,end2D);
	distance = sqrtf(abs(diff2D.x) + abs(diff2D.y));

	DirectX::XMFLOAT4X4 world;
	DirectX::XMMATRIX mat =
		DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(90)) *
		DirectX::XMMatrixRotationY(-angle) *
		DirectX::XMMatrixTranslation(
			(start.x + end.x) * 0.5f,
			0.1f,
			(start.z + end.z) * 0.5f
		);
	mat = DirectX::XMMatrixTranspose(mat);
	DirectX::XMStoreFloat4x4(&world, mat);

	Sprite::SetWorld(world);
	Sprite::SetSize({ height,width});
	Sprite::SetOffset({0,0});
	Sprite::SetUVPos({ 0,0 });
	Sprite::SetUVScale({ 1,1 });
	Sprite::SetColor({ 0.25f,0,0,0.5f });
	Sprite::SetTexture(tex);
	Sprite::Draw();
}

/// <summary>
/// 第一引数にその中心座標を渡し,第二引数にその攻撃の幅を定義。第三引数のその攻撃長さを定義。
/// 第四引数に角度を定義。この角度、0～2PI
/// </summary>
/// <param name="   pos : ">中心位置</param>
/// <param name=" width : ">幅</param>
/// <param name="length : ">長さ</param>
/// <param name=" angle : ">角度</param>
void DrawAttackRange(DirectX::XMFLOAT3 pos,float width,float length,float angle)
{
	using namespace DirectX;
	XMFLOAT3 vertex[4];
	XMFLOAT2 start2D,end2D;

	start2D = { pos.x - (length * cosf(angle)),pos.y - (length * sinf(angle)) };
	end2D = { pos.x + (length * cosf(angle)),pos.y + (length * sinf(angle)) };

	vertex[0] = {
		start2D.x + width * cosf(angle + (PI / 2.0f)),
		0.1f,
		start2D.y + width * sinf(angle + (PI / 2.0f))
	};
	vertex[1] = {
		end2D.x + width * cosf(angle + (PI / 2.0f)),
		0.1f,
		end2D.y + width * sinf(angle + (PI / 2.0f))
	};
	vertex[2] = {
		end2D.x - width * cosf(angle + (PI / 2.0f)),
		0.1f,
		end2D.y - width * sinf(angle + (PI / 2.0f))
	};
	vertex[3] = {
		start2D.x - width * cosf(angle + (PI / 2.0f)),
		0.1f,
		start2D.y - width * sinf(angle + (PI / 2.0f))
	};
	Geometory::AddLine(vertex[0], vertex[1], { 0,1,0,1 });
	Geometory::AddLine(vertex[1], vertex[2], { 0,1,0,1 });
	Geometory::AddLine(vertex[2], vertex[3], { 0,1,0,1 });
	Geometory::AddLine(vertex[3], vertex[0], { 0,1,0,1 });
}
/// <summary>
/// 第一引数にその中心座標を渡し,第二引数にその攻撃の幅を定義。第三引数のその攻撃長さを定義。
/// 第四引数にテクスチャを。
/// </summary>
/// <param name="   pos : ">中心位置</param>
/// <param name=" width : ">幅</param>
/// <param name="length : ">長さ</param>
/// <param name=" angle : ">角度</param>
/// <param name="   tex : ">テクスチャ</param>
void DrawAttackRange(DirectX::XMFLOAT3 pos, float width, float length, float angle, Texture* tex)
{

}
/// <summary>
/// 第一引数にその中心座標を渡し,第二引数にその攻撃の幅を定義。第三引数のその攻撃長さを定義。
/// 第四引数にテクスチャを。第五引数に攻撃発生までの予兆を0～1で。
/// </summary>
/// <param name="     pos : ">中心位置</param>
/// <param name="   width : ">幅</param>
/// <param name="  length : ">長さ</param>
/// <param name=" angle : ">角度</param>
/// <param name="     tex : ">テクスチャ</param>
/// <param name="progress : ">予兆までの達成率</param>
void DrawAttackRange(DirectX::XMFLOAT3 pos, float width, float length, float angle,Texture* tex,float progress)
{

}

void NewLastBoss::DrawDebugGUI()
{
	using namespace ImGui;
	Begin("Main");
	// Player
	float ppos[3] = { player.pos.x, player.pos.y, player.pos.z };
	float spos[3] = { player.size.x, player.size.y, player.size.z };
	int playerFacing = player.useBillboard ? 0 : 1;
	const char* facingItems[] = { u8"Billboard", u8"World" };
	// Boss
	float pboss[3] = { boss.pos.x, boss.pos.y, boss.pos.z };
	float sboss[3] = { boss.size.x, boss.size.y, boss.size.z };
	int bossFacing = boss.useBillboard ? 0 : 1;
	// Camera
	float CamPos[3] = { camera->GetPos().x,camera->GetPos().y,camera->GetPos().z };
	if (ImGui::CollapsingHeader(u8"プレイヤー", ImGuiTreeNodeFlags_DefaultOpen))
	{
		SeparatorText(u8"プレイヤー");
		DragFloat3(u8"位置", ppos, 0.05f);
		DragFloat3(u8"サイズ", spos, 0.05f, 0.05f, 20.0f);
		DragFloat(u8"プレイヤースピード", &playerSpeed,0.01f);
		Combo(u8"向き", &playerFacing, facingItems, IM_ARRAYSIZE(facingItems));
		if (playerFacing == 1)
		{
			DragFloat(u8"Y角度", &player.yawDeg, 1.0f, -360.0f, 360.0f);
		}
		SeparatorText(u8"ボス");
		PushID(1);
		DragFloat3(u8"位置", pboss, 0.05f);
		DragFloat3(u8"サイズ", sboss, 0.05f, 0.05f, 20.0f);
		Combo(u8"向き", &bossFacing, facingItems, IM_ARRAYSIZE(facingItems));
		if (bossFacing == 1)
		{
			DragFloat(u8"Y角度", &boss.yawDeg, 1.0f, -360.0f, 360.0f);
		}
		PopID();
	}

	if (ImGui::CollapsingHeader(u8"カメラ", ImGuiTreeNodeFlags_DefaultOpen))
	{
		PushID(2);
		DragFloat3(u8"位置", CamPos, 0.01f);
		camera->SetPos({ CamPos[0],CamPos[1],CamPos[2] });
		PopID();
	}
	if (ImGui::CollapsingHeader(u8"デバッグ", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginTabBar(u8"デバッグ"))
		{
			if (ImGui::BeginTabItem(u8"ランダムスラッシュ"))
			{
				if (Button(u8"RandomSlash"))
				{
					for (int i = 0; i < 10; i++)
					{
						rs_attack[i].isUsed = SlashState::UnUsed;
						rs_attack[i].angle = 0.0f;
						rs_attack[i].frame = 0;
					}
				}

				if (ImGui::TreeNode(u8"ランダムスラッシュ サイズとか"))
				{
					const int max = 5;
					// テーブルの開始
					// 第1引数: 文字列ID, 第2引数: 列数, 第3引数: フラグ（枠線やストライプなど）
					if (ImGui::BeginTable("table_example", max, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
					{
						rs_attack[0].angle;
						rs_attack[0].frame;
						rs_attack[0].isUsed;
						// ヘッダーの設定
						ImGui::TableSetupColumn(u8"Ｎ個目");
						ImGui::TableSetupColumn(u8"使用中か");
						ImGui::TableSetupColumn(u8"フレーム数");
						ImGui::TableSetupColumn(u8"角度");
						ImGui::TableSetupColumn(u8"サイズ");
						ImGui::TableHeadersRow(); // これを呼ぶとヘッダーが描画される

						// データの描画（例として3行分）
						for (int i = 0; i < 10; i++)
						{
							ImGui::TableNextRow();// 次の行に移動
							//int counter = 0;
							string isActiveText;
							float distance;
							PushID(i);
							for (int counter = 0; counter < max; counter++)
							{
								ImGui::TableSetColumnIndex(counter); // counter列目に移動

								switch (counter)
								{
								case 0:
									ImGui::Text(u8"%d個目", i);
									break;
								case 1:
									switch (rs_attack[i].isUsed)
									{
									case SlashState::UnUsed:isActiveText = u8"未使用";
										break;
									case SlashState::Useing:isActiveText = u8"使用中";
										break;
									case SlashState::Used:isActiveText = u8"使用済";
										break;
									default:
										break;
									}
									ImGui::Text(isActiveText.c_str());
									break;
								case 2:
									ImGui::Text(u8"現在フレーム数:%d", rs_attack[i].frame);
									break;
								case 3:
									ImGui::Text(u8"角度:%3.2f", DirectX::XMConvertToDegrees(rs_attack[i].angle));
									break;
								case 4:
									DirectX::XMFLOAT2 start2D = { rs_attack[i].start.x,rs_attack[i].start.z };
									DirectX::XMFLOAT2 end2D = { rs_attack[i].end.x,rs_attack[i].end.z };
									DirectX::XMVECTOR a = DirectX::XMLoadFloat2(&start2D);
									DirectX::XMVECTOR b = DirectX::XMLoadFloat2(&end2D);

									DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(b, a);
									DirectX::XMVECTOR len = DirectX::XMVector2Length(diff);

									distance = DirectX::XMVectorGetX(len);
									ImGui::Text(u8"攻撃範囲：%2.2f", distance);
									break;
								default:
									break;
								}
							}
							PopID();
						}

						ImGui::EndTable(); // テーブルの終了
					}
					ImGui::TreePop();
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(u8"格子状攻撃"))
			{
				if (Button(u8"格子状"))
				{
					count = 0;
					crossState = First;
				}
				string text = u8"何もなし";
				switch (crossState)
				{
				case NewLastBoss::First:text = u8"最初";
					break;
				case NewLastBoss::Second:text = u8"二回";
					break;
				case NewLastBoss::Final:text = u8"最後";
					break;
				case NewLastBoss::Max:
				default:text = u8"以外";
					break;
				}
				ImGui::Text(text.c_str());
				ImGui::Text(u8"現在フレーム数:%d", count);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(u8"P依存型攻撃"))
			{
				string ccStateText = u8"None";
				switch (cc.state)
				{
				case AttackState::None:ccStateText = u8"な  し";
					break;
				case AttackState::UnUsed:ccStateText = u8"未使用";
					break;
				case AttackState::Using:ccStateText = u8"使用中";
					break;
				case AttackState::Used:ccStateText = u8"使用済";
					break;
				default:
					break;
				}
				ImGui::TextDisabled(ccStateText.c_str());
				ImGui::TextDisabled(u8"最初の角度：%3.2f°", DirectX::XMConvertToDegrees(cc.firstAngle));
				ImGui::TextDisabled(u8"第二の角度：%3.2f°", DirectX::XMConvertToDegrees(cc.secondAngle));
				ImGui::TextDisabled(u8"幅：%3.2fｍ", cc.width);
				ImGui::TextDisabled(u8"長さ：%3.2fｍ", cc.length);
				Text(u8"まだナニモナイヨ");

				ImGui::EndTabItem();
			}


			ImGui::EndTabBar();
		}
		SeparatorText(u8"TabBarはここまで");
	}
	End();
	player.pos = { ppos[0], ppos[1], ppos[2] };
	player.size = { spos[0], spos[1], spos[2] };
	player.useBillboard = (playerFacing == 0);
	boss.pos = { pboss[0], pboss[1], pboss[2] };
	boss.size = { sboss[0], sboss[1], sboss[2] };
	boss.useBillboard = (bossFacing == 0);
}


NewLastBoss::NewLastBoss()
	: camera(nullptr)
	, m_pPlayer(nullptr)
	, m_pBoss(nullptr)
{
	Init();
}

NewLastBoss::~NewLastBoss()
{
	Uninit();
}

void NewLastBoss::Init()
{
	camera = new CameraDebug();
	static_cast<CameraDebug*>(camera)->SetPose({ 0.0f, 18.25f, -8.0f }, { 0.0f, 0.0f, 0.0f });

	player.pos = { 0.0f, 0.0f, 3.0f };
	player.size = { 0.5f, 1.0f, 0.5f };
	player.useBillboard = true;
	player.yawDeg = 0.0f;
	playerSpeed = 0.1f;
	boss.pos = { 0.0f, 0.0f, 0.0f };
	boss.size = { 1.5f, 2.0f, 1.0f };
	boss.useBillboard = true;
	boss.yawDeg = 0.0f;
	for (int i = 0; i < 10; i++)
	{
		rs_attack[i].isUsed = SlashState::Used;
		rs_attack[i].frame = 0;
		rs_attack[i].angle = 0.0f;
		rs_attack[i].start = { 0,0,0 };
		rs_attack[i].middle = { 0,0,0 };
		rs_attack[i].end = { 0,0,0 };
	}

	cc.firstAngle = 0.0f;
	cc.secondAngle = 0.0f;
	cc.width = 1.0f;
	cc.state = AttackState::UnUsed;


	m_pPlayer = new Texture();
	if (FAILED(m_pPlayer->Create("Assets/Texture/Effect/Star.png")))
	{
		MessageBoxA(nullptr, "Assets/Texture/Effect/Star.png", "Error", MB_OK);
	}
	m_pBoss = new Texture();
	if (FAILED(m_pBoss->Create("Assets/Texture/Chracter/genbaneko.png")))
	{
		MessageBoxA(nullptr, "Assets/Texture/Chracter/genbaneko.png", "Error", MB_OK);
	}
	m_pPlayer = new Texture();
	if (FAILED(m_pPlayer->Create("Assets/Texture/Effect/Star.png")))
	{
		MessageBoxA(nullptr, "Assets/Texture/Effect/Star.png", "Error", MB_OK);
	}
	m_pFieldtex = new Texture();
	if (FAILED(m_pFieldtex->Create("Assets/Texture/Game/jimen.png")))
	{
		MessageBoxA(nullptr, "Game/jimen.png", "Error", MB_OK);
	}
	m_pAtkTex = new Texture();
	if (FAILED(m_pAtkTex->Create("Assets/Texture/Game/white_dot.png")))
	{
		MessageBoxA(nullptr, "Game/white_dot.png", "error", MB_OK);
	}
}

void NewLastBoss::Uninit()
{
	if (m_pPlayer)
	{
		delete m_pPlayer;
		m_pPlayer = nullptr;
	}
	if (m_pBoss)
	{
		delete m_pBoss;
		m_pBoss = nullptr;
	}
	if (camera)
	{
		delete camera;
		camera = nullptr;
	}
	if (m_pFieldtex)
	{
		delete m_pFieldtex;
		m_pFieldtex = nullptr;
	}
	if (m_pAtkTex)
	{
		delete m_pAtkTex;
		m_pAtkTex = nullptr;
	}
}

void NewLastBoss::Update()
{
	if (camera)
	{
		camera->Update();
	}

	UpdatePlayer();
	UpdateBoss();

	RandomSlashUpdate();
	CrossUpdate();
	CircleCrossUpdate();
}

void NewLastBoss::Draw()
{
	Geometory::AddLine(player.pos, boss.pos, { 0, 1, 0, 1 });

	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 wvp[3];
	DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(0, 0, 0)));

	if (!camera)return;

	wvp[0] = world;
	wvp[1] = camera->GetViewMatrix();
	wvp[2] = camera->GetProjectionMatrix();

	ShaderList::SetWVP(wvp);
	Sprite::SetView(wvp[1]);
	Sprite::SetProjection(wvp[2]);
	Geometory::SetView(wvp[1]);
	Geometory::SetProjection(wvp[2]);

	DrawField();
	DrawCharSprite(m_pPlayer, camera, player, { 1, 1, 1, 1 });
	DrawCharSprite(m_pBoss, camera, boss, { 1, 1, 1, 1 });
	DrawCharCollision(player, {0,1,0,1});
	DrawCharCollision(boss, {0,1,0,1});

	RandomSlashDraw();
	CrossDraw();
	CircleCrossDraw();

	DrawDebugGUI();
}

void NewLastBoss::UpdatePlayer()
{
	if (IsKeyPress('A'))
	{
		player.pos.x -= playerSpeed;
	}
	if (IsKeyPress('D'))
	{
		player.pos.x += playerSpeed;
	}
	if (IsKeyPress('W'))
	{
		player.pos.z += playerSpeed;
	}
	if (IsKeyPress('S'))
	{
		player.pos.z -= playerSpeed;
	}

	if (player.pos.x > 10.0f) player.pos.x = 10.0f;
	if (player.pos.x < -10.0f) player.pos.x = -10.0f;
	if (player.pos.z > 10.0f) player.pos.z = 10.0f;
	if (player.pos.z < -10.0f) player.pos.z = -10.0f;
}

void NewLastBoss::UpdateBoss()
{
}

void NewLastBoss::DrawField()
{
	using namespace DirectX;
	DirectX::XMMATRIX mat = XMMatrixRotationX(XMConvertToRadians(90)) *
		XMMatrixTranslation(0, 0, 0);
	XMFLOAT4X4 world;
	XMStoreFloat4x4(&world, XMMatrixTranspose(mat));
	Sprite::SetWorld(world);
	Sprite::SetSize({20.0f,20.0f});
	Sprite::SetOffset({ 0.0f, 0.0f });
	Sprite::SetUVPos({ 0.0f, 0.0f });
	Sprite::SetUVScale({ 1.0f, 1.0f });
	Sprite::SetColor({ 0.5f,0.5f,0.5f,1 });
	Sprite::SetTexture(m_pFieldtex);
	Sprite::Draw();
}

void NewLastBoss::AddAttack()
{
	for (int i = 0; i < 10; i++)
	{
		//攻撃が発生している場合はスルー
		if (rs_attack[i].isUsed == SlashState::Useing || 
			rs_attack[i].isUsed != SlashState::UnUsed)continue;
		// 攻撃開始
		rs_attack[i].isUsed = SlashState::Useing;
		rs_attack[i].frame = 0;
		// 一周のうちランダムな角度を生成
		rs_attack[i].angle = rand() / float(RAND_MAX) * 2 * PI;
		// プレイヤーの位置を基準にランダムに生成した角度から反対側までの位置を攻撃範囲にする
		rs_attack[i].start = { player.pos.x + (2 * cosf(rs_attack[i].angle)),0.1f,player.pos.z + (2 * sinf(rs_attack[i].angle)) };
		rs_attack[i].end = { player.pos.x + (2 * cosf(rs_attack[i].angle + PI)),0.1f,player.pos.z + (2 * sinf(rs_attack[i].angle + PI)) };
		rs_attack[i].middle = player.pos;
		return;
	}
}

void NewLastBoss::RandomSlashUpdate()
{
	static int counter;
	counter++;
	// 30フレーム毎に攻撃を発生。合計10個生成で生成終了
	if (counter % 40 == 0)
	{
		AddAttack();
	}

	for (int i = 0; i < 10; i++)
	{
		switch (rs_attack[i].isUsed)
		{
		case SlashState::UnUsed:
			break;
		case SlashState::Useing:
			// 攻撃が発生後1秒経過で攻撃終了
			if (rs_attack[i].frame >= 120)
			{
				rs_attack[i].angle = 0.0f;
				rs_attack[i].frame = 0;
				rs_attack[i].start = { 0,0,0 };
				rs_attack[i].middle = { 0,0,0 };
				rs_attack[i].end = { 0,0,0 };
				rs_attack[i].isUsed = SlashState::Used;
				return;
			}
			// 攻撃更新
				rs_attack[i].frame++;
			break;
		case SlashState::Used:
			break;
		default:
			break;
		}
	}
}

void NewLastBoss::RandomSlashDraw()
{

	for (int i = 0; i < 10; i++)
	{
		if (rs_attack[i].isUsed == SlashState::Useing)
		{
			DrawAttackRange(rs_attack[i].start, rs_attack[i].end, 0.25f, { 0,1,0,1 });
			DrawAttackRange(rs_attack[i].start, rs_attack[i].end, 0.5f, 4.0f, m_pAtkTex);
			DrawAttackRange(rs_attack[i].start, rs_attack[i].end, 0.5f, 4.0f, m_pAtkTex, rs_attack[i].frame / 120.0f);
		}
	}
}

void NewLastBoss::CrossUpdate()
{
	if (crossState == Max)return;
	count++;
	if (count >= 240)
	{
		count = 0;
		crossState = CrossState(int(crossState) + 1);
	}
}

void NewLastBoss::CrossDraw()
{
	if (count >= 120)return;
		float width = 2.0f;
		float len2 = sqrtf(2.0f);
		float height = 10.0f * sqrtf(2.0f);
	
	switch (crossState)
	{
	case First:
		for (int x = 0; x <= 10; x++)
		{
			if (x < 6)
			{
				DrawAttackRange(
					{ -10.0f + (4 * x),0.1f, -10.0f },
					{ -10.0f,0.1f,-10.0f + (4 * x) },
					0.25f,
					{ 0,1,0,1 });
				DrawAttackRange(
					{ -10.0f + (4 * x),0.1f, 10.0f },
					{ -10.0f,0.1f,10.0f - (4 * x) },
					0.25f,
					{ 0,1,0,1 });

				DrawAttackRange(
					{ -10.0f + (4 * x),0.1f, -10.0f },
					{ -10.0f,0.1f,-10.0f + (4 * x) },
					0.5f,
					((x) * 4)* len2,
					m_pAtkTex);
				DrawAttackRange(
					{ -10.0f + (4 * x),0.1f, -10.0f },
					{ -10.0f,0.1f,-10.0f + (4 * x) },
					0.5f,
					((x) * 4)* len2,
					m_pAtkTex,count / 120.0f);

				DrawAttackRange(
					{ -10.0f + (4 * x),0.1f, 10.0f },
					{ -10.0f,0.1f,10.0f - (4 * x) },
					0.5f,
					((x) * 4)* len2,
					m_pAtkTex);
				DrawAttackRange(
					{ -10.0f + (4 * x),0.1f, 10.0f },
					{ -10.0f,0.1f,10.0f - (4 * x) },
					0.5f,
					((x) * 4)* len2,
					m_pAtkTex,count / 120.0f);
			}
			else
			{
				DrawAttackRange(
					{ -10.0f + (4 * (x - 5)),0.1f, 10.0f },
					{ 10.0f,0.1f,-10.0f + (4 * (x - 5)) },
					0.25f,
					{ 0,1,0,1 });
				DrawAttackRange(
					{ -10.0f + (4 * (x - 5)),0.1f, -10.0f },
					{ 10.0f,0.1f,10.0f - (4 * (x - 5)) },
					0.25f,
					{ 0,1,0,1 });

				DrawAttackRange(
					{ -10.0f + (4 * (x - 5)),0.1f, 10.0f },
					{ 10.0f,0.1f,-10.0f + (4 * (x - 5)) },
					0.5f,
					((10 -x) * 4)* len2,
					m_pAtkTex);
				DrawAttackRange(
					{ -10.0f + (4 * (x - 5)),0.1f, 10.0f },
					{ 10.0f,0.1f,-10.0f + (4 * (x - 5)) },
					0.5f,
					((10 - x) * 4)* len2,
					m_pAtkTex, count / 120.0f);

				DrawAttackRange(
					{ -10.0f + (4 * (x - 5)),0.1f, -10.0f },
					{ 10.0f,0.1f,10.0f - (4 * (x - 5)) },
					0.5f,
					((10 -x) * 4)* len2,
					m_pAtkTex);
				DrawAttackRange(
					{ -10.0f + (4 * (x - 5)),0.1f, -10.0f },
					{ 10.0f,0.1f,10.0f - (4 * (x - 5)) },
					0.5f,
					((10 - x) * 4)* len2,
					m_pAtkTex, count / 120.0f);
			}
		}
		break;
	case Second:
		for (int x = 0; x < 9; x++)
		{
			if (x < 5)
			{
				DrawAttackRange(
					{ -9.0f + (4 * x),0.1f, -10.0f },
					{ -10.0f,0.1f,-9.0f + (4 * x) },
					0.25f,
					{ 0,1,0,1 });
				DrawAttackRange(
					{ -9.0f + (4 * x),0.1f, 10.0f },
					{ -10.0f,0.1f,9.0f - (4 * x) },
					0.25f,
					{ 0,1,0,1 });

				DrawAttackRange(
					{ -9.0f + (4 * x),0.1f, -10.0f },
					{ -10.0f,0.1f,-9.0f + (4 * x) },
					0.5f,
					(x * 4.0f) * len2,
					m_pAtkTex);
				DrawAttackRange(
					{ -9.0f + (4 * x),0.1f, -10.0f },
					{ -10.0f,0.1f,-9.0f + (4 * x) },
					0.5f,
					(x * 4.0f)* len2,
					m_pAtkTex,count / 120.0f);

				DrawAttackRange(
					{ -9.0f + (4 * x),0.1f, 10.0f },
					{ -10.0f,0.1f,9.0f - (4 * x) },
					0.5f,
					(x * 4.0f) * len2,
					m_pAtkTex);
				DrawAttackRange(
					{ -9.0f + (4 * x),0.1f, 10.0f },
					{ -10.0f,0.1f,9.0f - (4 * x) },
					0.5f,
					(x * 4.0f)* len2,
					m_pAtkTex,count / 120.0f);
			}
			else
			{
				DrawAttackRange(
					{ -9.0f + (4 * (x - 5)),0.1f, 10.0f },
					{ 10.0f,0.1f,-9.0f + (4 * (x - 5)) },
					0.25f,
					{ 0,1,0,1 });
				DrawAttackRange(
					{ -9.0f + (4 * (x - 5)),0.1f, -10.0f },
					{ 10.0f,0.1f,9.0f - (4 * (x - 5)) },
					0.25f,
					{ 0,1,0,1 });

				DrawAttackRange(
					{ -9.0f + (4 * (x - 5)),0.1f, 10.0f },
					{ 10.0f,0.1f,-9.0f + (4 * (x - 5)) },
					0.5f,
					((9.75f - x) * 4.0f)* len2,
					m_pAtkTex);
				DrawAttackRange(
					{ -9.0f + (4 * (x - 5)),0.1f, 10.0f },
					{ 10.0f,0.1f,-9.0f + (4 * (x - 5)) },
					0.5f,
					((9.75f - x) * 4.0f) * len2,
					m_pAtkTex, count / 120.0f);

				DrawAttackRange(
					{ -9.0f + (4 * (x - 5)),0.1f, -10.0f },
					{ 10.0f,0.1f,9.0f - (4 * (x - 5)) },
					0.5f,
					((9.75f - x) * 4.0f)* len2,
					m_pAtkTex);
				DrawAttackRange(
					{ -9.0f + (4 * (x - 5)),0.1f, -10.0f },
					{ 10.0f,0.1f,9.0f - (4 * (x - 5)) },
					0.5f,
					((9.75f - x) * 4.0f) * len2,
					m_pAtkTex, count / 120.0f);
			}
		}
		break;
	case Final:
		// 右肩上がり
		DrawAttackRange(
			{ 0.0f,0.1f,-10.0f },
			{ 10.0f,0.1f,0.0f }, 
			width,
			{ 0,1,0,1 });
		DrawAttackRange(
			{ -10.0f,0.1f,-10.0f },
			{ 10.0f,0.1f,10.0f }, 
			width,
			{ 0,1,0,1 });
		DrawAttackRange(
			{ 0.0f,0.1f,10.0f },
			{ -10.0f,0.1f,0.0f }, 
			width,
			{ 0,1,0,1 });
		// 右肩下がり
		DrawAttackRange(
			{ 0.0f,0.1f,10.0f },
			{ 10.0f,0.1f,0.0f },
			width,
			{ 0,1,0,1 });
		DrawAttackRange(
			{ -10.0f,0.1f,10.0f },
			{ 10.0f,0.1f,-10.0f },
			width,
			{ 0,1,0,1 });
		DrawAttackRange(
			{ -10.0f,0.1f,0.0f },
			{ 0.0f,0.1f,-10.0f },
			width,
			{ 0,1,0,1 });
		//----------------------

		width *= 2.0f;

		// 右肩上がり
		DrawAttackRange(
			{ 0.0f,0.1f,-10.0f },
			{ 10.0f,0.1f,0.0f }, 
			width,
			height,
			m_pAtkTex,count / 120.0f);
		DrawAttackRange(
			{ -10.0f,0.1f,-10.0f },
			{ 10.0f,0.1f,10.0f }, 
			width,
			height * 2.0f,
			m_pAtkTex, count / 120.0f);
		DrawAttackRange(
			{ -10.0f,0.1f,0.0f },
			{ 0.0f,0.1f,10.0f },
			width,
			height,
			m_pAtkTex, count / 120.0f);
		
		DrawAttackRange(
			{ 0.0f,0.1f,-10.0f },
			{ 10.0f,0.1f,0.0f }, 
			width,
			height,
			m_pAtkTex);
		DrawAttackRange(
			{ -10.0f,0.1f,-10.0f },
			{ 10.0f,0.1f,10.0f },
			width,
			height * 2.0f,
			m_pAtkTex);
		DrawAttackRange(
			{ 0.0f,0.1f,10.0f },
			{ -10.0f,0.1f,0.0f },
			width,
			height,
			m_pAtkTex);
		
		// 右肩下がり
		DrawAttackRange(
			{ 0.0f,0.1f,10.0f },
			{ 10.0f,0.1f,0.0f },
			width,
			height,
			m_pAtkTex);
		DrawAttackRange(
			{ -10.0f,0.1f,10.0f },
			{ 10.0f,0.1f,-10.0f },
			width,
			height * 2.0f,
			m_pAtkTex);
		DrawAttackRange(
			{ -10.0f,0.1f,0.0f },
			{ 0.0f,0.1f,-10.0f },
			width,
			height,
			m_pAtkTex);

		DrawAttackRange(
			{ 0.0f,0.1f,10.0f },
			{ 10.0f,0.1f,0.0f },
			width,
			height,
			m_pAtkTex, count / 120.0f);
		DrawAttackRange(
			{ -10.0f,0.1f,10.0f },
			{ 10.0f,0.1f,-10.0f },
			width,
			height * 2.0f,
			m_pAtkTex, count / 120.0f);
		DrawAttackRange(
			{ -10.0f,0.1f,0.0f },
			{ 0.0f,0.1f,-10.0f },
			width,
			height,
			m_pAtkTex, count / 120.0f);
		break;
	case Max:
	default:
		return;
	}

}

void NewLastBoss::CircleCrossUpdate()
{
	// 中心位置からプレイヤー位置に対しての角度を求める

	cc.firstAngle = atan2f(player.pos.z,player.pos.x);
	cc.secondAngle = cc.firstAngle + (PI / 4.0f);

	// 求めた角度の中心を通る直線を一種類目の格子攻撃とし、
	// その直線に対して45度傾けた直線を二種類目の格子攻撃とする

}

void NewLastBoss::CircleCrossDraw()
{
	DrawAttackRange(
		{0,0,0},
		cc.width,
		cc.length,
		cc.firstAngle
	);
	DrawAttackRange(
		{0,0,0},
		cc.width,
		cc.length,
		cc.secondAngle
	);
}

