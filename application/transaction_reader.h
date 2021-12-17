/*
	A class used for reading transaction data from 
	a file and returning a vector of transactions.
	This class is generic, since it should generate
	generic type objects. The ID type is the type used
	for the identities of the sender and receiver and
	can be set to modify the length of the IDs.
	The Transactions class also requires N to be the
	maximum number of blocks included in each batch
	of transactions. 

	Author: Miguel Blom

*/


#include <fstream>
#include <iostream>
#include <vector>
#include <string.h>

#include "config.h"
#include "transaction.h"

#ifndef _TRANSACTIONREADER_
#define _TRANSACTIONREADER_

template <typename ID, unsigned int N>
class TransactionReader {
public:
	// Read a set of transaction batches from a file specified by the filename
	static std::vector<Transactions<ID, N> > ReadFile(char* filename){
		struct Transactions<ID, N> transactions;
		std::vector<Transactions<ID, N> > vec = std::vector<Transactions<ID, N> >();
		std::ifstream transaction_file;
		ID_TYPE sender, receiver;
		double amount;
		unsigned int i;
		
		transaction_file.open(filename);

		while(!transaction_file.eof()) {
			// Initialize the transactions to zero
			memset(&transactions, 0, sizeof(transactions));
			
			for (i = 0; i < N && !transaction_file.eof(); ++i) {
				// Read the contents
				transaction_file >> sender >> receiver >> amount;

				// Check if valid data
				if (sender == 0 && receiver == 0 && amount == 0) {
					i--; // yes, this is hacky, but we ++i right after
				} else {
					sender = transactions.transaction[i].sender;
					receiver = transactions.transaction[i].receiver;
					amount = transactions.transaction[i].amount;
				}
			}

			// If any valid transactions have been read
			if (i != 0) {
				// Add it to our list of transactions
				vec.push_back(transactions);
			}

		}
		return vec;
	}

};

#endif
