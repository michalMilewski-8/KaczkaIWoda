#include "particleSystem.h"

#include <iterator>

#include "dxDevice.h"
#include "exceptions.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;

const D3D11_INPUT_ELEMENT_DESC ParticleVertex::Layout[4] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

const float ParticleSystem::TIME_TO_LIVE = 3.0f;
const float ParticleSystem::EMISSION_RATE = 100.0f;
const float ParticleSystem::MAX_ANGLE = XM_PIDIV2 / 2.0f;
const float ParticleSystem::MIN_VELOCITY = 1.0f;
const float ParticleSystem::MAX_VELOCITY = 2.0f;
const float ParticleSystem::PARTICLE_SIZE = 0.08f;
const float ParticleSystem::PARTICLE_SCALE = 1.0f;
const int ParticleSystem::MAX_PARTICLES = 10000;
const XMFLOAT3 ParticleSystem::ACCELERATION = XMFLOAT3(0.0f, -1.5f, 0.0f);

ParticleSystem::ParticleSystem(DirectX::XMMATRIX emmiterMatrix, DirectX::XMFLOAT3 emitterPosition)
	: m_emitterPos(emitterPosition), m_emitterMtx(emmiterMatrix), m_particlesToCreate(0.0f), m_random(random_device{}())
{ }

vector<ParticleVertex> ParticleSystem::Update(float dt, DirectX::XMFLOAT4 cameraPosition)
{
	size_t removeCount = 0;
	for (auto& p : m_particles)
	{
		UpdateParticle(p, dt);
		if (p.Vertex.Age >= TIME_TO_LIVE)
			++removeCount;
	}
	m_particles.erase(m_particles.begin(), m_particles.begin() + removeCount);

	m_particlesToCreate += dt * EMISSION_RATE;
	while (m_particlesToCreate >= 1.0f)
	{
		--m_particlesToCreate;
		if (m_particles.size() < MAX_PARTICLES)
			m_particles.push_back(RandomParticle());
	}
	return GetParticleVerts(cameraPosition);
}

XMFLOAT3 ParticleSystem::RandomVelocity()
{
	static const uniform_real_distribution<float> angleDist(0, XM_2PI);
	float t = tan(MAX_ANGLE);
	static const uniform_real_distribution<float> magnitudeDist(0, tan(MAX_ANGLE));
	static const uniform_real_distribution<float> velDist(MIN_VELOCITY, MAX_VELOCITY);
	float angle = angleDist(m_random);
	float magnitude = magnitudeDist(m_random);
	XMFLOAT3 v{ cos(angle) * magnitude, 1.0f, sin(angle) * magnitude };
	XMStoreFloat3(&v, XMVector3TransformNormal(XMLoadFloat3(&v), XMMatrixRotationX(-DirectX::XM_PIDIV2) * m_emitterMtx));
	auto velocity = XMLoadFloat3(&v);
	auto len = velDist(m_random);
	velocity = len * XMVector3Normalize(velocity);
	XMStoreFloat3(&v, velocity);
	return v;
}

Particle ParticleSystem::RandomParticle()
{
	Particle p;
	p.Vertex = ParticleVertex();
	p.Vertex.Age = 0;
	p.Vertex.Size = PARTICLE_SIZE;
	p.Vertex.Pos = m_emitterPos;
	p.posHist.push_back(m_emitterPos);

	p.Velocities = ParticleVelocities(ACCELERATION);
	p.Velocities.Velocity = RandomVelocity();

	return p;
}

void ParticleSystem::UpdateParticle(Particle& p, float dt)
{
	int ind = p.posHist.size() - 20;
	if (ind < 0) ind = 0;
	p.Vertex.LastPos = p.posHist[ind];
	if (p.posHist.size() > 0)p.Vertex.Age += dt;

	XMVECTOR v0 = XMLoadFloat3(&p.Velocities.Acceleration);
	XMVECTOR v2 = XMLoadFloat3(&p.Velocities.Velocity);
	XMStoreFloat3(&p.Velocities.Velocity, v2 + dt * v0);
	XMVECTOR v1 = XMLoadFloat3(&p.Vertex.Pos);
	XMStoreFloat3(&p.Vertex.Pos, v1 + dt * v2);

	p.Vertex.Size += PARTICLE_SCALE * PARTICLE_SIZE * dt;
	p.posHist.push_back(p.Vertex.Pos);
}

struct myclass {
	DirectX::XMFLOAT4 camPos;
	bool operator() (ParticleVertex a, ParticleVertex b)
	{
		return dist(camPos, a.Pos) < dist(camPos, a.Pos);
	}
	float dist(XMFLOAT4 v1, XMFLOAT3 v2) {
		return sqrt((v1.x - v2.x) * (v1.x - v2.x) +
			(v1.y - v2.y) * (v1.y - v2.y) +
			(v1.z - v2.z) * (v1.z - v2.z)
		);
	}
} myobject;

vector<ParticleVertex> ParticleSystem::GetParticleVerts(DirectX::XMFLOAT4 cameraPosition)
{
	vector<ParticleVertex> vertices;
	for (int i = 0; i < m_particles.size(); ++i) vertices.push_back(m_particles[i].Vertex);

	myobject.camPos = cameraPosition;
	std::sort(vertices.begin(), vertices.end(), myobject);

	return vertices;
}