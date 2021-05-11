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
const XMFLOAT3 Robot::SHEET_POS = XMFLOAT3(-1.5f, 0.0f, 0.0f);
const float Robot::SHEET_SIZE = 1.5f;
const XMFLOAT4 Robot::SHEET_COLOR = XMFLOAT4(0.1f, 0.1f, 0.1f, 140.0f / 255.0f);

const float Robot::WALL_SIZE = 8.0f;
const XMFLOAT3 Robot::WALLS_POS = XMFLOAT3(0.0f, 3.0f, 0.0f);
const XMFLOAT4 LightPos = XMFLOAT4(-1.0f, 3.0f, -3.0f, 1.0f);

const float Robot::CYLINDER_RADIUS = 1.0f;
const float Robot::CYLINDER_LENGTH = 4.0f;
const int Robot::CYLINDER_RADIUS_SPLIT = 20;
const int Robot::CYLINDER_LENGTH_SPLIT = 1;
const XMFLOAT3 Robot::CYLINDER_POS = XMFLOAT3(0.0f, -1.0f, 2.0f);
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

	//Render states
	CreateRenderStates();

	//Meshes

	m_box = Mesh::ShadedBox(m_device);
	m_wall = Mesh::Rectangle(m_device);
	m_cylinder = Mesh::Cylinder(m_device, CYLINDER_RADIUS, CYLINDER_LENGTH, CYLINDER_RADIUS_SPLIT, CYLINDER_LENGTH_SPLIT);
	m_sheet = Mesh::Rectangle(m_device, SHEET_SIZE, SHEET_SIZE);

	m_arm0 = Mesh::LoadMesh(m_device, L"resources/puma/mesh1.txt");
	m_arm1 = Mesh::LoadMesh(m_device, L"resources/puma/mesh2.txt");
	m_arm2 = Mesh::LoadMesh(m_device, L"resources/puma/mesh3.txt");
	m_arm3 = Mesh::LoadMesh(m_device, L"resources/puma/mesh4.txt");
	m_arm4 = Mesh::LoadMesh(m_device, L"resources/puma/mesh5.txt");
	m_arm5 = Mesh::LoadMesh(m_device, L"resources/puma/mesh6.txt");

	SetShaders();
	ID3D11Buffer* vsb[] = { m_cbWorld.get(),  m_cbView.get(), m_cbProj.get(), m_cbPlane.get() };
	m_device.context()->VSSetConstantBuffers(0, 4, vsb);
	m_device.context()->GSSetConstantBuffers(0, 1, vsb+2);
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
	HandleArmsInput(dt);

	if (automaticArmsMovement)
	{
		circleAngle = fmod((circleAngle + dt * ARM_SPEED), DirectX::XM_2PI);
		XMFLOAT3 pos(CIRCLE_RADIUS * sin(circleAngle), CIRCLE_RADIUS * cos(circleAngle), 0.0f);
		XMFLOAT3 norm(0.0f, 0.0f, -1.0f);

		XMStoreFloat3(&pos, XMVector3TransformCoord(XMLoadFloat3(&pos), m_sheetMtx));
		XMStoreFloat3(&norm, XMVector3TransformNormal(XMLoadFloat3(&norm), m_sheetMtx));
		m_particles.UpdateEmitter(pos, m_sheetMtx);
		InverseKinematics(&pos, &norm);
		UpdateParticles(dt);
	}
}

void Robot::UpdateParticles(float dt)
{
	auto particles = m_particles.Update(dt, m_camera.getCameraPosition());
	UpdateBuffer(m_vbParticles, particles);
}


void Robot::InverseKinematics(XMFLOAT3* position, XMFLOAT3* normal)
{
	auto norm = XMLoadFloat3(normal);
	auto pos = XMLoadFloat3(position);
	float l1 = .91f, l2 = .81f, l3 = .33f, dy = .27f, dz = .26f;
	norm = XMVector3Normalize(norm);
	XMFLOAT3 norm_v;
	XMStoreFloat3(&norm_v, norm);
	XMFLOAT3 pos1;
	XMStoreFloat3(&pos1, pos + norm * l3);
	float e = sqrtf(pos1.z * pos1.z + pos1.x * pos1.x - dz * dz);
	a1 = atan2(pos1.z, -pos1.x) + atan2(dz, e);
	XMFLOAT3 pos2(e, pos1.y - dy, .0f);
	a3 = -acosf(min(1.0f, (pos2.x * pos2.x + pos2.y * pos2.y - l1 * l1 - l2 * l2) / (2.0f * l1 * l2)));
	float k = l1 + l2 * cosf(a3), l = l2 * sinf(a3);
	a2 = -atan2(pos2.y, sqrtf(pos2.x * pos2.x + pos2.z * pos2.z)) - atan2(l, k);

	XMVECTOR normal1;
	XMFLOAT3 normal1_v;

	normal1 = XMVector3TransformNormal(norm, XMMatrixRotationY(-a1));
	XMStoreFloat3(&normal1_v, XMVector3TransformNormal(normal1, XMMatrixRotationZ(-(a2 + a3))));


	a5 = acosf(normal1_v.x);
	a4 = atan2(normal1_v.z, normal1_v.y);
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

void Robot::SetParticlesShaders()
{
	m_device.context()->IASetInputLayout(m_particleLayout.get());
	m_device.context()->VSSetShader(m_particleVS.get(), 0, 0);
	m_device.context()->PSSetShader(m_particlePS.get(), 0, 0);
	m_device.context()->GSSetShader(m_particleGS.get(), nullptr, 0);
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
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
			{ /*.position =*/ XMFLOAT4(-2.0f,3.0f,-2.0f,1.0f), /*.color =*/ XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f) }
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
	KeyboardState kstate;
	bool moved = false;
	if (!m_keyboard.GetState(kstate))
		return false;
	for (int i = 0; i < 256; ++i)
	{
		if (kstate.isKeyDown(i))
		{
			int a = 0;
		}
	}
	float unit = 0.001f;

	if (kstate.isKeyDown(DIK_RBRACKET))// ]
	{
		moved = true;
		a1 += unit;
	}
	else if (kstate.isKeyDown(DIK_LBRACKET))// [
	{
		moved = true;
		a1 -= unit;
	}
	if (kstate.isKeyDown(DIK_APOSTROPHE))// '
	{
		moved = true;
		a2 += unit;
	}
	else if (kstate.isKeyDown(DIK_SEMICOLON))// ;
	{
		moved = true;
		a2 -= unit;
	}
	if (kstate.isKeyDown(DIK_PERIOD))// ,
	{
		moved = true;
		a3 += unit;
	}
	else if (kstate.isKeyDown(DIK_COMMA))// .
	{
		moved = true;
		a3 -= unit;
	}
	if (kstate.isKeyDown(DIK_P))// P
	{
		moved = true;
		a4 += unit;
	}
	else if (kstate.isKeyDown(DIK_O))// O
	{
		moved = true;
		a4 -= unit;
	}
	if (kstate.isKeyDown(DIK_L))// L
	{
		moved = true;
		a5 += unit;
	}
	else if (kstate.isKeyDown(DIK_K))// K
	{
		moved = true;
		a5 -= unit;
	}

	if (kstate.isKeyDown(DIK_SPACE))//SPACE
	{
		automaticArmsMovement = !automaticArmsMovement;
	}

	a1 = fmod(a1, DirectX::XM_2PI);
	a2 = fmod(a2, DirectX::XM_2PI);
	a3 = fmod(a3, DirectX::XM_2PI);
	a4 = fmod(a4, DirectX::XM_2PI);
	a5 = fmod(a5, DirectX::XM_2PI);

	return moved;
}

void Robot::DrawArms()
{
	XMFLOAT4X4 armMtx{};
	XMStoreFloat4x4(&armMtx, XMMatrixIdentity());

	XMVECTOR xRot = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR yRot = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR zRot = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	DrawMesh(m_arm0, armMtx);

	XMStoreFloat4x4(&armMtx, XMMatrixRotationAxis(yRot, a1) * XMLoadFloat4x4(&armMtx));
	DrawMesh(m_arm1, armMtx);

	XMStoreFloat4x4(&armMtx, XMMatrixTranslation(0.0f, -0.27f, 0.0f) * XMMatrixRotationAxis(zRot, a2) * XMMatrixTranslation(0.0f, 0.27f, 0.0f) * XMLoadFloat4x4(&armMtx));
	DrawMesh(m_arm2, armMtx);

	XMStoreFloat4x4(&armMtx, XMMatrixTranslation(0.91f, -0.27f, 0.0f) * XMMatrixRotationAxis(zRot, a3) * XMMatrixTranslation(-0.91f, 0.27f, 0.0f) * XMLoadFloat4x4(&armMtx));
	DrawMesh(m_arm3, armMtx);

	XMStoreFloat4x4(&armMtx, XMMatrixTranslation(0.0f, -0.27f, 0.26f) * XMMatrixRotationAxis(xRot, a4) * XMMatrixTranslation(0.0f, 0.27f, -0.26f) * XMLoadFloat4x4(&armMtx));
	DrawMesh(m_arm4, armMtx);

	XMStoreFloat4x4(&armMtx, XMMatrixTranslation(1.72f, -0.27f, 0.0f) * XMMatrixRotationAxis(zRot, a5) * XMMatrixTranslation(-1.72f, 0.27f, 0.0f) * XMLoadFloat4x4(&armMtx));
	DrawMesh(m_arm5, armMtx);
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
	m_cylinderMtx = XMMatrixTranslation(CYLINDER_POS.x, CYLINDER_POS.y, CYLINDER_POS.z);
}

void Robot::DrawCylinder()
{
	UpdateBuffer(m_cbWorld, m_cylinderMtx);
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(0.1f, 1.0f, 0.1f, 0.5f));
	m_cylinder.Render(m_device.context());
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
	m_sheetMtx = XMMatrixRotationX(SHEET_ANGLE) * XMMatrixRotationY(-DirectX::XM_PIDIV2) * XMMatrixTranslationFromVector(XMLoadFloat3(&SHEET_POS));
	m_revSheetMtx = XMMatrixRotationY(-DirectX::XM_PI) * m_sheetMtx;
}

void Robot::DrawParticles()
{
	m_device.context()->OMSetBlendState(m_bsAlpha.get(), nullptr, UINT_MAX);
	m_device.context()->OMSetDepthStencilState(m_dssNoDepthWrite.get(), 0);
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(0.1f, 0.1f, 0.1f, 0.9f));

	if (m_particles.particlesCount() == 0)
		return;
	//Set input layoutv primitive topology, shaders, vertex buffer, and draw particles
	SetTextures({ m_dropTexture.get()});
	SetParticlesShaders();
	unsigned int stride = sizeof(ParticleVertex);
	unsigned int offset = 0;
	auto vb = m_vbParticles.get();
	m_device.context()->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	m_device.context()->Draw(m_particles.particlesCount(), 0);
	SetShaders();

	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_device.context()->OMSetBlendState(nullptr, nullptr, UINT_MAX);
	m_device.context()->OMSetDepthStencilState(nullptr, 0);
}

void Robot::DrawMirroredWorld(unsigned int i)
//Draw the mirrored scene reflected in the i-th dodecahedron face
{
	m_device.context()->OMSetDepthStencilState(m_dssStencilWrite.get(), i + 1);

	XMFLOAT4 planePos = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 planeDir = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);

	XMMATRIX m_mirrorMtx;
	XMMATRIX m_curSheetMtx;

	if (i == 0)
	{
		m_curSheetMtx = m_sheetMtx;
	}
	else
	{
		m_curSheetMtx = m_revSheetMtx;
	}

	UpdateBuffer(m_cbWorld, m_curSheetMtx);
	m_mirrorMtx = XMMatrixInverse(nullptr, m_curSheetMtx) * XMMatrixScaling(1.0f, 1.0f, -1.0f) * m_curSheetMtx;
	XMStoreFloat4(&planePos, XMVector4Transform(XMLoadFloat4(&planePos), m_curSheetMtx));
	XMStoreFloat4(&planeDir, XMVector4Transform(XMLoadFloat4(&planeDir), m_curSheetMtx));
	UpdatePlaneCB(planePos, planeDir);

	UpdateBuffer(m_cbSurfaceColor, SHEET_COLOR);
	m_sheet.Render(m_device.context());

	m_device.context()->OMSetDepthStencilState(m_dssStencilTest.get(), i + 1);

	m_device.context()->RSSetState(m_rsCCW.get());
	XMFLOAT4X4 multiplied;
	XMFLOAT4X4 camTmp;
	XMStoreFloat4x4(&camTmp, m_camera.getViewMatrix());
	XMStoreFloat4x4(&multiplied, m_mirrorMtx * m_camera.getViewMatrix());
	UpdateCameraCB(multiplied);

	m_device.context()->OMSetBlendState(nullptr, nullptr, BS_MASK);
	Set1Light(LightPos);
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f));
	DrawWorld(i);

	m_device.context()->RSSetState(nullptr);

	m_device.context()->OMSetDepthStencilState(m_dssStencilTestNoDepthWrite.get(), i + 1);
	//DrawBillboards();
	XMFLOAT4X4 cameraMtx;
	XMStoreFloat4x4(&cameraMtx, m_camera.getViewMatrix());
	UpdateCameraCB(cameraMtx);

	SetCameraPlane();

	m_device.context()->OMSetDepthStencilState(nullptr, 0);
}

void Robot::SetCameraPlane()
{
	auto camPos = m_camera.getCameraPosition();
	XMFLOAT4 camDir;
	XMStoreFloat4(&camDir, m_camera.getForwardDir());
	UpdatePlaneCB(camPos, camDir);
}
void Robot::DrawWorld(int i = -1)
{
	//DrawBox();
	DrawArms();
	DrawWalls();
	DrawCylinder();
	if (i != 1)
		DrawParticles();
}

void mini::gk2::Robot::DrawShadowVolumes()
{
	XMFLOAT4X4 armMtx{};
	XMFLOAT4X4 cylMtx{};
	XMStoreFloat4x4(&armMtx, XMMatrixIdentity());
	XMStoreFloat4x4(&cylMtx, m_cylinderMtx);

	XMVECTOR xRot = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR yRot = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR zRot = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	DrawMesh(Mesh::ShadowBox(m_device,m_arm0,LightPos, armMtx), armMtx);


	XMStoreFloat4x4(&armMtx, XMMatrixRotationAxis(yRot, a1) * XMLoadFloat4x4(&armMtx));
	DrawMesh(Mesh::ShadowBox(m_device, m_arm1, LightPos, armMtx), armMtx);

	XMStoreFloat4x4(&armMtx, XMMatrixTranslation(0.0f, -0.27f, 0.0f) * XMMatrixRotationAxis(zRot, a2) * XMMatrixTranslation(0.0f, 0.27f, 0.0f) * XMLoadFloat4x4(&armMtx));
	DrawMesh(Mesh::ShadowBox(m_device, m_arm2, LightPos, armMtx), armMtx);

	XMStoreFloat4x4(&armMtx, XMMatrixTranslation(0.91f, -0.27f, 0.0f) * XMMatrixRotationAxis(zRot, a3) * XMMatrixTranslation(-0.91f, 0.27f, 0.0f) * XMLoadFloat4x4(&armMtx));
	DrawMesh(Mesh::ShadowBox(m_device, m_arm3, LightPos, armMtx), armMtx);

	XMStoreFloat4x4(&armMtx, XMMatrixTranslation(0.0f, -0.27f, 0.26f) * XMMatrixRotationAxis(xRot, a4) * XMMatrixTranslation(0.0f, 0.27f, -0.26f) * XMLoadFloat4x4(&armMtx));
	DrawMesh(Mesh::ShadowBox(m_device, m_arm4, LightPos, armMtx), armMtx);

	XMStoreFloat4x4(&armMtx, XMMatrixTranslation(1.72f, -0.27f, 0.0f) * XMMatrixRotationAxis(zRot, a5) * XMMatrixTranslation(-1.72f, 0.27f, 0.0f) * XMLoadFloat4x4(&armMtx));
	DrawMesh(Mesh::ShadowBox(m_device, m_arm5, LightPos, armMtx), armMtx);


	//DrawMesh(Mesh::ShadowBox(m_device, m_cylinder, LightPos, cylMtx), cylMtx);

}

void Robot::Render()
{
	Base::Render();
	SetShaders();
	DrawMirroredWorld(0);
	DrawMirroredWorld(1);
  
	//render dodecahedron with one light and alpha blendingw
	m_device.context()->OMSetBlendState(m_bsAlphaInv.get(), nullptr, BS_MASK);
	Set1Light(LightPos);
	DrawSheet(true);
	m_device.context()->OMSetBlendState(nullptr, nullptr, BS_MASK);


	TurnOffVision();
	//1. Rysowanie ca�ej sceny do depth buffora
	
	//m_device.context()->OMSetRenderTargets(0, 0,0);
	m_device.context()->OMSetDepthStencilState(m_dssDepthWrite.get(), 0);
	DrawSheet(true);
	DrawWorld();

	//2. Rysowanie bry� cienia do stencil buffer
	m_device.context()->OMSetDepthStencilState(m_dssStencilWriteSh.get(), 0);
	m_device.context()->RSSetState(m_rsCCW_backSh.get());
	// 	   Dla front face stencil++
	// 	   Dla back face stencil--
	DrawShadowVolumes();
	//DrawBox();
	//3. Render ca�ej sceny z uwzgl�dnieniem warto�ci w stencilu
	ResetRenderTarget();
	
	m_device.context()->OMSetDepthStencilState(m_dssStencilTestSh.get(), 0);
	m_device.context()->RSSetState(m_rsCCW_frontSh.get());
	Set1Light(LightPos);
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	DrawWorld();

	m_device.context()->OMSetDepthStencilState(m_dssStencilTestGreaterSh.get(), 0);
	m_device.context()->RSSetState(m_rsCCW_frontSh.get());
	Set3Lights();
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
	DrawWorld();
}
#pragma endregion