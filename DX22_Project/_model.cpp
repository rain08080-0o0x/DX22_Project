#include "Model.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void Model::MakeMesh(const void* ptr, float scale, Flip flip)
{
	// 事前準備
	aiVector3D zero3(0.0f, 0.0f, 0.0f);
	aiColor4D one4(1.0f, 1.0f, 1.0f, 1.0f);
	const aiScene* pScene = reinterpret_cast<const aiScene*>(ptr);

	// 反転係数
	float xFlip = (flip == Flip::XFlip) ? -1.0f : 1.0f;
	float zFlip = (flip == Flip::ZFlip || flip == Flip::ZFlipUseAnime) ? -1.0f : 1.0f;

	// 片軸だけ反転 = 鏡映（winding が反転する）
	// xFlip,zFlip は ±1 なので積が -1 のとき鏡映
	bool isMirror = ((xFlip * zFlip) < 0.0f);

	// インデックスの並び（winding）を入れ替えるかどうか
	// いままでのコードは XFlip / ZFlip だけ入れ替えだったが ZFlipUseAnime が漏れるので
	// 「鏡映かどうか」で一意に決める
	int idx1 = isMirror ? 2 : 1;
	int idx2 = isMirror ? 1 : 2;

	// メッシュの作成
	m_meshes.resize(pScene->mNumMeshes);
	for (unsigned int i = 0; i < m_meshes.size(); ++i)
	{
		// 頂点書き込み先の領域を用意
		m_meshes[i].vertices.resize(pScene->mMeshes[i]->mNumVertices);

		// 頂点データの書き込み
		for (unsigned int j = 0; j < m_meshes[i].vertices.size(); ++j)
		{
			// ☆モデルデータから値の取得
			aiVector3D pos = pScene->mMeshes[i]->mVertices[j];
			aiVector3D normal = pScene->mMeshes[i]->HasNormals() ?
				pScene->mMeshes[i]->mNormals[j] : zero3;
			aiVector3D uv = pScene->mMeshes[i]->HasTextureCoords(0) ?
				pScene->mMeshes[i]->mTextureCoords[0][j] : zero3;
			aiColor4D color = pScene->mMeshes[i]->HasVertexColors(0) ?
				pScene->mMeshes[i]->mColors[0][j] : one4;

			// ★重要：位置と同じ反転を法線にも適用
			DirectX::XMFLOAT3 n(
				normal.x * xFlip,
				normal.y,
				normal.z * zFlip
			);

			// ★保険：法線を正規化（長さ0は潰す）
			DirectX::XMVECTOR vn = DirectX::XMVectorSet(n.x, n.y, n.z, 0.0f);
			vn = DirectX::XMVector3LengthSq(vn).m128_f32[0] > 0.0f
				? DirectX::XMVector3Normalize(vn)
				: DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			DirectX::XMStoreFloat3(&n, vn);

			// ☆値を設定
			m_meshes[i].vertices[j] = {
				DirectX::XMFLOAT3(pos.x * scale * xFlip, pos.y * scale, pos.z * scale * zFlip),
				n,
				DirectX::XMFLOAT2(uv.x, uv.y),
				DirectX::XMFLOAT4(color.r, color.g, color.b, color.a)
			};
		}

		// ボーン生成
		MakeWeight(pScene, i);

		// インデックスの書き込み先の用意
		// mNumFacesはポリゴンの数を表す(１ポリゴンで3インデックス)
		m_meshes[i].indices.resize(pScene->mMeshes[i]->mNumFaces * 3);

		// インデックスの書き込み
		for (unsigned int j = 0; j < pScene->mMeshes[i]->mNumFaces; ++j)
		{
			// ☆モデルデータから値の取得
			aiFace face = pScene->mMeshes[i]->mFaces[j];

			// 念のため（三角形以外が来たら無視／ゼロ埋め）
			int idx = j * 3;
			if (face.mNumIndices < 3)
			{
				m_meshes[i].indices[idx + 0] = 0;
				m_meshes[i].indices[idx + 1] = 0;
				m_meshes[i].indices[idx + 2] = 0;
				continue;
			}

			// ☆値の設定（鏡映なら 1 と 2 を入れ替える）
			m_meshes[i].indices[idx + 0] = face.mIndices[0];
			m_meshes[i].indices[idx + 1] = face.mIndices[idx1];
			m_meshes[i].indices[idx + 2] = face.mIndices[idx2];
		}

		// マテリアルの割り当て
		m_meshes[i].materialID = pScene->mMeshes[i]->mMaterialIndex;

		// ☆頂点バッファに必要なデータを設定
		MeshBuffer::Description desc = {};
		desc.pVtx = m_meshes[i].vertices.data();
		desc.vtxSize = sizeof(Vertex);
		desc.vtxCount = m_meshes[i].vertices.size();
		desc.pIdx = m_meshes[i].indices.data();
		desc.idxSize = sizeof(unsigned long);
		desc.idxCount = m_meshes[i].indices.size();
		desc.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		// ☆頂点バッファ作成
		m_meshes[i].pMesh = new MeshBuffer();
		m_meshes[i].pMesh->Create(desc);
	}
}
void Model::MakeMaterial(const void* ptr, std::string directory)
{
	// 事前準備
	aiColor3D color(0.0f, 0.0f, 0.0f);
	float shininess = 0.0f;
	const aiScene* pScene = reinterpret_cast<const aiScene*>(ptr);

	// マテリアルの作成
	m_materials.resize(pScene->mNumMaterials);
	for (unsigned int i = 0; i < m_materials.size(); ++i)
	{
		//--- 各種マテリアルパラメーターの読み取り
		// ☆拡散光の読み取り
		if (pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
			m_materials[i].diffuse = DirectX::XMFLOAT4(color.r, color.g, color.b, 1.0f);
		else
			m_materials[i].diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		// ☆環境光の読み取り
		if (pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS)
			m_materials[i].ambient = DirectX::XMFLOAT4(color.r, color.g, color.b, 1.0f);
		else
			m_materials[i].ambient = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
		// ☆反射光の読み取り
		if (pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
			m_materials[i].specular = DirectX::XMFLOAT4(color.r, color.g, color.b, 0.0f);
		else
			m_materials[i].specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		// ☆反射光の強さを読み取り
		if (pScene->mMaterials[i]->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
			m_materials[i].specular.w = shininess;

		// テクスチャ読み込み処理
		HRESULT hr;
		aiString path;

		// テクスチャのパス情報を読み込み
		m_materials[i].pTexture = nullptr;
		if (pScene->mMaterials[i]->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path) != AI_SUCCESS) {
			continue;
		}

		// テクスチャ領域確保
		m_materials[i].pTexture = new Texture;

		// そのまま読み込み
		hr = m_materials[i].pTexture->Create(path.C_Str());
		if (SUCCEEDED(hr)) { continue; }

		// ディレクトリと連結して探索
		hr = m_materials[i].pTexture->Create((directory + path.C_Str()).c_str());
		if (SUCCEEDED(hr)) { continue; }

		// モデルと同じ階層を探索
		// パスからファイル名のみ取得
		std::string fullPath = path.C_Str();
		std::string::iterator strIt = fullPath.begin();
		while (strIt != fullPath.end()) {
			if (*strIt == '/')
				*strIt = '\\';
			++strIt;
		}
		size_t find = fullPath.find_last_of("\\");
		std::string fileName = fullPath;
		if (find != std::string::npos)
			fileName = fileName.substr(find + 1);
		// テクスチャの読込
		hr = m_materials[i].pTexture->Create((directory + fileName).c_str());
		if (SUCCEEDED(hr)) { continue; }

		// テクスチャが見つからなかった
		delete m_materials[i].pTexture;
		m_materials[i].pTexture = nullptr;
#ifdef _DEBUG
		m_errorStr += path.C_Str();
#endif
	}
}