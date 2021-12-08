mpicc -o main.out main.c 
mpirun -np "$1" --oversubscribe ./main.out