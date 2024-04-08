# LCI Examples

## Compiling

`mkdir build; cd build`

`cmake ../ -DLCI_PATH="<path to LCI installation>"`

`make`

## Running

`mpirun -n 2 ./ping_pong_m_queue <number of ping pong iterations> <message size in bytes>`

or

`srun -n 2 --mpi=pmi2 ./ping_pong_m_queue <number of ping pong iterations> <message size in bytes>`
