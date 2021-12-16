# Installation
Clone our github page on the DAS5 in your home directory.
Next, change the directory to the application directory and run `make` to build the project.
Cleaning the project can be performed by executing `make clean` in the same folder.

# Execution
For instance, we can run 2 peers (and 1 tracker making 3 in total), allocated for 10 minutes.
We will process 100 transactions distributed uniformly random over the peers.
The peers will be started 5 seconds apart.
`./start_das5.sh 3 00:10:00 100 5`

# Draw.io Pictures
To edit the images, go to draw.io and load them.

The envisioned system model:

![The envisioned system model](imgs/System_model.drawio.png)

The peer-to-peer blockchain model:

![The peer-to-peer blockchain model](imgs/P2Pblockchain.drawio.png)
