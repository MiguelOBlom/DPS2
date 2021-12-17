# Generates transaction data as traces
# Author: Miguel Blom

import random
import sys
import numpy
import os
import time

MEAN=0.0
STDDEV=100.0
MIN=1.0

if len(sys.argv) < 3:
	print("Usage: " + sys.argv[0] + " <n_peers> <n_transactions>")
	exit()

n_peers = int(sys.argv[1])
n_transactions = int(sys.argv[2])

# Create storage
trace = list()
for i in range(n_peers):
	trace.append(list())


random.seed()

# Generate transactions uniform random
for i in range(n_transactions):
	sender = random.choice(range(0, n_peers))
	
	l = list(range(0, n_peers))
	l.remove(sender)
	receiver = random.choice(l)

	amount = abs(numpy.random.normal(loc=MEAN, scale=STDDEV)) + MIN

	trace[sender].append((str(sender), str(receiver), str(amount)))

# Per peer_id create a .trc file for transactions it has sent
name = str(int(time.time()))
os.mkdir(name)

# Write to files, grouped by the sender id
for i in range(n_peers):
	with open(os.path.join(name, str(i) + ".trc"), "w+") as f:
		for transaction in trace[i]:
			f.write("\t".join(transaction) + "\n")
			
print(name)
