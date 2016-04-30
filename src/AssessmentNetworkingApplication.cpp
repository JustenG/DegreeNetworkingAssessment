#include "AssessmentNetworkingApplication.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>

#include "Gizmos.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::vec2;
using glm::vec3;
using glm::vec4;

AssessmentNetworkingApplication::AssessmentNetworkingApplication() 
: m_camera(nullptr),
m_peerInterface(nullptr) {

}

AssessmentNetworkingApplication::~AssessmentNetworkingApplication() {

}

bool AssessmentNetworkingApplication::startup() 
{
	m_packetTime = 0;
	m_largestTick = 0;

	// setup the basic window
	createWindow("Client Application", 1280, 720);

	Gizmos::create();

	// set up basic camera
	m_camera = new Camera(glm::pi<float>() * 0.25f, 16 / 9.f, 0.1f, 1000.f);
	m_camera->setLookAtFrom(vec3(10, 10, 10), vec3(0));

	// start client connection
	m_peerInterface = RakNet::RakPeerInterface::GetInstance();
	
	RakNet::SocketDescriptor sd;
	m_peerInterface->Startup(1, &sd, 1);

	// request access to server
	std::string ipAddress = "10.17.23.188";
	//std::cout << "Connecting to server at: ";
	//std::cin >> ipAddress;
	RakNet::ConnectionAttemptResult res = m_peerInterface->Connect(ipAddress.c_str(), SERVER_PORT, nullptr, 0);

	if (res != RakNet::CONNECTION_ATTEMPT_STARTED) {
		std::cout << "Unable to start connection, Error number: " << res << std::endl;
		return false;
	}

	return true;
}

void AssessmentNetworkingApplication::shutdown() {
	// delete our camera and cleanup gizmos
	delete m_camera;
	Gizmos::destroy();

	// destroy our window properly
	destroyWindow();
}

bool AssessmentNetworkingApplication::update(float deltaTime) 
{

	// close the application if the window closes
	if (glfwWindowShouldClose(m_window) ||
		glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		return false;

	// update camera
	m_camera->update(deltaTime);

	// handle network messages
	RakNet::Packet* packet;

	if ((packet = m_peerInterface->Receive()) == nullptr)
	{
		for (auto& ai : m_aiEntities)
		{
			ai.position.x += ai.velocity.x * 0.016666667f;
			ai.position.y += ai.velocity.y * 0.016666667f;
		}
		m_packetTime += deltaTime;
	}

	for (nullptr; packet; m_peerInterface->DeallocatePacket(packet), packet = m_peerInterface->Receive()) 
	{
		switch (packet->data[0]) {
		case ID_CONNECTION_REQUEST_ACCEPTED:
			std::cout << "Our connection request has been accepted." << std::endl;
			break;
		case ID_CONNECTION_ATTEMPT_FAILED:
			std::cout << "Our connection request failed!" << std::endl;
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			std::cout << "The server is full." << std::endl;
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			std::cout << "We have been disconnected." << std::endl;
			break;
		case ID_CONNECTION_LOST:
			std::cout << "Connection lost." << std::endl;
			break;
		case ID_ENTITY_LIST: {

			// receive list of entities
			RakNet::BitStream stream(packet->data, packet->length, false);
			stream.IgnoreBytes(sizeof(RakNet::MessageID));
			unsigned int size = 0;
			stream.Read(size);

			// first time receiving entities
			if (m_aiEntities.size() == 0) 
			{
				m_aiEntities.resize(size / sizeof(AIEntity));
				m_aiTrueData.resize(size / sizeof(AIEntity));
				m_aiLastFiltedFrame.resize(size / sizeof(AIEntity));
				stream.Read((char*)m_aiEntities.data(), size);
				m_aiLastFiltedFrame = m_aiEntities;
			}
			else
			{
				stream.Read((char*)m_aiEntities.data(), size);
				EntitySanityCheck(m_packetTime);
			}

			m_packetTime = deltaTime;

			break;
		}
		default:
			std::cout << "Received unhandled message." << std::endl;
			break;
		}
	}

	Gizmos::clear();

	// add a grid
	for (int i = 0; i < 21; ++i) {
		Gizmos::addLine(vec3(-10 + i, 0, 10), vec3(-10 + i, 0, -10),
						i == 10 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));

		Gizmos::addLine(vec3(10, 0, -10 + i), vec3(-10, 0, -10 + i),
						i == 10 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));
	}

	return true;
}

void AssessmentNetworkingApplication::EntitySanityCheck(float deltaTime)
{
	//AIEntity cAI = m_aiEntities[0];
	//AIEntity pAI = m_aiLastFrame[0];
	//AIVector expPos;

	////If teleported, flip pos
	//if (pAI.teleported)
	//{
	//	expPos.x = -pAI.position.x;
	//	expPos.y = -pAI.position.y;
	//}
	//else
	//{
	//	expPos.x = pAI.position.x;
	//	expPos.y = pAI.position.y;
	//}
	////Add expected movement
	//expPos.x += pAI.velocity.x * deltaTime;
	//expPos.y += pAI.velocity.y * deltaTime;

	////Check how different data is
	//vec2 differance;
	//differance.x = expPos.x - cAI.position.x;
	//differance.y = expPos.y - cAI.position.y;
	//float displacement = glm::length(differance);
	//printf("%f \n", displacement);

	////Check if differance is valid
	//float tolerance = 15;
	//bool goodPacket = displacement < tolerance;

	//for (size_t i = 0; i < m_aiEntities.size(); i++)
	//{
	//
	//	AIEntity ai;	
	//
	//	//If actually teleported
	//	if (goodPacket)
	//	{
	//		//Good Packet
	//		ai = m_aiEntities[i];
	//		if (m_aiEntities[i].teleported)
	//		{
	//			//ai.position = LowPass(m_aiLastFrame[i].position, m_aiEntities[i].position, deltaTime);

	//			ai.position.x = -ai.position.x;
	//			ai.position.y = -ai.position.y;

	//			ai.velocity = LowPass(m_aiLastFrame[i].velocity, m_aiEntities[i].velocity, deltaTime);
	//		}
	//		else
	//		{
	//			ai.position = LowPass(m_aiLastFrame[i].position, m_aiEntities[i].position, deltaTime);
	//			ai.velocity = LowPass(m_aiLastFrame[i].velocity, m_aiEntities[i].velocity, deltaTime);
	//		}
	//	}
	//	else
	//	{
	//		//Delayed Packet
	//		ai = m_aiLastFrame[i];
	//		ai.position.x += ai.velocity.x * 0.016666667f;
	//		ai.position.y += ai.velocity.y * 0.016666667f;
	//	}
	//
	//
	//	m_aiTrueData[i] = ai;
	//
	//}

	if (m_aiEntities[0].ticks >= m_largestTick)
	{
		for (size_t i = 0; i < m_aiEntities.size(); i++)
		{
			AIEntity ai;
			ai = m_aiEntities[i];

			ai.position = LowPass(m_aiLastFiltedFrame[i].position, m_aiEntities[i].position, deltaTime);
			ai.velocity = LowPass(m_aiLastFiltedFrame[i].velocity, m_aiEntities[i].velocity, deltaTime);

			m_aiTrueData[i] = ai;
		}

		m_aiLastFiltedFrame = m_aiTrueData;

	}
	else
	{

	}




}

AIVector AssessmentNetworkingApplication::LowPass(AIVector prevFiltered, AIVector currRaw, float smoothingFactor)
{
	AIVector resultingEntity;
	resultingEntity.x = prevFiltered.x + smoothingFactor * (currRaw.x - prevFiltered.x);
	resultingEntity.y = prevFiltered.y + smoothingFactor * (currRaw.y - prevFiltered.y);

	return resultingEntity;
}

float AssessmentNetworkingApplication::LowPass(float prevFiltered, float currRaw, float smoothingFactor)
{
	float resultingEntity;
	resultingEntity = prevFiltered + smoothingFactor * (currRaw - prevFiltered);

	return resultingEntity;
}

void AssessmentNetworkingApplication::draw() {

	// clear the screen for this frame
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// draw entities
	for (auto& ai : m_aiTrueData)
	{
		vec3 p1 = vec3(ai.position.x + ai.velocity.x * 0.25f, 0, ai.position.y + ai.velocity.y * 0.25f);
		vec3 p2 = vec3(ai.position.x, 0, ai.position.y) - glm::cross(vec3(ai.velocity.x, 0, ai.velocity.y), vec3(0, 1, 0)) * 0.1f;
		vec3 p3 = vec3(ai.position.x, 0, ai.position.y) + glm::cross(vec3(ai.velocity.x, 0, ai.velocity.y), vec3(0, 1, 0)) * 0.1f;
		Gizmos::addTri(p1, p2, p3, glm::vec4(1, 0, 0, 1));
	}

	// display the 3D gizmos
	Gizmos::draw(m_camera->getProjectionView());
}