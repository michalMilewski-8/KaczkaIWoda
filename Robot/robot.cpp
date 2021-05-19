#include "robot.h"
#include "particleSystem.h"
#include <cmath>


using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;

#pragma region Constants
const unsigned int Robot::BS_MASK = 0xffffffff;

const XMFLOAT4 Robot::GREEN_LIGHT_POS = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
const XMFLOAT4 Robot::RED_LIGHT_POS = XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f);

const float Robot::ARM_SPEED = 2.0f;
const float Robot::CIRCLE_RADIUS = 0.5f;

const float Robot::SHEET_ANGLE = DirectX::XM_PIDIV4;
const float Robot::SHEET_SIZE = 2.0f;
const XMFLOAT3 Robot::SHEET_POS = XMFLOAT3(0.0f, -0.5f, 0.0f);
const XMFLOAT4 Robot::SHEET_COLOR = XMFLOAT4(1.0f, 1.0f, 1.0f, 255.0f / 255.0f);

const float Robot::WALL_SIZE = 2.0f;
const XMFLOAT3 Robot::WALLS_POS = XMFLOAT3(0.0f, 0.0f, 0.0f);
const XMFLOAT4 LightPos = XMFLOAT4(0.0f, 0.5f, 1.0f, 1.0f);

#pragma endregion


#pragma region Initalization
Robot::Robot(HINSTANCE hInstance)
	: Base(hInstance, 1280, 720, L"Kaczor"),
	m_cbWorld(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbView(m_device.CreateConstantBuffer<XMFLOAT4X4, 2>()),
	m_cbLighting(m_device.CreateConstantBuffer<Lighting>()),
	m_cbSurfaceColor(m_device.CreateConstantBuffer<XMFLOAT4>())
{
	//Projection matrix
	auto s = m_window.getClientSize();
	auto ar = static_cast<float>(s.cx) / s.cy;

	XMStoreFloat4x4(&m_projMtx, XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f));
	m_cbProj = m_device.CreateConstantBuffer<XMFLOAT4X4>();
	UpdateBuffer(m_cbProj, m_projMtx);

	XMFLOAT4X4 cameraMtx;
	XMStoreFloat4x4(&cameraMtx, m_camera.getViewMatrix());
	UpdateCameraCB(cameraMtx);

	//Sampler States
	SamplerDescription sd;
	sd.Filter = D3D11_FILTER_ANISOTROPIC;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.MaxAnisotropy = 16;
	m_samplerWrap = m_device.CreateSamplerState(sd);

	//Regular shaders
	auto vsCode = m_device.LoadByteCode(L"vs.cso");
	auto psCode = m_device.LoadByteCode(L"ps.cso");
	m_vs = m_device.CreateVertexShader(vsCode);
	m_ps = m_device.CreatePixelShader(psCode);
	m_il = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);


	// texture shaders
	vsCode = m_device.LoadByteCode(L"textureVS.cso");
	psCode = m_device.LoadByteCode(L"texturePS.cso");
	m_textureVS = m_device.CreateVertexShader(vsCode);
	m_texturePS = m_device.CreatePixelShader(psCode);
	m_textureIL = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);

	// texture shaders
	vsCode = m_device.LoadByteCode(L"kaczorVS.cso");
	psCode = m_device.LoadByteCode(L"kaczorPS.cso");
	m_kaczorVS = m_device.CreateVertexShader(vsCode);
	m_kaczorPS = m_device.CreatePixelShader(psCode);
	m_kaczorIL = m_device.CreateInputLayout(VertexPositionNormalTex::Layout, vsCode);

	//Render states
	CreateRenderStates();

	m_wall = Mesh::Rectangle(m_device);
	m_sheet = Mesh::Rectangle(m_device);
	m_duck = Mesh::LoadMesh(m_device, L"resources/duck/duck.txt");

	SetShaders();
	ID3D11Buffer* vsb[] = { m_cbWorld.get(),  m_cbView.get(), m_cbProj.get() };
	m_device.context()->VSSetConstantBuffers(0, 3, vsb);
	ID3D11Buffer* psb[] = { m_cbSurfaceColor.get(), m_cbLighting.get() };
	m_device.context()->PSSetConstantBuffers(0, 2, psb);

	CreateSheetMtx();
	CreateWallsMtx();

	d = vector<vector<float>>(Nsize);
	heightMap = vector<vector<float>>(Nsize);
	heightMapOld = vector<vector<float>>(Nsize);
	normalMap = vector<BYTE>(arraySize);

	for (int i = 0; i < Nsize; i++) {
		d[i] = vector<float>(Nsize);
		heightMap[i] = vector<float>(Nsize);
		heightMapOld[i] = vector<float>(Nsize);
		for (int j = 0; j < Nsize; j++) {
			heightMap[i][j] = 0.0f;
			heightMapOld[i][j] = 0.0f;

			float scaledi = (((i / (float)(Nsize - 1)) * 2.0f) - 1.0f);
			float scaledj = (((j / (float)(Nsize - 1)) * 2.0f) - 1.0f);
			XMFLOAT2 curr = { scaledi ,scaledj };
			float l = min(abs(1.0f - curr.x), min(abs(curr.x + 1.0f), min(abs(1.0f - curr.y), abs(curr.y + 1.0f))));
			l *= 5.0f;
			d[i][j] = 0.95f * min(1.0f, l);
		}
	}

	for (int i = 0; i < NumberOfRandomCheckPoints; i++) {
		deBoorPoints.push_back(XMFLOAT3(-0.9f + (0.0019f * (rand() % 1000)), SHEET_POS.y, -0.9f + (0.0019f * (rand() % 1000))));
		T.push_back(i-1);
	}
	kaczordt = 3.0f;
	deBoorPoints.push_back(deBoorPoints[0]);
	T.push_back(NumberOfRandomCheckPoints -1);
	deBoorPoints.push_back(deBoorPoints[1]);
	T.push_back(NumberOfRandomCheckPoints);
	deBoorPoints.push_back(deBoorPoints[2]);
	T.push_back(NumberOfRandomCheckPoints +1);
	KaczorowyDeBoor();

	auto texDesc = Texture2DDescription(Nsize, Nsize);
	texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	waterTex = m_device.CreateTexture(texDesc);
	m_waterTexture = m_device.CreateShaderResourceView(waterTex);
	m_cubeTexture = m_device.CreateShaderResourceView(L"resources/textures/output_skybox.dds");
	m_kaczorTexture = m_device.CreateShaderResourceView(L"resources/duck/ducktex.jpg");
}

void Robot::CreateRenderStates()
//Setup render states used in various stages of the scene rendering
{
}

#pragma endregion

#pragma region Per-Frame Update
void Robot::Update(const Clock& c)
{
	Base::Update(c);
	double dt = c.getFrameTime();
	if (HandleCameraInput(dt))
	{
		XMFLOAT4X4 cameraMtx;
		XMStoreFloat4x4(&cameraMtx, m_camera.getViewMatrix());
		UpdateCameraCB(cameraMtx);
	}
}

void Robot::UpdateCameraCB(DirectX::XMFLOAT4X4 cameraMtx)
{
	XMMATRIX mtx = XMLoadFloat4x4(&cameraMtx);
	XMVECTOR det;
	auto invvmtx = XMMatrixInverse(&det, mtx);
	XMFLOAT4X4 view[2] = { cameraMtx };
	XMStoreFloat4x4(view + 1, invvmtx);
	UpdateBuffer(m_cbView, view);
}

#pragma endregion
#pragma region Frame Rendering Setup
void Robot::SetShaders()
{
	m_device.context()->IASetInputLayout(m_il.get());
	m_device.context()->VSSetShader(m_vs.get(), 0, 0);
	m_device.context()->PSSetShader(m_ps.get(), 0, 0);
	m_device.context()->GSSetShader(nullptr, nullptr, 0);
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Robot::SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps)
{
	m_device.context()->VSSetShader(vs.get(), nullptr, 0);
	m_device.context()->PSSetShader(ps.get(), nullptr, 0);
}

void Robot::SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps, const dx_ptr<ID3D11InputLayout>& il)
{
	m_device.context()->IASetInputLayout(il.get());
	m_device.context()->VSSetShader(vs.get(), nullptr, 0);
	m_device.context()->PSSetShader(ps.get(), nullptr, 0);
}

void mini::gk2::Robot::KaczorowyDeBoor()
{
	float N[5] = { 0,1,0,0,0 };

	float A[5], B[5];
	int i = (int)kaczordt + 1;
	for (int j = 1; j <= 3; j++) {
		A[j] = T[i+j] - kaczordt;
		B[j] = kaczordt - T[i+1-j];
		float saved = 0;
		for (int k = 1; k <= j; k++) {
			float term = N[k] / (A[k] + B[j +1 - k]);
			N[k] = saved + A[k] * term;
			saved = B[j +1- k] * term;
		}
		N[j + 1] = saved;
	}
	XMFLOAT3 oldPos = kaczorPosition;
	kaczorPosition.x = N[1] * deBoorPoints[i-3].x + N[2] * deBoorPoints[i-2].x + N[3] * deBoorPoints[i-1].x + N[4] * deBoorPoints[i].x;
	kaczorPosition.y = N[1] * deBoorPoints[i-3].y + N[2] * deBoorPoints[i-2].y + N[3] * deBoorPoints[i-1].y + N[4] * deBoorPoints[i].y;
	kaczorPosition.z = N[1] * deBoorPoints[i-3].z + N[2] * deBoorPoints[i-2].z + N[3] * deBoorPoints[i-1].z + N[4] * deBoorPoints[i].z;
	kaczordt += kaczorSpeed;
	XMStoreFloat3(&kaczorDirection, XMVector3Normalize(XMLoadFloat3(&kaczorPosition)-XMLoadFloat3(&oldPos)));
}

void mini::gk2::Robot::CreateKaczorMtx()
{
	m_kaczorMtx = XMMatrixScaling(0.001, 0.001, 0.001)* XMMatrixRotationAxis(XMVECTOR{ 0, -1, 0 }, std::atan2f(kaczorDirection.x, kaczorDirection.z) + g_XMPi.f[0]) * XMMatrixTranslation(kaczorPosition.z, kaczorPosition.y, kaczorPosition.x);
}

void Robot::SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler)
{
	m_device.context()->PSSetShaderResources(0, resList.size(), resList.begin());
	auto s_ptr = sampler.get();
	m_device.context()->PSSetSamplers(0, 1, &s_ptr);
}

void Robot::Set1Light(XMFLOAT4 poition)
//Setup one positional light at the camera
{
	Lighting l{
		/*.ambientColor = */ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		/*.surface = */ XMFLOAT4(0.2f, 0.8f, 0.8f, 200.0f),
		/*.lights =*/ {
			{ /*.position =*/ poition, /*.color =*/ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
			//other 2 lights set to 0
		}
	};
	ZeroMemory(&l.lights[1], sizeof(Light) * 2);
	UpdateBuffer(m_cbLighting, l);
}

void Robot::Set3Lights()
//Setup one white positional light at the camera
{
	Lighting l{
		/*.ambientColor = */ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		/*.surface = */ XMFLOAT4(0.2f, 0.8f, 0.8f, 200.0f),
		/*.lights =*/ {
			{ /*.position =*/ XMFLOAT4(0.0f,0.5f,0.0f,1.0f), /*.color =*/ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
			//other 2 lights set to 0
		}
	};

	//comment the following line when structure is filled
	ZeroMemory(&l.lights[1], sizeof(Light) * 2);

	UpdateBuffer(m_cbLighting, l);
}
#pragma endregion

#pragma region Drawing
void Robot::SetWorldMtx(DirectX::XMFLOAT4X4 mtx)
{
	UpdateBuffer(m_cbWorld, mtx);
}

void Robot::DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx)
{
	SetWorldMtx(worldMtx);
	m.Render(m_device.context());
}

void Robot::CreateWallsMtx()
{
	XMVECTOR xRot = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR yRot = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR zRot = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);


	float a = WALL_SIZE;
	float scale = WALL_SIZE;

	m_wallsMtx[0] = XMMatrixScaling(scale, scale, scale) * XMMatrixTranslation(0.0f, 0.0f, a / 2);

	for (int i = 1; i < 4; ++i)
	{
		m_wallsMtx[i] = m_wallsMtx[i - 1] * XMMatrixRotationAxis(yRot, DirectX::XM_PIDIV2);
	}
	m_wallsMtx[4] = m_wallsMtx[3] * XMMatrixRotationAxis(zRot, DirectX::XM_PIDIV2);
	m_wallsMtx[5] = m_wallsMtx[4] * XMMatrixRotationAxis(zRot, DirectX::XM_PI);


	for (int i = 0; i < 6; ++i)
	{
		m_wallsMtx[i] = m_wallsMtx[i] * XMMatrixTranslation(WALLS_POS.x, WALLS_POS.y, WALLS_POS.z);
	}
}

void Robot::DrawWalls()
{
	for (int i = 0; i < 6; ++i)
	{
		UpdateBuffer(m_cbWorld, m_wallsMtx[i]);
		UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 0.1f, 0.1f, 0.5f));
		m_wall.Render(m_device.context());
	}
}

void Robot::DrawSheet(bool colors)
{

	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	UpdateBuffer(m_cbWorld, m_sheetMtx);
	m_sheet.Render(m_device.context());

	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, -1.0f, 1.0f, 1.0f));

	UpdateBuffer(m_cbWorld, m_revSheetMtx);
	m_sheet.Render(m_device.context());
}

void Robot::CreateSheetMtx()
{
	m_sheetMtx = XMMatrixScaling(SHEET_SIZE, SHEET_SIZE, 1.0f) * XMMatrixRotationX(DirectX::XM_PIDIV2) * XMMatrixTranslationFromVector(XMLoadFloat3(&SHEET_POS));
	m_revSheetMtx = XMMatrixRotationY(-DirectX::XM_PI) * m_sheetMtx;
}

void Robot::GenerateHeightMap()
{
	auto dnorm = normalMap.data();

	for (int i = 0; i < Nsize; i++) {
		for (int j = 0; j < Nsize; j++) {
			if (rand() % 100000 < 1 && rand() % 20 < 3)
				heightMapOld[i][j] = 0.25f;
		}
	}
	int u = (kaczorPosition.x + 1.0f) * 0.5f * (Nsize - 1);
	int v = (kaczorPosition.z + 1.0f) * 0.5f * (Nsize - 1);
	heightMapOld[u][v] = 0.25f;
	constexpr float A = c * c * dt * dt / (h * h);
	constexpr float B = 2 - 4 * A;
	for (int i = 0; i < Nsize; i++) {
		for (int j = 0; j < Nsize; j++) {
			float zip = 0.0f;
			if (i >0)
				zip += heightMapOld[i - 1][j];
			if (j > 0)
				zip += heightMapOld[i][j - 1];
			if (i < Nsize - 1)
				zip += heightMapOld[i + 1][j];
			if (j < Nsize - 1)
				zip += heightMapOld[i][j + 1];

			heightMap[i][j] = d[i][j] * (A * zip + B * heightMapOld[i][j] - heightMap[i][j]);
		}
	}

	std::swap(heightMapOld, heightMap);
	for (int i = 0; i < Nsize; i++) {
		for (int j = 0; j < Nsize; j++) {
			float zip = 0, zim = 0, zjp = 0, zjm = 0;
			if (i > 0)
				zim = heightMap[i - 1][j];
			if (j > 0)
				zjm = heightMap[i][j - 1];
			if (i < Nsize - 1)
				zip = heightMap[i + 1][j];
			if (j < Nsize - 1)
				zjp = heightMap[i][j + 1];

			float curr = heightMap[i][j];

			XMVECTOR vecp = { 10,(zip - curr) / h, 0.0f };
			XMVECTOR vecl = { -10,(zim - curr) / h , 0.0f };
			XMVECTOR vecg = { 0.0f,(zjm - curr) / h , -10 };
			XMVECTOR vecd = { 0.0f,(zjp - curr) / h, 10 };

			auto res = XMVector3Cross(vecg, vecl);
			auto res2 = XMVector3Cross(vecd, vecp);
			auto normal = XMVector3Normalize(res + res2);

			XMFLOAT3 normalny;
			XMStoreFloat3(&normalny, normal);

			*(dnorm++) = static_cast<BYTE>((normalny.x + 1.0f) / 2.0f * 255.0f);
			*(dnorm++) = static_cast<BYTE>((normalny.y + 1.0f) / 2.0f * 255.0f);
			*(dnorm++) = static_cast<BYTE>((normalny.z + 1.0f) / 2.0f * 255.0f);
			*(dnorm++) = 255;
		}
	}


	m_device.context()->UpdateSubresource(waterTex.get(), 0, nullptr, normalMap.data(), Nsize * pixelSize, arraySize);
	m_device.context()->GenerateMips(m_waterTexture.get());
}

void Robot::DrawKaczor()
{
	UpdateBuffer(m_cbWorld, m_kaczorMtx);

	m_duck.Render(m_device.context());
}

void Robot::Render()
{
	Base::Render();
	KaczorowyDeBoor();
	GenerateHeightMap();
	SetShaders(m_textureVS, m_texturePS);
	SetTextures({ m_waterTexture.get(), m_cubeTexture.get() }, m_samplerWrap);
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
	DrawSheet(true);

	SetShaders(m_kaczorVS, m_kaczorPS, m_kaczorIL);
	SetTextures({ m_kaczorTexture.get() }, m_samplerWrap);
	CreateKaczorMtx();
	DrawKaczor();

	SetShaders();
	SetTextures({ m_cubeTexture.get() }, m_samplerWrap);
	Set1Light(LightPos);
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
	DrawWalls();
}
#pragma endregion