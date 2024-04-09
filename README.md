# LCI Examples

## Compiling

`mkdir build; cd build`

`cmake ../ -DLCI_PATH="<path to LCI installation>"`

`make`

## Running

`mpirun -n 2 ./send_recv_s_sync`

or

`srun -n 2 --mpi=pmi2 ./send_recv_s_sync`
