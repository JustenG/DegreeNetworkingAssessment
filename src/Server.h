#pragma once
#include <iostream>
#include <string>
#include <unordered_map>

#include <RakPeerInterface.h>
#include <BitStream.h>

#include "../src/AIEntity.h"

class Server {
public:

	Server(unsigned int entityCount, float arenaRadius, float packetlossPercentage, float delayPercentage, float delayRange);
	~Server();

	void	run();
			
private:
	
	// occasionally loses or delays packets
	void	broadcastFaultyData(const char* data, unsigned int size);

	// sends stream immediately
	void	sendBitStream(RakNet::BitStream* stream);

	// set up / update AI data and broadcast
	void	setupAIEntities(unsigned int count);
	void	updateAIEntities(float deltaTime);

	// helper method, returns random range [0,1]
	static float	randf();

	// wander data
	float		m_arenaRadius;
	const float MAX_VELOCITY = 10;
	const float WANDER_JITTER = 0.05f;
	const float WANDER_OFFSET = 2.5f;
	const float WANDER_RADIUS = 1.5f;

	// this data is sent to clients
	std::vector<AIEntity>		m_aiEntities;

	// this data is NOT sent to clients, handles wandering
	std::vector<AIServerEntity>	m_aiServerEntities;

	// raknet
	const unsigned short PORT = 5456;
	RakNet::RakPeerInterface*	m_peerInterface;

	// faults
	float					m_packetlossPercentage;
	float					m_delayPercentage;
	float					m_delayRange;
	
	struct DelayedBroadcast {
		double delayMicroseconds;
		RakNet::BitStream stream;
	};
	std::list<DelayedBroadcast*>	m_delayedMessages;
};