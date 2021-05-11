#pragma once
#include <DirectXMath.h>

#include "dxApplication.h"
#include "mesh.h"
#include "particleSystem.h"

namespace mini::gk2
{
	//Position and color of a single light
	struct Light
	{
		//Light position coordinates [x, y, z, 1.0f]
		DirectX::XMFLOAT4 position;
		//Light color [r, g, b, 1.0f] - on
		//or [r, g, b, 0.0f] - off
		DirectX::XMFLOAT4 color;
	};

	//Lighting parameters (except surface color)
	struct Lighting
	{
		//ambient reflection color [r, g, b, a]
		DirectX::XMFLOAT4 ambientColor;
		//surface reflection coefficients [ka, kd, ks, m]
		DirectX::XMFLOAT4 surface;
		//Light positions and colors
		Light lights[3];
	};

	class Robot : public DxApplication
	{
	public:
		using Base = DxApplication;

		explicit Robot(HINSTANCE hInstance);

	protected:
		void Update(const Clock& dt) override;
		void Render() override;

	private:
#pragma region CONSTANTS
		static const unsigned int BS_MASK;
		void CreateRenderStates();

		//Static light positions
		static const DirectX::XMFLOAT4 GREEN_LIGHT_POS;
		static const DirectX::XMFLOAT4 RED_LIGHT_POS;

		static const float ARM_SPEED;
		static const float CIRCLE_RADIUS;

		static const float SHEET_ANGLE;
		static const DirectX::XMFLOAT3 SHEET_POS;
		static const float SHEET_SIZE;
		static constexpr unsigned int MAP_SIZE = 1024;
		static const DirectX::XMFLOAT4 SHEET_COLOR;

		static const float WALL_SIZE;
		static const DirectX::XMFLOAT3 WALLS_POS;

		static const float CYLINDER_RADIUS;
		static const float CYLINDER_LENGTH;
		static const int CYLINDER_RADIUS_SPLIT;
		static const int CYLINDER_LENGTH_SPLIT;
		static const DirectX::XMFLOAT3 CYLINDER_POS;
#pragma endregion

#pragma region D3D Resources
		//Shader's constant buffer containing Local -> World matrix
		dx_ptr<ID3D11Buffer> m_cbWorld;
		//ConstantBuffer<DirectX::XMFLOAT4X4> m_cbWorld;
		//Shader's constant buffer containing World -> Camera and Camera -> World matrix
		dx_ptr<ID3D11Buffer> m_cbView;
		//ConstantBuffer<DirectX::XMFLOAT4X4, 2> m_cbView;
		//Shader's constant buffer containing projection matrix
		dx_ptr<ID3D11Buffer> m_cbProj;
		//ConstantBuffer<DirectX::XMFLOAT4X4> m_cbProj;
		//Shader's constant buffer containing lighting parameters (except surface color)
		dx_ptr<ID3D11Buffer> m_cbLighting;
		//ConstantBuffer<Lighting> m_cbLighting;

		dx_ptr<ID3D11Buffer> m_cbMapMtx; //pixel shader constant buffer slot 2
		//Shader's constant buffer containing surface color
		dx_ptr<ID3D11Buffer> m_cbSurfaceColor;
		//ConstantBuffer<DirectX::XMFLOAT4> m_cbSurfaceColor;
		dx_ptr<ID3D11Buffer> m_cbPlane;
		//ConstantBuffer<DirectX::XMFLOAT4> m_cbSurfaceColor;
		// 


		dx_ptr<ID3D11SamplerState> m_sampler;


		dx_ptr<ID3D11VertexShader> m_vs, m_particleVS;
		dx_ptr<ID3D11GeometryShader> m_particleGS;
		dx_ptr<ID3D11PixelShader> m_ps, m_particlePS;
		dx_ptr<ID3D11InputLayout> m_il, m_particleLayout;

		dx_ptr<ID3D11ShaderResourceView> m_dropTexture;
		dx_ptr<ID3D11SamplerState> m_samplerWrap;
		dx_ptr<ID3D11SamplerState> m_samplerWrap_back;

		//Box mesh
		Mesh m_box;
		//Wall mesh
		Mesh m_wall;
		//Wall mesh
		Mesh m_sheet;
		//Cylinder mesh
		Mesh m_cylinder;
		//Arms meshes
		Mesh m_arm0;
		Mesh m_arm1;
		Mesh m_arm2;
		Mesh m_arm3;
		Mesh m_arm4;
		Mesh m_arm5;

		//Arms angles
		float a1 = DirectX::XM_PI;
		float a2 = DirectX::XM_PI;
		float a3 = 0;
		float a4 = 0;
		float a5 = 0;
		bool automaticArmsMovement = true;
		float circleAngle = 0.0f;
		//Depth stencil state used for drawing billboards without writing to the depth buffer
		dx_ptr<ID3D11DepthStencilState> m_dssNoDepthWrite;
		dx_ptr<ID3D11DepthStencilState> m_dssDepthWrite;
		//Depth stencil state used to fill the stencil buffer
		dx_ptr<ID3D11DepthStencilState> m_dssStencilWrite;
		dx_ptr<ID3D11DepthStencilState> m_dssStencilWriteSh;
		//Depth stencil state used to perform stencil test when drawing mirrored scene
		dx_ptr<ID3D11DepthStencilState> m_dssStencilTest;
		dx_ptr<ID3D11DepthStencilState> m_dssStencilTestSh;
		dx_ptr<ID3D11DepthStencilState> m_dssStencilTestGreaterSh;
		//Depth stencil state used to perform stencil test when drawing mirrored billboards without writing to the depth buffer
		dx_ptr<ID3D11DepthStencilState> m_dssStencilTestNoDepthWrite;
		//Rasterizer state used to define front faces as counter-clockwise, used when drawing mirrored scene
		dx_ptr<ID3D11RasterizerState> m_rsCCW;
		dx_ptr<ID3D11RasterizerState> m_rsCCW_backSh;
		dx_ptr<ID3D11RasterizerState> m_rsCCW_frontSh;
		//Blend state used to draw dodecahedron faced with alpha blending.
		dx_ptr<ID3D11BlendState> m_bsAlpha;
		dx_ptr<ID3D11BlendState> m_bsAlphaInv;
		//Blend state used to draw billboards.
		dx_ptr<ID3D11BlendState> m_bsAdd;

		dx_ptr<ID3D11DepthStencilView> m_shadowDepthBuffer;

		dx_ptr<ID3D11ShaderResourceView> m_shadowMap;

		dx_ptr<ID3D11Buffer> m_vbParticles;
		ParticleSystem m_particles;
#pragma endregion

#pragma region Matrices
		DirectX::XMFLOAT4X4 m_projMtx;
		DirectX::XMMATRIX m_wallsMtx[6];
		DirectX::XMMATRIX m_sheetMtx;
		DirectX::XMMATRIX m_revSheetMtx;
		DirectX::XMMATRIX m_cylinderMtx;
#pragma endregion
		void SetWorldMtx(DirectX::XMFLOAT4X4 mtx);
		void DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx);
		void UpdateCameraCB(DirectX::XMFLOAT4X4 cameraMtx);
		void UpdatePlaneCB(DirectX::XMFLOAT4 pos, DirectX::XMFLOAT4 dir);
		void Set1Light(DirectX::XMFLOAT4 LightPos);
		void SetShaders();
		void SetParticlesShaders();
		void Set3Lights();
		void DrawBox();
		void CreateWallsMtx();
		void DrawWalls();
		void CreateCylinderMtx();
		void DrawCylinder();
		void CreateSheetMtx();
		void DrawSheet(bool colors);
		void DrawArms();
		bool HandleArmsInput(double dt);
		void InverseKinematics(DirectX::XMFLOAT3* position, DirectX::XMFLOAT3* normal);
		void DrawMirroredWorld(unsigned int i);
		void DrawShadowVolumes();
		void SetCameraPlane();

		void SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler);
		void SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList) { SetTextures(std::move(resList), m_sampler); }
		void DrawWorld(int i);
		void UpdateParticles(float dt);
		void DrawParticles();
	};
}