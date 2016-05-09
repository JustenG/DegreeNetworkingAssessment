#include "Server.h"
#include <RakNetTypes.h>
#include <Windows.h>
#include <chrono>

Server::Server(unsigned int entityCount, float arenaRadius, float packetlossPercentage, float delayPercentage, float delayRange)
	: m_arenaRadius(arenaRadius),
	m_packetlossPercentage(packetlossPercentage),
	m_delayPercentage(delayPercentage),
	m_delayRange(delayRange)
{
	// initialize the Raknet peer interface first
	m_peerInterface = RakNet::RakPeerInterface::GetInstance();

	setupAIEntities(entityCount);
	
	//Set number of sent messages to 0
	m_numMessagesSent = 0;
}

Server::~Server() {

	// delete delayed threads
	while (m_delayedMessages.empty() == false) {
		auto t = m_delayedMessages.back();
		delete t;
		m_delayedMessages.pop_back();
	}

	m_peerInterface->Shutdown(0);
	RakNet::RakPeerInterface::DestroyInstance(m_peerInterface);
}

void Server::run() {

	// startup the server, and start it listening to clients
	std::cout << "Starting up the server..." << std::endl;
	std::cout << "Press ESCAPE to close the server..." << std::endl;

	// create a socket descriptor to describe this connection
	RakNet::SocketDescriptor sd(SERVER_PORT, 0);

	// now call startup - max of 1024 connections, on the assigned port
	m_peerInterface->Startup(1024, &sd, 1);
	m_peerInterface->SetMaximumIncomingConnections(1024);

	std::cout << "Server IP: " << m_peerInterface->GetInternalID(RakNet::UNASSIGNED_SYSTEM_ADDRESS).ToString() << std::endl << std::endl;

	RakNet::Packet* packet = nullptr;
	auto previousTime = std::chrono::high_resolution_clock::now();
	double microsecondCounter = 0;

	while (true) {

		// update entities at 60fps
		auto time = std::chrono::high_resolution_clock::now();
		double deltaMicroseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(time - previousTime).count() / 1000.0f;
		microsecondCounter += deltaMicroseconds;
		// if 16666 microseconds have passed then update and broadcast entities
		while (microsecondCounter > 16666) {
			updateAIEntities(0.016666667f);
			microsecondCounter -= 16666;
		}
		previousTime = time;

		// remove any finished delayed threads
		for (auto iter = m_delayedMessages.begin(); iter != m_delayedMessages.end(); ) {
			(*iter)->delayMicroseconds -= deltaMicroseconds;
			if ((*iter)->delayMicroseconds <= 0) {
				sendBitStream(&(*iter)->stream);
				delete (*iter);
				iter = m_delayedMessages.erase(iter);
			}
			else
				++iter;
		}

		// handle received messages
		for ( packet = m_peerInterface->Receive();
			  packet;
			  m_peerInterface->DeallocatePacket(packet), packet = m_peerInterface->Receive()) {

			switch (packet->data[0]) {
			case ID_NEW_INCOMING_CONNECTION: {
				std::cout << "A connection is incoming.\n";
				break;
			}
			case ID_DISCONNECTION_NOTIFICATION:
				std::cout << "A client has disconnected.\n";
				break;
			case ID_CONNECTION_LOST:
				std::cout << "A client lost the connection.\n";
				break;
			default:
				std::cout << "Received a message with a unknown id: " << packet->data[0];
				break;
			}
		}

		if (GetAsyncKeyState(VK_ESCAPE))
			break;
	}
}

void Server::broadcastFaultyData(const char* data, unsigned int size) {

	// lose messages every so often
	if (randf() * 100 < m_packetlossPercentage)
		return;

	// delay messages every so often
	if (randf() * 100 < m_delayPercentage) {
		DelayedBroadcast* b = new DelayedBroadcast;
		b->stream.Write((RakNet::MessageID)GameMessages::ID_ENTITY_LIST);
		b->stream.Write(size);
		b->stream.Write(data, size);
		float delay = randf() * m_delayRange;
		b->delayMicroseconds = (double)(delay * 1000.0 * 1000.0);
		m_delayedMessages.push_back(b);
	}
	else {
		// just send the stream
		RakNet::BitStream stream;
		stream.Write((RakNet::MessageID)GameMessages::ID_ENTITY_LIST);
		stream.Write(size);
		stream.Write(data, size);
		sendBitStream(&stream);
	}
}

float Server::randf() {
	return rand() / (float)RAND_MAX;
}

void Server::sendBitStream(RakNet::BitStream* stream) {
	m_peerInterface->Send(stream, HIGH_PRIORITY, UNRELIABLE, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void Server::setupAIEntities(unsigned int count) {
	unsigned int nextId = 0;
	m_aiEntities.resize(count);
	m_aiServerEntities.resize(count);
	for (auto& ai : m_aiEntities) {
		// random position and facing
		float facing = randf() * 3.14159f * 2;
		float offsetDir = randf() * 3.14159f * 2;
		float offset = m_arenaRadius * randf();

		m_aiServerEntities[nextId].data = &ai;
		m_aiServerEntities[nextId].wanderAngle = randf() * 3.14159f * 2;

		ai.id = nextId++;
		ai.position.x = sinf(offsetDir) * offset;
		ai.position.y = cosf(offsetDir) * offset;

		ai.velocity.x = sinf(facing) * MAX_VELOCITY;
		ai.velocity.y = cosf(facing) * MAX_VELOCITY;
		
		ai.ticks = 0;
	}
}

void Server::updateAIEntities(float deltaTime) {

	//Update message index count
	m_numMessagesSent++;

	for (auto& ai : m_aiServerEntities) {

		// jitter offset
		ai.wanderAngle += (randf() * 2 - 1) * WANDER_JITTER;

		AIVector f = ai.data->velocity;
		f.normalise();

		// wander force
		ai.data->velocity.x += sinf(ai.wanderAngle) * WANDER_RADIUS + f.x * WANDER_OFFSET;
		ai.data->velocity.y += cosf(ai.wanderAngle) * WANDER_RADIUS + f.y * WANDER_OFFSET;

		// truncate
		if (ai.data->velocity.lengthSqr() > (MAX_VELOCITY * MAX_VELOCITY)) {
			ai.data->velocity.normalise();
			ai.data->velocity.x *= MAX_VELOCITY;
			ai.data->velocity.y *= MAX_VELOCITY;
		}

		// move
		ai.data->position.x += ai.data->velocity.x * deltaTime;
		ai.data->position.y += ai.data->velocity.y * deltaTime;

		ai.data->teleported = false;

		// teleport if needed to stay in arena
		if (ai.data->position.lengthSqr() > (m_arenaRadius * m_arenaRadius)) {
			ai.data->teleported = true;
			AIVector offset = ai.data->position;
			offset.normalise();
			ai.data->position.x -= offset.x * m_arenaRadius * 2;
			ai.data->position.y -= offset.y * m_arenaRadius * 2;
		}
		
		//Add message number index to entity for sanity check clinet side
		ai.data->ticks = m_numMessagesSent;
	}

	// broadcast entities
	broadcastFaultyData((const char*)m_aiEntities.data(), m_aiEntities.size() * sizeof(AIEntity));
}

// application main, uses command line options
void main(int argc, char* argv[]) {

	std::cout << "Use command line options: -count N -radius M -loss X -delay Y -range Z" << std::endl;
	std::cout << "N: entity count as int" << std::endl;
	std::cout << "M: arena radius as float" << std::endl;
	std::cout << "X: packetloss percentage as float" << std::endl;
	std::cout << "Y: packet delay percentage as float" << std::endl;
	std::cout << "Z: delay range in seconds as float" << std::endl << std::endl;

	unsigned int entityCount = 100;
	float radius = 50;
	float packetlossPercentage = 10;
	float delayPercentage = 10;
	float delayRange = 1;

	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "-count") == 0) {
			entityCount = (unsigned int)atoi(argv[i + 1]);
		}
		if (strcmp(argv[i], "-radius") == 0) {
			radius = (float)atof(argv[i + 1]);
		}
		if (strcmp(argv[i], "-loss") == 0) {
			packetlossPercentage = (float)atof(argv[i + 1]);
		}
		if (strcmp(argv[i], "-delay") == 0) {
			delayPercentage = (float)atof(argv[i + 1]);
		}
		if (strcmp(argv[i], "-range") == 0) {
			delayRange = (float)atof(argv[i + 1]);
		}
	}

	std::cout << "Entity Count: " << entityCount << std::endl;
	std::cout << "Arena Radius: " << radius << std::endl;
	std::cout << "Packet Loss Percentage: " << packetlossPercentage << std::endl;
	std::cout << "Packet Delay Percentage: " << delayPercentage << std::endl;
	std::cout << "Max Delay Time in Seconds: " << delayRange << std::endl << std::endl;

	Server server(entityCount, radius, packetlossPercentage, delayPercentage, delayRange);
	server.run();
}