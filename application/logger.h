/*
Log for experiments when all transactions have been added
START_TIME\t<TIME>

SENDING_BLOCK\t<TIME>\t<BLOCK_NUMBER>

REQUEST_BLOCK\t<TIME>\t<BLOCK_NUMBER>
RECEIVED_BLOCK\t<TIME>\t<BLOCK_NUMBER>\t[FAIL/SUCCESS]

BLOCK_ADDED_START\t<TIME>\t<BLOCK_NUMBER>
BLOCK_ADDED_STOP\t<TIME>\t<BLOCK_NUMBER>\t[FAIL/SUCCESS]
*/

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <iostream>

class Logger {
private:
	bool first_write;
	std::vector<std::string> log = std::vector<std::string>();
	std::string get_time();


public:
	Logger();
	~Logger();

	void LogStartTime();
	void LogSendingBlock(size_t block_id);
	void LogRequestBlock(size_t block_id);
	void LogReceivedBlock(size_t block_id, bool success);
	void LogBlockAddedStart(size_t block_id);
	void LogBlockAddedStop(size_t block_id, bool success);
	void WriteBack(char * filename);
};