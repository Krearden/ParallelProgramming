//Лабораторная работа 2, вариант 21
//Выполнил студент 3-го курса физического факультета Запорожченко Кирилл (ФЗ-11)
//2023 г.


#include "mpi.h"
#include <opencv2/opencv.hpp>
#include <iostream>


#define PADDING 3
#define NORM_COEFICIENT 13


using namespace std;
using namespace cv;


int main(int argc, char** argv)
{
	int rank, procs_amount;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &procs_amount);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int input_rows, input_cols;
	int temp_rows, temp_cols;

	if (rank == 0)
	{
		// Загружаем изображение с компьютера
		Mat input_image = imread("C:\\Users\\User\\Documents\\ParallelProgramming\\LR2\\LR2_Step_by_step\\images\\input_1024x1024.png");

		//проверка
		if (input_image.empty()) {
			printf("\nCould not read the image");
			MPI_Finalize();
			return 1;
		}

		// определяем размеры исходного изображения
		input_rows = input_image.rows;
		input_cols = input_image.cols;
		
		//размеры временного изображения, прим. для прим. матричного фильтра наращивания
		temp_rows = input_rows + PADDING * 2;
		temp_cols = input_cols + PADDING * 2;

		//создаем временное изображение с добавленным паддингом 
		Mat temp_image = Mat::zeros(input_image.size(), input_image.type());
		copyMakeBorder(input_image, temp_image, PADDING, PADDING, PADDING, PADDING, BORDER_REPLICATE);

		// Compute the number of rows and columns for each process
		int num_rows_per_process = temp_image.rows / procs_amount;
		int num_cols_per_process = temp_image.cols;
	}

	//передаем всем процессам размеры исходного изображения
	MPI_Bcast(&input_rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&input_cols, 1, MPI_INT, 0, MPI_COMM_WORLD);


	printf("\nTotal procs = %d, I'm number %d; rows = %d", procs_amount, rank, input_rows);

	MPI_Finalize();
	return 0;
}
