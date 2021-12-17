/*
	Author: Miguel Blom
*/

#include "transaction_reader.h"

// The following code can be uncommented to test out this class
/*
int main(int argc, char**argv) {
	std::vector<Transactions<char, 5> > vec = TransactionReader<char, 5>::ReadFile(argv[1]);

	for (auto item : vec) {
		for ( unsigned int i = 0; i < 5; i++ ) {
			std::cout << item.transaction[i].sender << " " << item.transaction[i].receiver << " " << item.transaction[i].amount << std::endl;
		}
	}

}
*/
