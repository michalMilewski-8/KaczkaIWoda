#include "mesh.h"
#include <algorithm>
#include <fstream>
using namespace std;
using namespace mini;
using namespace DirectX;

Mesh::Mesh()
	: m_indexCount(0), m_primitiveType(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
{ }

Mesh::Mesh(dx_ptr_vector<ID3D11Buffer>&& vbuffers, vector<unsigned int>&& vstrides, vector<unsigned int>&& voffsets,
	dx_ptr<ID3D11Buffer>&& indices, unsigned int indexCount, D3D_PRIMITIVE_TOPOLOGY primitiveType)
{
	assert(vbuffers.size() == voffsets.size() && vbuffers.size() == vstrides.size());
	m_indexCount = indexCount;
	m_primitiveType = primitiveType;
	m_indexBuffer = move(indices);

	m_vertexBuffers = std::move(vbuffers);
	m_strides = move(vstrides);
	m_offsets = move(voffsets);
}

Mesh::Mesh(Mesh&& right) noexcept
	: m_indexBuffer(move(right.m_indexBuffer)), m_vertexBuffers(move(right.m_vertexBuffers)),
	m_strides(move(right.m_strides)), m_offsets(move(right.m_offsets)),
	m_indexCount(right.m_indexCount), m_primitiveType(right.m_primitiveType), indices_(), vertex_(), wszystko(right.wszystko)
{
	for (auto& ver : right.vertex_)
		vertex_.push_back(ver);
	for (auto& ver : right.indices_)
		indices_.push_back(ver);
	right.Release();
}

void Mesh::Release()
{
	m_vertexBuffers.clear();
	m_strides.clear();
	m_offsets.clear();
	m_indexBuffer.reset();
	m_indexCount = 0;
	m_primitiveType = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

Mesh& Mesh::operator=(Mesh&& right) noexcept
{
	Release();
	m_vertexBuffers = move(right.m_vertexBuffers);
	m_indexBuffer = move(right.m_indexBuffer);
	m_strides = move(right.m_strides);
	m_offsets = move(right.m_offsets);
	m_indexCount = right.m_indexCount;
	m_primitiveType = right.m_primitiveType;
	wszystko = right.wszystko;
	for (auto& ver : right.vertex_)
		vertex_.push_back(ver);
	for (auto& ver : right.indices_)
		indices_.push_back(ver);
	right.Release();
	return *this;
}

void Mesh::Render(const dx_ptr<ID3D11DeviceContext>& context) const
{
	if (!m_indexBuffer || m_vertexBuffers.empty())
		return;
	context->IASetPrimitiveTopology(m_primitiveType);
	context->IASetIndexBuffer(m_indexBuffer.get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetVertexBuffers(0, m_vertexBuffers.size(), m_vertexBuffers.data(), m_strides.data(), m_offsets.data());
	context->DrawIndexed(m_indexCount, 0, 0);
}

Mesh::~Mesh()
{
	Release();
}

std::vector<VertexPositionColor> mini::Mesh::ColoredBoxVerts(float width, float height, float depth)
{
	return {
		//Front Face
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 0.0f } },
		{ { +0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 0.0f } },

		//Back Face
		{ { +0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 1.0f } },
		{ { -0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 1.0f } },
		{ { -0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 1.0f } },
		{ { +0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 1.0f } },

		//Left Face
		{ { -0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 0.0f } },
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 1.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, -0.5f * depth }, { 0.0f, 1.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 0.0f } },

		//Right Face
		{ { +0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, -0.5f * height, +0.5f * depth }, { 1.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, +0.5f * height, +0.5f * depth }, { 1.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, +0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 1.0f } },

		//Bottom Face
		{ { -0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 0.0f, 1.0f } },
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 0.0f, 1.0f } },

		//Top Face
		{ { -0.5f * width, +0.5f * height, -0.5f * depth }, { 1.0f, 1.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, -0.5f * depth }, { 1.0f, 1.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, +0.5f * depth }, { 1.0f, 1.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, +0.5f * depth }, { 1.0f, 1.0f, 0.0f } }
	};
}

std::vector<VertexPositionNormal> mini::Mesh::ShadedBoxVerts(float width, float height, float depth)
{
	return {
		//Front Face
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 0.0f, -1.0f } },
		{ { +0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 0.0f, -1.0f } },
		{ { +0.5f * width, +0.5f * height, -0.5f * depth }, { 0.0f, 0.0f, -1.0f } },
		{ { -0.5f * width, +0.5f * height, -0.5f * depth }, { 0.0f, 0.0f, -1.0f } },

		//Back Face
		{ { +0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 0.0f, 1.0f } },
		{ { -0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 0.0f, 1.0f } },
		{ { -0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f, 0.0f, 1.0f } },

		//Left Face
		{ { -0.5f * width, -0.5f * height, +0.5f * depth }, { -1.0f, 0.0f, 0.0f } },
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { -1.0f, 0.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, -0.5f * depth }, { -1.0f, 0.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, +0.5f * depth }, { -1.0f, 0.0f, 0.0f } },

		//Right Face
		{ { +0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 0.0f } },
		{ { +0.5f * width, -0.5f * height, +0.5f * depth }, { 1.0f, 0.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, +0.5f * depth }, { 1.0f, 0.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 0.0f } },

		//Bottom Face
		{ { -0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, -1.0f, 0.0f } },
		{ { +0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, -1.0f, 0.0f } },
		{ { +0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, -1.0f, 0.0f } },
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, -1.0f, 0.0f } },

		//Top Face
		{ { -0.5f * width, +0.5f * height, -0.5f * depth }, { 0.0f, 1.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, -0.5f * depth }, { 0.0f, 1.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 0.0f } }
	};
}

std::vector<VertexPositionNormal> mini::Mesh::ShadedSheetVerts(float side, int number_of_divisions)
{
	auto verts = std::vector<VertexPositionNormal>();
	float stride = side / (number_of_divisions - 1);

	for (int i = 0; i < number_of_divisions; i++) {
		for (int j = 0; j < number_of_divisions; j++) {
			verts.push_back({ {i * stride - (side/2.0f),j * stride - (side / 2.0f),0.0f}, {1,0,0} });
		}
	}
	return verts;
}

std::vector<unsigned short> mini::Mesh::ShadedSheetIdxs(int number_of_divisions)
{
	auto idxs = std::vector<unsigned short>();

	for (int i = 0; i < number_of_divisions-1; i++) {
		for (int j = 0; j < number_of_divisions-1; j++) {
			idxs.push_back(i * number_of_divisions + j);
			idxs.push_back(i * number_of_divisions + j+1);
			idxs.push_back((i+1) * number_of_divisions + j+1);

			idxs.push_back(i * number_of_divisions + j);
			idxs.push_back((i + 1) * number_of_divisions + j + 1);
			idxs.push_back((i + 1) * number_of_divisions + j);
		}
	}
	return idxs;
}

std::vector<unsigned short> mini::Mesh::BoxIdxs()
{
	return {
		 0, 2, 1,  0, 3, 2,
		 4, 6, 5,  4, 7, 6,
		 8,10, 9,  8,11,10,
		12,14,13, 12,15,14,
		16,18,17, 16,19,18,
		20,22,21, 20,23,22
	};
}

std::vector<VertexPositionNormal> mini::Mesh::CylinderVerts(float radius, float length, int radiusSplit, int lengthSplit)
{
	auto res = std::vector<VertexPositionNormal>();
	float x = -length / 2;

	res.push_back({ {-length / 2,0.0f,0.0f}, {-1.0f,0.0f,0.0f} });
	for (int i = 0; i < radiusSplit; ++i)
	{
		float alpha = (float)i * DirectX::XM_2PI / radiusSplit;
		res.push_back({ {-length / 2,radius * sin(alpha),radius * cos(alpha)}, {-1.0f,0.0f,0.0f} });
	}

	res.push_back({ {length / 2,0.0f,0.0f}, {1.0f,0.0f,0.0f} });
	for (int i = 0; i < radiusSplit; ++i)
	{
		float alpha = (float)i * DirectX::XM_2PI / radiusSplit;
		res.push_back({ {length / 2,radius * sin(alpha),radius * cos(alpha)}, {1.0f,0.0f,0.0f} });
	}


	for (int i = 0; i <= lengthSplit; ++i)
	{
		for (int j = 0; j < radiusSplit; ++j)
		{
			float alpha = (float)j * DirectX::XM_2PI / radiusSplit;
			res.push_back({ {x,radius * sin(alpha),radius * cos(alpha)},{0.0f,sin(alpha),cos(alpha)} });
		}
		x = x + length / lengthSplit;
	}
	return res;
}

std::vector<unsigned short> mini::Mesh::CylinderIdxs(int radiusSplit, int lengthSplit)
{
	auto res = std::vector<unsigned short>();
	int zero = 0;

	for (int i = 0; i < radiusSplit; ++i)
	{
		res.push_back(zero);
		res.push_back(zero + 1 + i);
		res.push_back(zero + 1 + (i + 1) % radiusSplit);
	}
	zero += radiusSplit + 1;

	for (int i = 0; i < radiusSplit; ++i)
	{
		res.push_back(zero);
		res.push_back(zero + 1 + ((i + 1) % radiusSplit));
		res.push_back(zero + 1 + i);
	}
	zero += radiusSplit + 1;

	for (int i = 0; i < lengthSplit; ++i)
	{
		for (int j = 0; j < radiusSplit; ++j)
		{
			res.push_back(zero + i * radiusSplit + j);
			res.push_back(zero + (i + 1) * radiusSplit + j);
			res.push_back(zero + i * radiusSplit + (j + 1) % radiusSplit);


			res.push_back(zero + (i + 1) * radiusSplit + j);
			res.push_back(zero + (i + 1) * radiusSplit + (j + 1) % radiusSplit);
			res.push_back(zero + i * radiusSplit + (j + 1) % radiusSplit);

		}
	}

	return res;
}

std::vector<DirectX::XMFLOAT3> mini::Mesh::BillboardVerts(float width, float height)
{
	return {
		{-0.5f * width,-0.5f * height,0.0f},
		{-0.5f * width, 0.5f * height,0.0f},
		{0.5f * width ,-0.5f * height,0.0f},
		{0.5f * width , 0.5f * height,0.0f}
	};
}

std::vector<unsigned short> mini::Mesh::BillboardIdx()
{
	return {
	1,2,0,
	2,1,3
	};
}

std::vector<VertexPositionNormal> mini::Mesh::RectangleVerts(float width, float height)
{
	return {
		{{-0.5f * width,-0.5f * height,0.0f}, {0.0f,0.0f,-1.0f}},
		{{-0.5f * width, 0.5f * height,0.0f}, {0.0f,0.0f,-1.0f}},
		{{0.5f * width ,-0.5f * height,0.0f}, {0.0f,0.0f,-1.0f}},
		{{0.5f * width , 0.5f * height,0.0f}, {0.0f,0.0f,-1.0f}}
	};
}

std::vector<unsigned short> mini::Mesh::RectangleIdxs()
{
	return {
	1,2,0,
	2,1,3
	};
}

Mesh mini::Mesh::ShadowBox(const DxDevice& device, Mesh& source, DirectX::XMFLOAT4 lightPosition, DirectX::XMFLOAT4X4 world)
{
	if (source.wszystko.edges.size() == 0) return {};
	auto lightPos = XMLoadFloat4(&lightPosition);
	auto world_ = XMLoadFloat4x4(&world);

	XMVECTOR det;
	auto inv = XMMatrixInverse(&det, world_);
	lightPos = XMVector4Transform(lightPos, inv);

	std::vector<DirectX::XMFLOAT3> verts;
	std::vector<unsigned short> indices;

	//for (int i = 0; i < source.vertex_.size(); i++) {
	//	verts[2 * i] = source.vertex_[i];
	//	verts[2 * i + 1] = source.vertex_[i];
	//	auto vertPos = XMLoadFloat3(&(verts[2 * i]));
	//	auto from_light = XMVector3Normalize(vertPos - lightPos);

	//	XMStoreFloat3(&(verts[2 * i + 1]), vertPos + 10 * from_light);
	//}
	unsigned short index = 0;
	for (int i = 0; i < source.wszystko.edges.size(); i++)
	{
		DirectX::XMVECTOR v11 = XMLoadFloat3(&source.wszystko.triangles[source.wszystko.edges[i].tr1].p2) - XMLoadFloat3(&source.wszystko.triangles[source.wszystko.edges[i].tr1].p1);
		DirectX::XMVECTOR v12 = XMLoadFloat3(&source.wszystko.triangles[source.wszystko.edges[i].tr1].p3) - XMLoadFloat3(&source.wszystko.triangles[source.wszystko.edges[i].tr1].p2);
		DirectX::XMVECTOR v21 = XMLoadFloat3(&source.wszystko.triangles[source.wszystko.edges[i].tr2].p2) - XMLoadFloat3(&source.wszystko.triangles[source.wszystko.edges[i].tr2].p1);
		DirectX::XMVECTOR v22 = XMLoadFloat3(&source.wszystko.triangles[source.wszystko.edges[i].tr2].p3) - XMLoadFloat3(&source.wszystko.triangles[source.wszystko.edges[i].tr2].p2);

		auto from = source.wszystko.edges[i].posFrom;
		auto to = source.wszystko.edges[i].posTo;

		auto toLight = XMVector3Normalize(lightPos - XMLoadFloat3(&from));


		DirectX::XMVECTOR cross1 = XMVector3Cross(v11, v12);
		DirectX::XMVECTOR cross2 = XMVector3Cross(v21, v22);

		DirectX::XMFLOAT3 dott1;
		DirectX::XMFLOAT3 dott2;

		XMStoreFloat3(&dott1, XMVector3Dot(cross1, toLight));
		XMStoreFloat3(&dott2, XMVector3Dot(cross2, toLight));

		float dot1 = dott1.x;
		float dot2 = dott2.x;

		if (dot1 * dot2 < 0.0f || (dot1 == 0.0f && dot2 > 0.0f) || (dot2 == 0.0f && dot1 > 0.0f))
		{
			Triangle t = dot1 > dot2 ? source.wszystko.triangles[source.wszystko.edges[i].tr1] : source.wszystko.triangles[source.wszystko.edges[i].tr2];

			DirectX::XMFLOAT3 from_light_from; XMStoreFloat3(&from_light_from, XMVector3Normalize(XMLoadFloat3(&from) - lightPos));
			DirectX::XMFLOAT3 inf_from; XMStoreFloat3(&inf_from, XMLoadFloat3(&from) + 10.0f * XMLoadFloat3(&from_light_from));
			DirectX::XMFLOAT3 from_light_to; XMStoreFloat3(&from_light_to, XMVector3Normalize(XMLoadFloat3(&to) - lightPos));
			DirectX::XMFLOAT3 inf_to; XMStoreFloat3(&inf_to, XMLoadFloat3(&to) + 10.0f * XMLoadFloat3(&from_light_to));

			verts.push_back(from);
			verts.push_back(inf_from);
			verts.push_back(inf_to);
			verts.push_back(to);


			if ((t.p1.x == from.x && t.p1.y == from.y && t.p1.z == from.z &&
				t.p2.x == to.x && t.p2.y == to.y && t.p2.z == to.z) ||
				(t.p2.x == from.x && t.p2.y == from.y && t.p2.z == from.z &&
					t.p3.x == to.x && t.p3.y == to.y && t.p3.z == to.z) ||
				(t.p3.x == from.x && t.p3.y == from.y && t.p3.z == from.z &&
					t.p1.x == to.x && t.p1.y == to.y && t.p1.z == to.z))
			{
				indices.push_back(index);
				indices.push_back(index + 2);
				indices.push_back(index + 1);

				indices.push_back(index);
				indices.push_back(index + 3);
				indices.push_back(index + 2);
			}
			else
			{
				indices.push_back(index);
				indices.push_back(index + 1);
				indices.push_back(index + 2);

				indices.push_back(index);
				indices.push_back(index + 2);
				indices.push_back(index + 3);
			}
			index += 4;


		}
	}



	//for (int i = 0; i < source.vertex_.size() / 2; i++) {
	//	source.indices_;
	//	indices[6 * i + 0] = 4 * i + 0;
	//	indices[6 * i + 1] = 4 * i + 2;
	//	indices[6 * i + 2] = 4 * i + 1;

	//	indices[6 * i + 3] = 4 * i + 2;
	//	indices[6 * i + 4] = 4 * i + 3;
	//	indices[6 * i + 5] = 4 * i + 1;

	//	/*	indices[18 * i] = 2 * source.indices_[3 * i];
	//		indices[18 * i + 2] = 2 * source.indices_[3 * i +1];
	//		indices[18 * i + 1] = 2 * source.indices_[3 * i] +1;

	//		indices[18 * i + 3] = 2 * source.indices_[3 * i + 1];
	//		indices[18 * i + 5] = 2 * source.indices_[3 * i + 1] + 1;
	//		indices[18 * i + 4] = 2 * source.indices_[3 * i] + 1;

	//		indices[18 * i + 6] = 2 * source.indices_[3 * i +1];
	//		indices[18 * i + 8] = 2 * source.indices_[3 * i + 2];
	//		indices[18 * i + 7] = 2 * source.indices_[3 * i + 1] + 1;

	//		indices[18 * i + 9] = 2 * source.indices_[3 * i + 2];
	//		indices[18 * i + 11] = 2 * source.indices_[3 * i + 2] + 1;
	//		indices[18 * i + 10] = 2 * source.indices_[3 * i + 1] + 1;

	//		indices[18 * i + 12] = 2 * source.indices_[3 * i + 2];
	//		indices[18 * i + 14] = 2 * source.indices_[3 * i + 0];
	//		indices[18 * i + 13] = 2 * source.indices_[3 * i + 2] + 1;

	//		indices[18 * i + 15] = 2 * source.indices_[3 * i + 2];
	//		indices[18 * i + 17] = 2 * source.indices_[3 * i + 2] + 1;
	//		indices[18 * i + 16] = 2 * source.indices_[3 * i + 0] + 1;*/
	//}


	return SimpleTriMesh(device, verts, indices);
}

Mesh mini::Mesh::LoadMesh(const DxDevice& device, const std::wstring& meshPath)
{
	//File format for VN vertices and IN indices (IN divisible by 3, i.e. IN/3 triangles):
	//VN IN
	//pos.x pos.y pos.z norm.x norm.y norm.z tex.x tex.y [VN times, i.e. for each vertex]
	//t.i1 t.i2 t.i3 [IN/3 times, i.e. for each triangle]

	ifstream input;
	// In general we shouldn't throw exceptions on end-of-file,
	// however, in case of this file format if we reach the end
	// of a file before we read all values, the file is
	// ill-formated and we would need to throw an exception anyway
	input.exceptions(ios::badbit | ios::failbit | ios::eofbit);
	input.open(meshPath);

	int vn, in, kn;
	input >> vn;

	vector<DirectX::XMFLOAT3> verts_tmp(vn);
	for (auto i = 0; i < vn; ++i)
		input >> verts_tmp[i].x >> verts_tmp[i].y >> verts_tmp[i].z;

	input >> vn;
	vector<VertexPositionNormal> verts(vn);
	int ind;
	for (auto i = 0; i < vn; ++i)
	{
		input >> ind >> verts[i].normal.x >> verts[i].normal.y >> verts[i].normal.z;
		verts[i].position.x = verts_tmp[ind].x;
		verts[i].position.y = verts_tmp[ind].y;
		verts[i].position.z = verts_tmp[ind].z;
	}

	input >> in;
	vector<unsigned short> inds(3 * in);
	vector<Triangle> triangles(in);
	for (auto i = 0; i < in; ++i) {
		input >> inds[3 * i] >> inds[3 * i + 1] >> inds[3 * i + 2];
		triangles[i].p1 = verts[inds[3 * i]].position;
		triangles[i].p2 = verts[inds[3 * i + 1]].position;
		triangles[i].p3 = verts[inds[3 * i + 2]].position;
	}

	input >> kn;
	vector<Edge> lines(kn);
	int a, b, c, d;
	for (auto i = 0; i < kn; ++i) {
		input >> a >> b >> lines[i].tr1 >> lines[i].tr2;
		lines[i].posFrom = verts_tmp[a];
		lines[i].posTo = verts_tmp[b];
	}


	return SimpleTriMesh(device, verts, inds);
}