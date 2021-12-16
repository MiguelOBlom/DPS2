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
		ID_TYPE sender, receiver;
		double amount;
		
		transaction_file.open(filename);
		
		while(!transaction_file.eof()) {
			struct Transactions<ID, N> transactions;
			memset(&transactions, 0, sizeof(transactions));
			unsigned int i = 0;
			for (; i < N && !transaction_file.eof(); ++i) {
				transaction_file >> sender >> receiver >> amount;

				if (sender == 0 && receiver == 0 && amount == 0) {
					i--; // yes, but we ++i right after
				} else {
					sender = transactions.transaction[i].sender;
					receiver = transactions.transaction[i].receiver;
					amount = transactions.transaction[i].amount;
				}

			}

			if (i != 0) {
				vec.push_back(transactions);
			}

		}
		return vec;
	}


};

#endif