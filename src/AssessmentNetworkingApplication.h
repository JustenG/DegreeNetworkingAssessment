#pragma once

#include "BaseApplication.h"
#include "AIEntity.h"
#include <vector>
#include <queue>

class Camera;

namespace RakNet {
	class RakPeerInterface;
}

class AssessmentNetworkingApplication : public BaseApplication {
public:

	AssessmentNetworkingApplication();
	virtual ~AssessmentNetworkingApplication();

	virtual bool startup();
	virtual void shutdown();

	virtual bool update(float deltaTime);

	virtual void draw();

	void EntitySanityCheck(float deltaTime);
	AIVector LowPass(AIVector prevFiltered, AIVector currRaw, float smoothingFactor);
	float LowPass(float prevFiltered, float currRaw, float smoothingFactor);

private:

	RakNet::RakPeerInterface*	m_peerInterface;

	Camera*						m_camera;

	std::vector<AIEntity>		m_aiEntities;

	std::vector<AIEntity>		m_aiLastFiltedFrame;
	std::vector<AIEntity>		m_aiTrueData;
	std::vector<AIEntity>		m_aiPastEntitys;
	std::vector<AIEntity>		m_aiSkippedEntitys;

	bool m_lastFrameSkipped;

	int m_largestTick;
	float m_packetTime;
	int m_skippedFrames;

	float prevTime;
	float deltaTime;



};