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

positions = np.array([4, 5, 6, 7, 12, 17, 22, 27, 32])
labels = positions - 2
print(labels)

data_files = [
    "data.txt"
]

df = pandas.read_csv(data_files[0], lineterminator='\n', header=0, delim_whitespace=True)
df = df.drop(labels=["repetition"], axis=1)
data = []

for i, v in enumerate(positions):
    data.append(df.loc[df["nodes"] == v]["time"].reset_index(drop=True))

df = pandas.concat(data, axis=1, ignore_index=True)

# Scaling boxplot
fig1 = plt.figure()
ax1 = fig1.add_subplot(111)
ax1.tick_params(axis='both', direction='in', length=8)
ax1.set_ylabel("join time (s)")
ax1.set_xlabel("number of peers already in network")
ax1.set_xlim([0, 40])
# ax5.set_xscale('log', basex=2)
ax1.set_title("Time needed to join a network vs the number of active peers already in network.")
ax1.boxplot(df, labels=labels, whis=[1,99], positions=positions, widths=0.8)
ax1.set_ylim([0,150])
fig1.set_size_inches(size[0]*2,size[1])
fig1.savefig("join_time.png", dpi=300)
# plt.show()

sys.exit()