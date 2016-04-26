#include "AssessmentNetworkingApplication.h"

int main() {
	
	BaseApplication* app = new AssessmentNetworkingApplication();
	if (app->startup())
		app->run();
	app->shutdown();

	return 0;
}