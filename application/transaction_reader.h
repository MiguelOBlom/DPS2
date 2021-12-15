#include <fstream>
#include <iostream>
#include <vector>
#include <string.h>

#include "transaction.h"

#ifndef _TRANSACTIONREADER_
#define _TRANSACTIONREADER_

template <typename ID, unsigned int N>
class TransactionReader {
private:

public:

	static std::vector<Transactions<ID, N> > ReadFile(char* filename) {
		std::vector<Transactions<ID, N> > vec = std::vector<Transactions<ID, N> >();
		std::ifstream transaction_file;
		uint32_t sender, receiver;
		double amount;
		
		transaction_file.open(filename);
		
		while(!transaction_file.eof()) {
			struct Transactions<ID, N> transactions;
			memset(&transactions, 0, sizeof(transactions));
			for (unsigned int i = 0; i < N && !transaction_file.eof(); ++i) {
				transaction_file >> transactions.transaction[i].sender >> transactions.transaction[i].receiver >> transactions.transaction[i].amount;
			}
			vec.push_back(transactions);

		}
		return vec;
	}


};

#endif