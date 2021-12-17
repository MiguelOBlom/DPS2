import matplotlib.pyplot as plt
import numpy as np
import pandas
import datetime
import glob
import csv
import sys

plt.rcParams.update({'font.size': 13})

cm = 1/2.54
size = [14*cm, 18*cm]

positions = np.array([100, 50, 20, 10])


data_files = [
    "blocksize_vs_time_2.txt",
]

df = pandas.read_csv(data_files[0], lineterminator='\n', header=0, delim_whitespace=True)
df = df.drop(labels=["repetition"], axis=1)
data = []

for i, v in enumerate(positions):
    data.append(df.loc[df['blocksize'] == v]["time"].reset_index(drop=True))

df = pandas.concat(data, axis=1, ignore_index=True)

# Scaling boxplot
fig1 = plt.figure()
ax1 = fig1.add_subplot(111)
ax1.tick_params(axis='both', direction='in', length=8)
ax1.set_ylabel("process time (s)")
ax1.set_xlabel("number of transactions per block")
ax1.set_xlim([0, 120])
# ax5.set_xscale('log', basex=2)
ax1.set_title("Process time for 100 transactions divided over different block sizes\non a network of 20 nodes.")
ax1.boxplot(df, labels=positions, whis=[1,99], positions=positions, widths=2)
ax1.set_ylim([0,60])
fig1.set_size_inches(size[0]*2,size[1])
fig1.savefig("addition_time_20_nodes.png", dpi=300)
# plt.show()

sys.exit()
# Scaling boxplot
fig6 = plt.figure()
ax6 = fig6.add_subplot(111)
ax6.tick_params(axis='both', direction='in', length=8)
ax6.set_ylabel("Runtime in seconds")
ax6.set_xlabel("Number of nodes")
ax6.set_xlim([0, 72])
# ax5.set_xscale('log', basex=2)
ax6.set_title("GraphX strong scaling of Conn. Comp. on Twitter data (replication)")
positions = np.array([8,16,32,48,64])
ax6.plot(bs_vs_t.loc[:5], labels=positions, whis=[1,99], positions=positions, widths=2)
ax6.set_ylim([100,275])
fig6.set_size_inches(size[0]*2,size[1])
fig6.savefig("images/graphx_scaling.png", dpi=300)
# plt.show()

sys.exit()