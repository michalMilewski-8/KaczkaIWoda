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
const XMFLOAT3 Robot::SHEET_POS = XMFLOAT3(0.0f, -1.0f, 0.0f);
const float Robot::SHEET_SIZE = 6.0f;
const XMFLOAT4 Robot::SHEET_COLOR = XMFLOAT4(1.0f, 1.0f, 1.0f, 255.0f / 255.0f);

const float Robot::WALL_SIZE = 6.0f;
const XMFLOAT3 Robot::WALLS_POS = XMFLOAT3(0.0f, -0.0f, 0.0f);
const XMFLOAT4 LightPos = XMFLOAT4(-0.0f, 2.0f, -0.0f, 1.0f);

const float Robot::CYLINDER_RADIUS = 1.0f;
const float Robot::CYLINDER_LENGTH = 4.0f;
const int Robot::CYLINDER_RADIUS_SPLIT = 20;
const int Robot::CYLINDER_LENGTH_SPLIT = 1;
const XMFLOAT3 Robot::CYLINDER_POS = XMFLOAT3(0.0f, -1.0f, 2.0f);
#pragma endregion
float closest_maximum(XMFLOAT2 a, XMFLOAT2 b) {
	return max(abs(a.x - b.x), abs(a.y - b.y));
}

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
	sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.BorderColor[0] = 0.0f;
	sd.BorderColor[1] = 0.0f;
	sd.BorderColor[2] = 0.0f;
	sd.BorderColor[3] = 0.0f;
	sd.MipLODBias = 0.5f;
	m_samplerWrap_back = m_device.CreateSamplerState(sd);

	m_vbParticles = m_device.CreateVertexBuffer<ParticleVertex>(ParticleSystem::MAX_PARTICLES);

	//Camera Plane
	SetCameraPlane();

	//Regular shaders
	auto vsCode = m_device.LoadByteCode(L"vs.cso");
	auto psCode = m_device.LoadByteCode(L"ps.cso");
	m_vs = m_device.CreateVertexShader(vsCode);
	m_ps = m_device.CreatePixelShader(psCode);

	m_il = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);

	vsCode = m_device.LoadByteCode(L"particleVS.cso");
	psCode = m_device.LoadByteCode(L"particlePS.cso");
	auto gsCode = m_device.LoadByteCode(L"particleGS.cso");
	m_particleVS = m_device.CreateVertexShader(vsCode);
	m_particlePS = m_device.CreatePixelShader(psCode);
	m_particleGS = m_device.CreateGeometryShader(gsCode);
	m_particleLayout = m_device.CreateInputLayout<ParticleVertex>(vsCode);


	// texture shaders
	vsCode = m_device.LoadByteCode(L"textureVS.cso");
	psCode = m_device.LoadByteCode(L"texturePS.cso");
	m_textureVS = m_device.CreateVertexShader(vsCode);
	m_texturePS = m_device.CreatePixelShader(psCode);

	m_textureIL = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);

	//Render states
	CreateRenderStates();

	//Meshes

	m_box = Mesh::ShadedBox(m_device);
	m_wall = Mesh::Rectangle(m_device);
	m_sheet = Mesh::ShadedSheet(m_device, 1.0f, Nsize);

	SetShaders();
	ID3D11Buffer* vsb[] = { m_cbWorld.get(),  m_cbView.get(), m_cbProj.get(), m_cbPlane.get() };
	m_device.context()->VSSetConstantBuffers(0, 4, vsb);
	m_device.context()->GSSetConstantBuffers(0, 1, vsb + 2);
	ID3D11Buffer* psb[] = { m_cbSurfaceColor.get(), m_cbLighting.get() };
	m_device.context()->PSSetConstantBuffers(0, 2, psb);

	CreateSheetMtx();
	CreateWallsMtx();
	CreateCylinderMtx();

	SamplerDescription sd2;

	// TODO : 1.05 Create sampler with appropriate border color and addressing (border) and filtering (bilinear) modes
	sd2.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sd2.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sd2.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sd2.BorderColor[0] = 0;
	sd2.BorderColor[1] = 0;
	sd2.BorderColor[2] = 0;
	sd2.BorderColor[3] = 0;
	sd2.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	m_sampler = m_device.CreateSamplerState(sd2);

	//Textures
	// TODO : 1.10 Create shadow texture with appropriate width, height, format, mip levels and bind flags
	Texture2DDescription td;

	td.Width = 1280;
	td.Height = 720;
	td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	td.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	td.MipLevels = 1;

	auto shadowTexture = m_device.CreateTexture(td);

	DepthStencilViewDescription dvd;

	// TODO : 1.11 Create depth-stencil-view for the shadow texture with appropriate format
	dvd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	m_shadowDepthBuffer = m_device.CreateDepthStencilView(shadowTexture, dvd);

	//ShaderResourceViewDescription srvd;

	//// TODO : 1.12 Create shader resource view for the shadow texture with appropriate format, view dimensions, mip levels and most detailed mip level
	//srvd.Format = DXGI_FORMAT_R32_FLOAT;
	//srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//srvd.Texture2D.MipLevels = 1;
	//srvd.Texture2D.MostDetailedMip = 0;

	//m_shadowMap = m_device.CreateShaderResourceView(shadowTexture, srvd);

	d = vector<vector<float>> (Nsize);
	heightMap = vector<vector<float>>(Nsize);
	heightMapNew = vector<vector<float>>(Nsize);
	heightMapOld = vector<vector<float>>(Nsize);
	normalMap = vector<BYTE>(arraySize);

	for (int i = 0; i < Nsize; i++) {
		d[i] = vector<float> (Nsize);
		heightMap[i] = vector<float>(Nsize);
		heightMapOld[i] = vector<float>(Nsize);
		heightMapNew[i] = vector<float>(Nsize);
		for (int j = 0; j < Nsize; j++) {
			heightMap[i][j] = 0.0f;
			heightMapNew[i][j] = 0.0f;
			heightMapOld[i][j] = 0.0f;

			float scaledi = ((i / (float)(Nsize - 1) * SHEET_SIZE) - SHEET_SIZE/2.0f);
			float scaledj = ((j / (float)(Nsize - 1) * SHEET_SIZE) - SHEET_SIZE / 2.0f);
			XMFLOAT2 curr = { scaledi ,scaledj };
			float l = min(closest_maximum(curr, XMFLOAT2(-SHEET_SIZE / 2.0f, scaledj)),
				min(closest_maximum(curr, XMFLOAT2(SHEET_SIZE / 2.0f, scaledj)),
					min(closest_maximum(curr, XMFLOAT2(scaledi, -SHEET_SIZE / 2.0f)),
						closest_maximum(curr, XMFLOAT2(scaledi, SHEET_SIZE / 2.0f)))));
			l *= 5.0f;
			d[i][j] = 0.95f * (min(1.0f, l));
		}
	}

	auto texDesc = Texture2DDescription(Nsize, Nsize);
	texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	//texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	//texDesc.Usage = D3D11_USAGE_DYNAMIC;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	waterTex = m_device.CreateTexture(texDesc);
	m_waterTexture = m_device.CreateShaderResourceView(waterTex);
}

void Robot::CreateRenderStates()
//Setup render states used in various stages of the scene rendering
{
	DepthStencilDescription dssDesc;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; //Enable writing to depth buffer
	m_dssNoDepthWrite = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; //Enable writing to depth buffer
	m_dssDepthWrite = m_device.CreateDepthStencilState(dssDesc);
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	dssDesc.StencilEnable = true;
	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
	dssDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_DECR_SAT;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	m_dssStencilWrite = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.StencilEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dssDesc.StencilWriteMask = 0xff;
	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
	dssDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	m_dssStencilWriteSh = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.StencilEnable = true;
	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	m_dssStencilTestNoDepthWrite = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	m_dssStencilTest = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dssDesc.DepthFunc = D3D11_COMPARISON_EQUAL;
	dssDesc.StencilWriteMask = 0x0;
	m_dssStencilTestSh = m_device.CreateDepthStencilState(dssDesc);


	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dssDesc.StencilWriteMask = 0x0;
	dssDesc.DepthFunc = D3D11_COMPARISON_EQUAL;
	m_dssStencilTestGreaterSh = m_device.CreateDepthStencilState(dssDesc);

	RasterizerDescription rsDesc;
	rsDesc.FrontCounterClockwise = true;
	m_rsCCW = m_device.CreateRasterizerState(rsDesc);

	rsDesc.FrontCounterClockwise = false;
	rsDesc.CullMode = D3D11_CULL_NONE;
	m_rsCCW_backSh = m_device.CreateRasterizerState(rsDesc);
	rsDesc.CullMode = D3D11_CULL_BACK;
	m_rsCCW_frontSh = m_device.CreateRasterizerState(rsDesc);

	BlendDescription bsDesc;
	bsDesc.RenderTarget[0].BlendEnable = true;
	bsDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bsDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	bsDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	m_bsAlpha = m_device.CreateBlendState(bsDesc);

	bsDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	m_bsAlphaInv = m_device.CreateBlendState(bsDesc);

	bsDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	bsDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bsDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
	m_bsAdd = m_device.CreateBlendState(bsDesc);
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
			{ /*.position =*/ XMFLOAT4(0.0f,2.0f,0.0f,1.0f), /*.color =*/ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
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
	XMFLOAT4X4 worldMtx;
	XMStoreFloat4x4(&worldMtx, XMMatrixIdentity());
	UpdateBuffer(m_cbWorld, worldMtx);
	m_box.Render(m_device.context());
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
		UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f));
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
	auto camPos = m_camera.getCameraPosition();
	XMFLOAT4 camDir;
	XMStoreFloat4(&camDir, m_camera.getForwardDir());
	UpdatePlaneCB(camPos, camDir);
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
			heightMapNew[i][j] = d[i][j] * (A * (zip + zim + zjp + zjm) + B * heightMap[i][j] - heightMapOld[i][j]);
		}
	}

	for (int i = 0; i < Nsize; i++) {
		for (int j = 0; j < Nsize; j++) {
			heightMapOld[i][j] = heightMap[i][j];
			heightMap[i][j] = heightMapNew[i][j];
		}
	}

	for (int i = 0; i < Nsize; i++) {
		for (int j = 0; j < Nsize; j++) {
			heightMapOld[i][j] = heightMap[i][j];
			heightMap[i][j] = heightMapNew[i][j];
		}
	}
	auto dnorm = normalMap.data();
	for (int i = 0; i < Nsize; i++) {
		for (int j = 0; j < Nsize; j++) {
			if (rand() % 100000 < 1) 
				heightMap[i][j] = 1.0f;
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

			XMVECTOR vecp = { 1.0f,0.0f,zip - curr };
			XMVECTOR vecl = { -1.0f,0.0f,curr - zim };
			XMVECTOR vecg = { 0.0f,1.0f,zjp - curr };
			XMVECTOR vecd = { 0.0f,-1.0f,curr - zjm };

			auto res = XMVector3Cross(vecg, vecl);
			auto res2 = XMVector3Cross(vecd, vecp);
			auto normal = XMVector3Normalize(res + res2);

			XMFLOAT3 normalny;
			XMStoreFloat3(&normalny, normal);

			*(dnorm++) = static_cast<BYTE>(normalny.x * 255.0f);
			*(dnorm++) = static_cast<BYTE>(normalny.y * 255.0f);
			*(dnorm++) = static_cast<BYTE>(normalny.z * 255.0f);
			*(dnorm++) = 255;
		}
	}

	m_device.context()->UpdateSubresource(waterTex.get(), 0, nullptr, normalMap.data(), Nsize*pixelSize, arraySize);
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
	Set3Lights();

	GenerateHeightMap();
	SetShaders(m_textureVS, m_texturePS);
	SetTexturesVS({ m_waterTexture.get() }, m_samplerWrap);
	DrawSheet(true);
	//DrawMirroredWorld(0);
	//DrawMirroredWorld(1);
 // 
	////render dodecahedron with one light and alpha blendingw
	//m_device.context()->OMSetBlendState(m_bsAlphaInv.get(), nullptr, BS_MASK);
	//Set1Light(LightPos);
	//DrawSheet(true);
	//m_device.context()->OMSetBlendState(nullptr, nullptr, BS_MASK);


	//TurnOffVision();
	////1. Rysowanie ca�ej sceny do depth buffora
	//
	////m_device.context()->OMSetRenderTargets(0, 0,0);
	//m_device.context()->OMSetDepthStencilState(m_dssDepthWrite.get(), 0);
	//DrawSheet(true);
	//DrawWorld();

	////2. Rysowanie bry� cienia do stencil buffer
	//m_device.context()->OMSetDepthStencilState(m_dssStencilWriteSh.get(), 0);
	//m_device.context()->RSSetState(m_rsCCW_backSh.get());
	//// 	   Dla front face stencil++
	//// 	   Dla back face stencil--
	//DrawShadowVolumes();
	////DrawBox();
	////3. Render ca�ej sceny z uwzgl�dnieniem warto�ci w stencilu
	//ResetRenderTarget();
	//
	//m_device.context()->OMSetDepthStencilState(m_dssStencilTestSh.get(), 0);
	//m_device.context()->RSSetState(m_rsCCW_frontSh.get());
	//Set1Light(LightPos);
	//UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	//DrawWorld();

	//m_device.context()->OMSetDepthStencilState(m_dssStencilTestGreaterSh.get(), 0);
	//m_device.context()->RSSetState(m_rsCCW_frontSh.get());
	SetShaders();
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
	DrawWalls();
}
#pragma endregion