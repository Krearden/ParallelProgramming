#include "mpi.h"
#include <stdio.h>
#include <iostream>
#include "time.h"
using namespace std;


//Лабораторная работа 1, I8 M4.
//Выполнил студент 3-го курса физического факультета Запорожченко Кирилл (ФЗ-11)


double getFx(double x)
{
	return 1 / (sqrt(x) - 1);
}

double get_h(double a, double b, double N)
{
	return (b - a) / N;
}

double simpsonMethod(double a, double b, double N)
{
	double h = get_h(a, b, N);
	double f_x0 = getFx(a);
	double sum_even = 0;
	double sum_uneven = 0;

	for (int i = 1; i < N; i += 2) 
	{
		sum_even += getFx(a + i * h);
		sum_uneven += getFx(a + (i + 1) * h);
	}

	return h/3 * (f_x0 + 2 * sum_even + 4 * sum_uneven);
}


int main(int argc, char** argv)
{
	//PARALLEL METHOD
	int N;
	double a, b;
	int local_N;
	double local_a, local_b, len;
	double result_parallel, local_result;
	double starttime, endtime, restime;
	double local_time;
	double error, ethalone = 2.86376645469595;

	int current_rank, amount_procs;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &current_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &amount_procs);

	//201 итерация
	for (N = 100000; N <= 2100000; N += 10000)
	{
		//Запуск таймера
		starttime = MPI_Wtime();

		//Если текущий процесс root, задаем начальные данные
		if (current_rank == 0)
		{
			a = 4;
			b = 8;
		}

		//Посылаем данные остальным процессам от процесса 0
		MPI_Bcast(&a, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&b, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

		MPI_Barrier(MPI_COMM_WORLD);

		//Вычисление локальных параметров интегрирования для текущего процесса
		len = (b - a) / amount_procs; //длина локального промежутка для текущего процесса
		local_N = N / amount_procs;
		local_a = a + current_rank * len;
		local_b = local_a + len;

		//Вычисление части интеграла текущим процессом
		local_time = MPI_Wtime();
		local_result = simpsonMethod(local_a, local_b, local_N);
		local_time = MPI_Wtime() - local_time;
		printf("\nProc #%d, local_integral = %f (N = %f, local_a = %f, local_b = %f); time = %f sec.", current_rank, local_result, local_N, local_a, local_b, local_time);

		MPI_Reduce(&local_result, &result_parallel, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
		MPI_Barrier(MPI_COMM_WORLD);

		//Стоп таймера
		endtime = MPI_Wtime();
		restime = endtime - starttime;

		if (current_rank == 0)
		{
			error = fabs(result_parallel - ethalone) / ethalone;
			printf("\n%d %f %.14f %e", N, restime, result_parallel, error);
		}
	}
	//printf("%f", max_time);
	MPI_Finalize();

	return 0;
}

