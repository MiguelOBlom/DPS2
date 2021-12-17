/*
	Executed to run the application.

	Author: Miguel Blom
*/


#include "application.h"

int main (int argc, char ** argv) {
	// Initialize random
	srand(time(NULL));

	// Get the arguments
	if (argc != 8) {
		printf("Usage: %s <tracker_addr> <tracker_port> <addr> <port> <transaction_trace> <output_file> <0/1 for initialization>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// Start the application
	Application* a = new Application(argv[1], argv[2], argv[3], argv[4], TransactionReader<ID_TYPE, MAX_TRANSACTIONS>::ReadFile(argv[5]), argv[6], argv[7][0]);
	a->Run();

	// Clean up
	delete a;
	return 0;
}