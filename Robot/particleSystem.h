#pragma once
#include <DirectXMath.h>
#include <vector>
#include <random>
#include <d3d11.h>

namespace mini
{
	namespace gk2
	{
		struct ParticleVertex
		{
			DirectX::XMFLOAT3 Pos;
			float Age;
			float Size;
			DirectX::XMFLOAT3 LastPos;
			static const D3D11_INPUT_ELEMENT_DESC Layout[4];

			ParticleVertex() : Pos(0.0f, 0.0f, 0.0f), Age(0.0f), Size(0.0f) { }
		};

		struct ParticleVelocities
		{
			DirectX::XMFLOAT3 Velocity;
			DirectX::XMFLOAT3 Acceleration;
			float AngularVelocity;

			ParticleVelocities(DirectX::XMFLOAT3 acceleration) : Velocity(0.0f, 0.0f, 0.0f), AngularVelocity(0.0f), Acceleration(acceleration) { }
			ParticleVelocities() : ParticleVelocities(DirectX::XMFLOAT3(0.0f,0.0f,0.0f)) { }
		};

		struct Particle
		{
			ParticleVertex Vertex;
			ParticleVelocities Velocities;
			std::vector<DirectX::XMFLOAT3> posHist;
		};

		class ParticleSystem
		{
		public:
			ParticleSystem() = default;

			ParticleSystem(ParticleSystem&& other) = default;

			ParticleSystem(DirectX::XMMATRIX emitterMatrix, DirectX::XMFLOAT3 emitterPosition);

			ParticleSystem& operator=(ParticleSystem&& other) = default;

			std::vector<ParticleVertex> Update(float dt, DirectX::XMFLOAT4 cameraPosition);

			void UpdateEmitter(DirectX::XMFLOAT3 emitterPosition, DirectX::XMMATRIX emitterMatrix) { m_emitterPos = emitterPosition;  m_emitterMtx = emitterMatrix; };

			size_t particlesCount() const { return m_particles.size(); }
			static const int MAX_PARTICLES;		//maximal number of particles in the system

		private:
			static const float TIME_TO_LIVE;	//time of particle's life in seconds
			static const float EMISSION_RATE;	//number of particles to be born per second
			static const float MAX_ANGLE;		//maximal angle declination from mean direction
			static const float MIN_VELOCITY;	//minimal value of particle's velocity
			static const float MAX_VELOCITY;	//maximal value of particle's velocity
			static const float PARTICLE_SIZE;	//initial size of a particle
			static const float PARTICLE_SCALE;	//size += size*scale*dtime
			static const DirectX::XMFLOAT3 ACCELERATION;	//particle acceleration

			DirectX::XMFLOAT3 m_emitterPos;
			DirectX::XMMATRIX m_emitterMtx;
			float m_particlesToCreate;

			std::vector<Particle> m_particles;

			std::default_random_engine m_random;

			DirectX::XMFLOAT3 RandomVelocity();
			Particle RandomParticle();
			static void UpdateParticle(Particle& p, float dt);
			std::vector<ParticleVertex> GetParticleVerts(DirectX::XMFLOAT4 cameraPosition);
		};
	}
}