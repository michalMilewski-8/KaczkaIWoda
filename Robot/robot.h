#pragma once
#include <DirectXMath.h>

#include "dxApplication.h"
#include "mesh.h"
#include "particleSystem.h"
#include <queue>

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
		static constexpr unsigned int Nsize = 256;
		static constexpr unsigned int pixelSize = 4;
		static constexpr unsigned int NumberOfRandomCheckPoints = 1000;
		static constexpr unsigned int arraySize = Nsize * Nsize * pixelSize;
		static constexpr float h = 2.0f/(Nsize - 1);
		static constexpr float kaczorSpeed = 0.01f;
		static constexpr unsigned int c = 1;
		static constexpr float dt = 1.0f/Nsize;
		static const DirectX::XMFLOAT4 SHEET_COLOR;

		static const float WALL_SIZE;
		static const DirectX::XMFLOAT3 WALLS_POS;
		
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

		dx_ptr<ID3D11SamplerState> m_sampler;


		dx_ptr<ID3D11VertexShader> m_vs;
		dx_ptr<ID3D11PixelShader> m_ps;
		dx_ptr<ID3D11InputLayout> m_il;

		dx_ptr<ID3D11SamplerState> m_samplerWrap;

		//Box mesh
		Mesh m_box;
		//Wall mesh
		Mesh m_wall;
		//Wall mesh
		Mesh m_sheet;
		//Kaczor mesh
		Mesh m_duck;

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

		std::vector<std::vector<float>> heightMap;
		std::vector<std::vector<float>> heightMapOld;
		std::vector<std::vector<float>> d;
		std::vector<BYTE> normalMap;
		std::vector<DirectX::XMFLOAT3> deBoorPoints;
		std::vector<int> T;
		DirectX::XMFLOAT3 depoints[4];
		float kaczordt;
		DirectX::XMFLOAT3 kaczorPosition;
		DirectX::XMFLOAT3 kaczorDirection;

		dx_ptr<ID3D11ShaderResourceView> m_waterTexture;
		dx_ptr<ID3D11ShaderResourceView> m_cubeTexture;
		dx_ptr<ID3D11ShaderResourceView> m_kaczorTexture;
		dx_ptr<ID3D11Texture2D> waterTex;
		dx_ptr<ID3D11SamplerState> m_samplerTex;

		// wodne te
		dx_ptr<ID3D11VertexShader> m_textureVS;
		dx_ptr<ID3D11PixelShader> m_texturePS;
		dx_ptr<ID3D11InputLayout> m_textureIL;

		//kaczorowe shadery
		dx_ptr<ID3D11VertexShader> m_kaczorVS;
		dx_ptr<ID3D11PixelShader> m_kaczorPS;
		dx_ptr<ID3D11InputLayout> m_kaczorIL;
#pragma endregion

#pragma region Matrices
		DirectX::XMFLOAT4X4 m_projMtx;
		DirectX::XMMATRIX m_wallsMtx[6];
		DirectX::XMMATRIX m_sheetMtx;
		DirectX::XMMATRIX m_revSheetMtx;
		DirectX::XMMATRIX m_kaczorMtx;
#pragma endregion
		void SetWorldMtx(DirectX::XMFLOAT4X4 mtx);
		void DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx);
		void UpdateCameraCB(DirectX::XMFLOAT4X4 cameraMtx);
		void Set1Light(DirectX::XMFLOAT4 LightPos);
		void SetShaders();
		void Set3Lights();
		void CreateWallsMtx();
		void DrawWalls();
		void CreateSheetMtx();
		void DrawSheet(bool colors);
		void CreateKaczorMtx();
		void DrawKaczor();

		void GenerateHeightMap();

		void KaczorowyDeBoor();

		void SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps);
		void SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps, const dx_ptr<ID3D11InputLayout>& il);
		void SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler);
		
	};
}