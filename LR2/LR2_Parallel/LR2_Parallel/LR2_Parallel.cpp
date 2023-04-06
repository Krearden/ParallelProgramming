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

	int partial_rows, partial_cols;
	int channels;
	Mat temp_image;

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

		//создаем временное изображение с добавленным паддингом 
		temp_image = Mat::zeros(input_image.size(), input_image.type());
		copyMakeBorder(input_image, temp_image, PADDING, PADDING, PADDING, PADDING, BORDER_REPLICATE);

		//данные изобаржения с replicate padding
		int temp_rows = temp_image.rows;
		int temp_cols = temp_image.cols;

		//вычисляем размеры каждой части изображения
		partial_rows = temp_rows / procs_amount;
		partial_cols = temp_cols;

		//число каналов изображений, с которыми ведется работа
		channels = temp_image.channels();
	}

	MPI_Bcast(&partial_rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&partial_cols, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&channels, 1, MPI_INT, 0, MPI_COMM_WORLD);

	//выделяем память для части изображения на каждом процессе
	Mat partial_image(partial_rows, partial_cols, CV_8UC3);

	//делим изображение между процессами
	MPI_Scatter(temp_image.data, partial_rows * partial_cols * channels, MPI_BYTE,
		partial_image.data, partial_rows * partial_cols * channels, MPI_BYTE,
		0, MPI_COMM_WORLD);

	imshow("Partial img", partial_image);
	waitKey(0);

	printf("\ntotal procs = %d, i'm number %d; pr = %d, pc = %d, ch = %d, img = %d", procs_amount, rank, partial_rows, partial_cols, channels, partial_image.at<Vec3b>(0, 0)[0]);

	MPI_Finalize();
	return 0;
}
