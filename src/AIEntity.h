#pragma once

#include <MessageIdentifiers.h>
#include <cmath>

enum GameMessages {
	// this ID is used for sending the AI entities
	// the structure of the bitstream is:
	// [ message ID, unsigned int bytecount, AIEntity array of size (bytecount / sizeof(AIEntity)) ]
	ID_ENTITY_LIST = ID_USER_PACKET_ENUM + 1,
};

static const unsigned short SERVER_PORT = 5456;

// very basic 2D vector struct with helper methods
struct AIVector {
	float x, y;

	void normalise() {
		float m = sqrt(x * x + y * y);
		x /= m; y /= m;
	}

	float length() const {
		return sqrt(x * x + y * y);
	}

	inline float lengthSqr() const {
		return x * x + y * y;
	}
};

// basic AI entity data that is broadcast by the server
struct AIEntity {
	unsigned int id;
	AIVector position;
	AIVector velocity;
	bool teleported;
};

// server's entity data that it uses to update entities
struct AIServerEntity {
	AIEntity* data;
	float wanderAngle;
};