// lab3_parallel.cpp: определяет точку входа для приложения.
//

#include <iostream>
#include <mpi.h>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <exception>
using namespace std;
using f_function = double(double x);

double L(double u1, double u2, double u3, double step_in_space, double step_in_time) {
	double q = step_in_time / (step_in_space * step_in_space);
	return q * (u1 + u3) + (1 - 2 * q) * u2;
}

void linearCommunication(double left_send, double right_send, double* left_get, double* right_get, double left, double right, MPI_Comm MPI_COMM_LINEAR)
{
	// Получение рангов левого и правого соседей
	int left_rank, right_rank, rank;
	MPI_Cart_shift(MPI_COMM_LINEAR, 0, -1, &rank, &left_rank);
	MPI_Cart_shift(MPI_COMM_LINEAR, 0, 1, &rank, &right_rank);

	// Обмен значениями с левым соседом
	if (left_rank == MPI_PROC_NULL) {
		// Если левого соседа нет, присваиваем левое граничное условие
		*left_get = left;
	}
	else {
		// Принимаем значение от левого соседа
		MPI_Recv(left_get, 1, MPI_DOUBLE, left_rank, 0, MPI_COMM_LINEAR, MPI_STATUS_IGNORE);
		// Отправляем значение левому соседу
		MPI_Send(&left_send, 1, MPI_DOUBLE, left_rank, 0, MPI_COMM_LINEAR);
	}

	// Обмен значениями с правым соседом
	if (right_rank == MPI_PROC_NULL) {
		// Если правого соседа нет, присваиваем правое граничное условие
		*right_get = right;
	}
	else {
		// Отправляем значение правому соседу
		MPI_Send(&right_send, 1, MPI_DOUBLE, right_rank, 0, MPI_COMM_LINEAR);
		// Принимаем значение от правого соседа
		MPI_Recv(right_get, 1, MPI_DOUBLE, right_rank, 0, MPI_COMM_LINEAR, MPI_STATUSES_IGNORE);
	}
}

void solveEquation(vector<double>& result_grid, vector<double>& init, vector<double>& left, vector<double>& right, int N, int T, double step_in_space, double step_in_time, int rank, int size) {
	//создание линейного коммуникатора 
	MPI_Comm MPI_COMM_LINEAR;
	vector<int> dims = { size };
	vector<int> periods = { 0 };
	int reorder = 0;
	MPI_Cart_create(MPI_COMM_WORLD, 1, dims.data(), periods.data(), reorder, &MPI_COMM_LINEAR);

	//броадкаст размеров пространственной и временной сеток всем процессам коммутатора MPI_COMM_LINEAR
	MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_LINEAR);
	MPI_Bcast(&T, 1, MPI_INT, 0, MPI_COMM_LINEAR);

	//количество данных для работы каждого процесса (за искл. граничных элементов)
	int xs_per_process = ceil((double)(N - 2) / size);
	int xs_for_last_process = (N - 2) - xs_per_process * (size - 1);

	vector<double> partition; //partition - массив, в котором хранятся элементы всех слоев последовательно
	partition = (rank != (size - 1) ? vector<double>(T * (xs_per_process + 2), 0) : vector<double>(T * (xs_for_last_process + 2), 0));

	//Распределение данных между процессами с пом. Scatterv
	vector<int> sendcounts(size, xs_per_process);
	sendcounts[size - 1] = xs_for_last_process;
	vector<int> displs(size, 0);
	for (int i = 0; i < size; i++)
		displs[i] = xs_per_process * i;
	MPI_Scatterv(init.data() + 1, sendcounts.data(), displs.data(), MPI_DOUBLE, partition.data() + 1, sendcounts[rank], MPI_DOUBLE, 0, MPI_COMM_LINEAR);


	double left_temp = 0;
	double right_temp = 0;
	int x_len = 0;
	x_len = (rank != size - 1 ? (xs_per_process + 2) : (xs_for_last_process + 2));

	size_t temp, temp2, temp3;
	for (int i = 0; i < T - 1; i++) {
		if (rank == 0) {
			left_temp = left[i];
		}
		if (rank == size - 1) {
			right_temp = right[i];
		}
		temp = i * x_len;
		temp2 = (i + 1) * x_len;
		temp3 = (T - 1) * x_len; 
		//обмен значениями граничных точек между соседними процессами
		linearCommunication(partition[temp + 1], partition[temp + x_len - 2], &(partition[temp + 0]), &(partition[temp + x_len - 1]), left_temp, right_temp, MPI_COMM_LINEAR);
		for (int j = 1; j < x_len - 1; j++) {
			partition[temp2 + j] = L(partition[temp + j - 1], partition[temp + j], partition[temp + j + 1], step_in_space, step_in_time);
		}
	}
	linearCommunication(partition[temp3 + 1], partition[temp3 + x_len - 2], &(partition[temp3 + 0]), &(partition[temp3 + x_len - 1]), left_temp, right_temp, MPI_COMM_LINEAR);
	if (rank == 0) {
		result_grid = vector<double>(T * N, 0);
	}
	MPI_Datatype MPI_SURFACE_LINE_REGULAR;
	MPI_Type_vector(T, xs_per_process, xs_per_process + 2, MPI_DOUBLE, &MPI_SURFACE_LINE_REGULAR);
	MPI_Type_commit(&MPI_SURFACE_LINE_REGULAR);
	MPI_Datatype MPI_SURFACE_LINE_LAST;
	MPI_Type_vector(T, xs_for_last_process, xs_for_last_process + 2, MPI_DOUBLE, &MPI_SURFACE_LINE_LAST);
	MPI_Type_commit(&MPI_SURFACE_LINE_LAST);
	MPI_Datatype MPI_SURFACE_LINE_REGULAR_RECV;
	MPI_Type_vector(T, xs_per_process, N, MPI_DOUBLE, &MPI_SURFACE_LINE_REGULAR_RECV);
	MPI_Type_commit(&MPI_SURFACE_LINE_REGULAR_RECV);
	MPI_Datatype MPI_SURFACE_LINE_LAST_RECV;
	MPI_Type_vector(T, xs_for_last_process, N, MPI_DOUBLE, &MPI_SURFACE_LINE_LAST_RECV);
	MPI_Type_commit(&MPI_SURFACE_LINE_LAST_RECV);
	vector<int> ranks = { size - 1 };
	vector<int> counts2(size - 1, 1);
	vector<int> displs2(size - 1, 0);
	for (int i = 0; i < size - 1; i++) {
		displs2[i] = displs[i];
	}

	if (rank == 0) {
		for (int i = 0; i < T; i++) {
			temp = i * N;
			temp2 = i * (xs_per_process + 2);
			for (int j = 0; j < xs_per_process; j++) {
				result_grid[temp + j + 1] = partition[temp2 + j + 1];
			}
		}
	}
	if (rank == 0) {
		for (int i = 1; i < size - 1; i++) {
			MPI_Recv(result_grid.data() + displs2[i] + 1, 1, MPI_SURFACE_LINE_REGULAR_RECV, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
	}
	if (rank != 0 && rank != size - 1) {
		MPI_Send(partition.data() + 1, 1, MPI_SURFACE_LINE_REGULAR, 0, 0, MPI_COMM_WORLD);
	}
	if (rank == size - 1) {
		MPI_Send(partition.data() + 1, 1, MPI_SURFACE_LINE_LAST, 0, 0, MPI_COMM_WORLD);
	}
	else if (rank == 0) {
		MPI_Recv(result_grid.data() + displs[size - 1] + 1, 1, MPI_SURFACE_LINE_LAST_RECV, size - 1, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
	}
	if (rank == 0) {
		for (int i = 0; i < T; i++) {
			result_grid[i * N + 0] = left[i];
			result_grid[i * N + N - 1] = right[i];
		}
	}
	MPI_Type_free(&MPI_SURFACE_LINE_REGULAR);
	MPI_Type_free(&MPI_SURFACE_LINE_LAST);
	MPI_Type_free(&MPI_SURFACE_LINE_REGULAR_RECV);
	MPI_Type_free(&MPI_SURFACE_LINE_LAST_RECV);
	MPI_Comm_free(&MPI_COMM_LINEAR);
}



double f(double x) {
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

double f_left(double x) {
	return f(0);
}

double f_right(double x) {
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

void writeToFile(vector<double>& result_grid, string output_filename, int N, int T, double step_in_space, double step_in_time)
{
	ofstream fout(output_filename);
	fout << 0;
	for (size_t i = 0; i < N; i++) {
		fout << "," << i * step_in_space;
	}
	fout << endl;
	for (size_t i = 0; i < T; i++) {
		fout << i * step_in_time;
		for (size_t j = 0; j < N; j++) {
			fout << "," << result_grid[i * N + j];
		}
		fout << endl;
	}
	fout.close();
}

int main(int argc, char** argv)
{
	try {
		int rank, size;
		MPI_Init(&argc, &argv);
		MPI_Comm_size(MPI_COMM_WORLD, &size);
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);

		//кол-во точек в пространственной области
		int N = 100;
		//кол-во моментов времени, на котор. делится интервал от 0 до T
		int T = 1000;
		string output_filename = "output_N" + to_string(N) + "_T" + to_string(T) + ".txt";
		vector<double> init;
		vector<double> left;
		vector<double> right;
		vector<double> result_grid;
		double start_time, end_time, time, total_time = 0;
		double step_in_time = 0;
		double step_in_space = 0;


		//заполняем вектора начального и краевых условий на нулевом процессе
		if (rank == 0) {
			step_in_space = getInitValues(f, N, 1, init);
			step_in_time = getInitValues(f_left, T, 0.4, left);
			getInitValues(f_right, T, 0.4, right);
		}
		//заполням вектор краевого условия на последнем процессе
		if (rank == size - 1) {
			getInitValues(f_right, T, 0.4, right);
		}

		MPI_Bcast(&step_in_time, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&step_in_space, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

		//замер времени 1000 запусков
		
		for (int i = 0; i < 1000; i++)
		{
			/*MPI_Barrier() для синхронизации всех процессов перед началом каждого запуска, 
			чтобы убедиться, что все процессы начинают одновременно и измерение времени происходит синхронно.*/
			MPI_Barrier(MPI_COMM_WORLD);
			start_time = MPI_Wtime();
			solveEquation(result_grid, init, left, right, N, T, step_in_space, step_in_time, rank, size);
			end_time = MPI_Wtime();
			total_time += end_time - start_time;
		}
		
		

		//запись в файл
		if (rank == 0) {
			writeToFile(result_grid, output_filename, N, T, step_in_space, step_in_time);
		}

		//среднее время с тысячи запусков
		time = total_time / 1000;
		//находим максимальное время среди всех процессов и выводим его на экран
		double max_time;
		MPI_Reduce(&time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
		if (rank == 0) {
			printf("Runtime: %.6f\n", max_time);
		}
		MPI_Finalize();
	}
	catch (exception e) {
		cout << e.what() << endl;
	}
	return 0;
}
