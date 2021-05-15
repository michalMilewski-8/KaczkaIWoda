#include "robot.h"
#include "particleSystem.h"


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
	: Base(hInstance, 1280, 720, L"Robot"),
	m_cbWorld(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbView(m_device.CreateConstantBuffer<XMFLOAT4X4, 2>()),
	m_cbLighting(m_device.CreateConstantBuffer<Lighting>()),
	m_cbSurfaceColor(m_device.CreateConstantBuffer<XMFLOAT4>()),
	m_cbPlane(m_device.CreateConstantBuffer<XMFLOAT4, 2>()),
	m_particles{ {m_sheetMtx}, {XMFLOAT3(0.0f,0.0f,0.0f)} },
	m_dropTexture(m_device.CreateShaderResourceView(L"resources/textures/drop.png"))
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

	SamplerDescription sd2;
	/*sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.BorderColor[0] = 0.0f;
	sd.BorderColor[1] = 0.0f;
	sd.BorderColor[2] = 0.0f;
	sd.BorderColor[3] = 0.0f;
	sd.MipLODBias = 0.5f;*/
	m_samplerWrap_back = m_device.CreateSamplerState(sd2);

	//Camera Plane
	SetCameraPlane();

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

	//Render states
	CreateRenderStates();

	m_wall = Mesh::Rectangle(m_device);
	m_sheet = Mesh::Rectangle(m_device);

	SetShaders();
	ID3D11Buffer* vsb[] = { m_cbWorld.get(),  m_cbView.get(), m_cbProj.get(), m_cbPlane.get() };
	m_device.context()->VSSetConstantBuffers(0, 4, vsb);
	m_device.context()->GSSetConstantBuffers(0, 1, vsb + 2);
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

			float scaledi = ((i / (float)(Nsize - 1) * SHEET_SIZE) - SHEET_SIZE / 2.0f);
			float scaledj = ((j / (float)(Nsize - 1) * SHEET_SIZE) - SHEET_SIZE / 2.0f);
			XMFLOAT2 curr = { scaledi ,scaledj };
			float l = min(abs(SHEET_SIZE / 2.0f - curr.x), min(abs(curr.x + SHEET_SIZE / 2.0f), min(abs(SHEET_SIZE / 2.0f - curr.y), abs(curr.y + SHEET_SIZE / 2.0f))));
			l *= 5.0f;
			d[i][j] = 0.7f * min(1.0f, l);
		}
	}

	auto texDesc = Texture2DDescription(Nsize, Nsize);
	texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	waterTex = m_device.CreateTexture(texDesc);
	m_waterTexture = m_device.CreateShaderResourceView(waterTex);

	m_cubeTexture = m_device.CreateShaderResourceView(L"resources/textures/cubeMap.dds");
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

void Robot::UpdateParticles(float dt)
{
}


void Robot::InverseKinematics(XMFLOAT3* position, XMFLOAT3* normal)
{
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
void Robot::UpdatePlaneCB(DirectX::XMFLOAT4 pos, DirectX::XMFLOAT4 dir)
{
	XMFLOAT4 plane[2] = { pos,dir };
	UpdateBuffer(m_cbPlane, plane);
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

void Robot::SetParticlesShaders()
{
}

void Robot::SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler)
{
	m_device.context()->PSSetShaderResources(0, resList.size(), resList.begin());
	auto s_ptr = sampler.get();
	m_device.context()->PSSetSamplers(0, 1, &s_ptr);
}

void Robot::SetTexturesVS(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler)
{
	m_device.context()->VSSetShaderResources(0, resList.size(), resList.begin());
	auto s_ptr = sampler.get();
	m_device.context()->VSSetSamplers(0, 1, &s_ptr);
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
void Robot::DrawBox()
{
}

void Robot::SetWorldMtx(DirectX::XMFLOAT4X4 mtx)
{
	UpdateBuffer(m_cbWorld, mtx);
}

void Robot::DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx)
{
	SetWorldMtx(worldMtx);
	m.Render(m_device.context());
}

bool Robot::HandleArmsInput(double dt)
{
	return false;
}

void Robot::DrawArms()
{
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

void mini::gk2::Robot::CreateCylinderMtx()
{
}

void Robot::DrawCylinder()
{
}

void Robot::DrawSheet(bool colors)
{
	if (colors)
	{
		UpdateBuffer(m_cbSurfaceColor, SHEET_COLOR);
	}
	else
	{
		UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	}
	UpdateBuffer(m_cbWorld, m_sheetMtx);
	m_sheet.Render(m_device.context());

	UpdateBuffer(m_cbWorld, m_revSheetMtx);
	m_sheet.Render(m_device.context());
}

void Robot::CreateSheetMtx()
{
	m_sheetMtx = XMMatrixScaling(SHEET_SIZE, SHEET_SIZE, 1.0f) * XMMatrixRotationX(DirectX::XM_PIDIV2) * XMMatrixTranslationFromVector(XMLoadFloat3(&SHEET_POS));
	m_revSheetMtx = XMMatrixRotationY(-DirectX::XM_PI) * m_sheetMtx;
}

void Robot::DrawParticles()
{
}

void Robot::DrawMirroredWorld(unsigned int i)
{
}

void Robot::SetCameraPlane()
{
}
void Robot::GenerateHeightMap()
{
	for (int i = 0; i < Nsize; i++) {
		for (int j = 0; j < Nsize; j++) {
			constexpr float A = c * c * dt * dt / (h * h);
			constexpr float B = 2 - 4 * A;
			float zip, zim, zjp, zjm;
			if (i == 0)
				zim = 0.0f;
			else
				zim = heightMap[i - 1][j];
			if (j == 0)
				zjm = 0.0f;
			else
				zjm = heightMap[i][j - 1];
			if (i == Nsize - 1)
				zip = 0.0f;
			else
				zip = heightMap[i + 1][j];
			if (j == Nsize - 1)
				zjp = 0.0f;
			else
				zjp = heightMap[i][j + 1];
			heightMapOld[i][j] = d[i][j] * (A * (zip + zim + zjp + zjm) + B * heightMap[i][j] - heightMapOld[i][j]);

			if (rand() % 100000 < 1 && rand() % 20 < 3)
				heightMapOld[i][j] += 0.25f;
		}
	}

	std::swap(heightMapOld, heightMap);

	auto dnorm = normalMap.data();
	for (int i = 0; i < Nsize; i++) {
		for (int j = 0; j < Nsize; j++) {

			float zip, zim, zjp, zjm;
			float curr = heightMap[i][j];
			if (i == 0)
				zim = 0.0f;
			else
				zim = heightMap[i - 1][j];
			if (j == 0)
				zjm = 0.0f;
			else
				zjm = heightMap[i][j - 1];
			if (i == Nsize - 1)
				zip = 0.0f;
			else
				zip = heightMap[i + 1][j];
			if (j == Nsize - 1)
				zjp = 0.0f;
			else
				zjp = heightMap[i][j + 1];

			XMVECTOR vecp = { 1,(zip - curr)/h, 0.0f };
			XMVECTOR vecl = { -1,(zim - curr)/-h , 0.0f };
			XMVECTOR vecg = { 0.0f,(zjm - curr)/h , -1 };
			XMVECTOR vecd = { 0.0f,(zjp - curr)/-h, 1 };

			auto res = XMVector3Cross( vecg,vecl );
			auto res2 = XMVector3Cross(vecd, vecp);
			auto normal = XMVector3Normalize(res + res2);

			XMFLOAT3 normalny;
			XMStoreFloat3(&normalny, normal);

			*(dnorm++) = static_cast<BYTE>((normalny.x + 1.0f)/2.0f * 255.0f);
			*(dnorm++) = static_cast<BYTE>((normalny.y + 1.0f) / 2.0f * 255.0f);
			*(dnorm++) = static_cast<BYTE>((normalny.z + 1.0f) / 2.0f * 255.0f);
			*(dnorm++) = 255;
		}
	}

	m_device.context()->UpdateSubresource(waterTex.get(), 0, nullptr, normalMap.data(), Nsize * pixelSize, arraySize);
	m_device.context()->GenerateMips(m_waterTexture.get());
}

void Robot::DrawWorld(int i = -1)
{
}

void mini::gk2::Robot::DrawShadowVolumes()
{
}

void Robot::Render()
{
	Base::Render();
	GenerateHeightMap();
	SetShaders(m_textureVS, m_texturePS);
	SetTextures({ m_waterTexture.get(), m_cubeTexture.get() }, m_samplerWrap);
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
	DrawSheet(true);

	SetShaders();
	SetTextures({ m_cubeTexture.get() }, m_samplerWrap_back);
	Set1Light(LightPos);
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
	DrawWalls();
}
#pragma endregion