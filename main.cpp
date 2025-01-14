#include <mpi.h>
#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
	int size,rank, len;
	char processor[100];
	MPI_Init(&argc,&argv);

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Get_processor_name(processor, &len);

	cout << "Hello world: " << rank << " of " << size << " na (" << processor << ")\n";
	MPI_Finalize();
}