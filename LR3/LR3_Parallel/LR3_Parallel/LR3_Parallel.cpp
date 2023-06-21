#include "mpi.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
using namespace std;
using f_function = double(double x);


//FUNCTIONS//

//Значение функции
double f(double x)
{
	if (x >= 0 && x <= 0.3)
	{
		return -20 * x + 6;
	}
	else if (x >= 0.3 && x <= 0.5)
	{
		return 0;
	}
	else
	{
		return 32 * x - 16;
	}
}

//Значение функции на левом конце отрезка (x = 0) для краевого условия 𝑢(0, 𝑡) = 𝑓(0)
double f_left(double x)
{
	return f(0);
}

//Значение функции на правом конце отрезка (x = 1) для краевого условия 𝑢(1, 𝑡) = 𝑓(1)
double f_right(double x)
{
	return f(1);
}

//Заполняет вектор начальных значений, возвращает шаг по пространству
double getInitValues(f_function f, int N, double interval, vector<double>& result)
{
	result = vector<double>(N, 0);
	double step = interval / (N - 1);
	for (int i = 0; i < N; i++)
	{
		result[i] = f(step * i);
	}

	return step;
}

//Явная трехточечная конечно-разностная схема
double L(double previousEl, double currentEl, double nextEl, double step_in_space, double step_in_time)
{
	double q = step_in_time / (step_in_space * step_in_space);
	return q * (previousEl + nextEl) + (1 - 2 * q) * currentEl;
}

void solveEquation(vector<double> init, vector<double>& left, vector<double>& right, int N, int T, int rank, int size)
{
	//создание линейного коммуникатора 
	MPI_Comm MPI_COMM_LINEAR;
	vector<int> dims = {size};	
	vector<int> periods = { 0 };
	int reorder = 0;
	MPI_Cart_create(MPI_COMM_WORLD, 1, dims.data(), periods.data(), reorder, &MPI_COMM_LINEAR);

	//броадкаст размеров пространственной и временной сеток всем процессам коммутатора MPI_COMM_LINEAR
	MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_LINEAR);
	MPI_Bcast(&T, 1, MPI_INT, 0, MPI_COMM_LINEAR);

	//количество данных для работы каждого процесса (за искл. граничных элементов)
	int xs_per_process = ceil((double)(N - 2) / size);
	int xs_for_last_process = (N - 2) - xs_per_process * (size - 1);

	vector<double> partition;
	partition = (rank != (size - 1) ? vector<double>(T * (xs_per_process + 2), 0) : vector<double>(T * (xs_for_last_process + 2), 0));

	//Распределение данных между процессами с пом. Scatterv
	vector<int> sendcounts(size, xs_per_process);
	sendcounts[size - 1] = xs_for_last_process;
	vector<int> displs(size, 0); 
	for (int i = 0; i < size; i++)
		displs[i] = xs_per_process * i;
	MPI_Scatterv(init.data() + 1, sendcounts.data(), displs.data(), MPI_DOUBLE, partition.data() + 1, sendcounts[rank], MPI_DOUBLE, 0, MPI_COMM_LINEAR);


	MPI_Comm_free(&MPI_COMM_LINEAR);
}

//Запись результирующей сетки в файл
void writeGridToFile(const string& filename, int N, int T, double step_in_space, double step_in_time, const vector<vector<double>>& result_grid)
{
	ofstream fout(filename);
	fout << 0;
	for (size_t i = 0; i < N; i++) {
		fout << "," << i * step_in_space;
	}
	fout << endl;
	for (size_t i = 0; i < T; i++) {
		fout << i * step_in_time;
		for (size_t j = 0; j < N; j++) {
			fout << "," << result_grid[i][j];
		}
		fout << endl;
	}
}


//MAIN//
int main(int argc, char** argv)
{
	int rank, size;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//кол-во точек в пространственной области
	int N = 10;
	//кол-во моментов времени, на котор. делится интервал от 0 до T
	int T = 100;
	string output_filename = "output_N" + to_string(N) + "_T" + to_string(T) + ".txt";
	vector<double> init;
	vector<double> left;
	vector<double> right;
	double step_in_space;
	double step_in_time;
	double start_time, end_time, total_time = 0;
	

	//заполняем вектора начального и краевых условий на нулевом процессе
	if (rank == 0) {
		step_in_space = getInitValues(f, N, 1, init);
		step_in_time = getInitValues(f_left, T, 0.4, left);
		getInitValues(f_right, T, 0.4, right);
	}

	//(1)

	MPI_Bcast(&step_in_time, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&step_in_space, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);


	start_time = MPI_Wtime();
	solveEquation(result_grid, init, left, right, step_in_space, step_in_time, rank, size);
	end_time = MPI_Wtime();
	total_time += end_time - start_time;

	if (rank == 0)
	{
		writeGridToFile(output_filename, N, T, step_in_space, step_in_time, result_grid);
	}
	
	cout << " Number of process = " << size << "   My rank = " << rank << endl;

	MPI_Finalize();
	return 0;
}
