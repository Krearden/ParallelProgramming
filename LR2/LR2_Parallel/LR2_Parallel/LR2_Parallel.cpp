//Лабораторная работа 2, вариант 21 - часть с параллельной программной
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

	int partial_rows, partial_cols, input_cols, input_rows;
	int channels;
	Mat input_image;


	if (rank == 0)
	{
		// Загружаем изображение с компьютера
		input_image = imread("C:\\Users\\User\\Documents\\ParallelProgramming\\LR2\\LR2_Step_by_step\\images\\input_1024x1024.png");

		//проверка
		if (input_image.empty()) {
			printf("\nCould not read the image");
			MPI_Finalize();
			return 1;
		}

		//размеры изначального изображения
		input_rows = input_image.rows;
		input_cols = input_image.cols;

		//вычисляем размеры каждой части изображения с добавленным паддингом
		partial_rows = input_rows / procs_amount;
		partial_cols = input_cols;

		//число каналов изображений, с которыми ведется работа
		channels = input_image.channels();
	}

	MPI_Bcast(&partial_rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&partial_cols, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&channels, 1, MPI_INT, 0, MPI_COMM_WORLD);

	//выделяем память для части изображения на каждом процессе
	Mat partial_image(partial_rows, partial_cols, CV_8UC3);
	
	//делим изображение между процессами
	MPI_Scatter(input_image.data, partial_rows * partial_cols * channels, MPI_BYTE,
		partial_image.data, partial_rows * partial_cols * channels, MPI_BYTE,
		0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

	//imshow("partial img #" + std::to_string(rank) +" " + std::to_string(partial_image.cols) + "x" + std::to_string(partial_image.rows), partial_image);
	//waitKey(0);



	//add padding replicate
	if (rank == 0)
	{
		copyMakeBorder(partial_image, partial_image, PADDING, 0, PADDING, PADDING, BORDER_REPLICATE); //replicate padding

	}
	else if (rank == (procs_amount - 1))
	{
		copyMakeBorder(partial_image, partial_image, 0, PADDING, PADDING, PADDING, BORDER_REPLICATE); //replicate padding

	}
	else
	{
		copyMakeBorder(partial_image, partial_image, 0, 0, PADDING, PADDING, BORDER_REPLICATE); //replicate padding
	}
	MPI_Barrier(MPI_COMM_WORLD);


	//add padding, состоящий из частей других частей изображений
	int partial_padding_cols = partial_cols + PADDING * 2;

	Rect top_strip(0, 0, partial_padding_cols, 3);
	Mat top_strip_image = partial_image(top_strip);

	Rect bottom_strip(0, partial_rows - 3, partial_padding_cols, 3);
	Mat bottom_strip_image = partial_image(bottom_strip);

	int data_count = top_strip_image.total() * top_strip_image.elemSize(); // количество байт в каждой полоске

	Mat partial_padding_img;

	if (rank == 0)
	{
		int proc_to_comm = 1;
		MPI_Send(bottom_strip_image.data, data_count, MPI_BYTE, proc_to_comm, 0, MPI_COMM_WORLD);

		Mat received_strip_for_bottom_padding(3, partial_padding_cols, CV_8UC3);
		MPI_Recv(received_strip_for_bottom_padding.data, data_count, MPI_BYTE,
			proc_to_comm, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		vconcat(partial_image, received_strip_for_bottom_padding, partial_padding_img);

	}
	else if (rank == (procs_amount - 1))
	{
		int proc_to_comm = (rank - 1);
		MPI_Send(top_strip_image.data, data_count, MPI_BYTE, proc_to_comm, 0, MPI_COMM_WORLD);

		Mat received_strip_for_top_padding(3, partial_padding_cols, CV_8UC3);
		MPI_Recv(received_strip_for_top_padding.data, data_count, MPI_BYTE,
			proc_to_comm, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		vconcat(received_strip_for_top_padding, partial_image, partial_padding_img);
	}
	else
	{
		int proc_above = (rank - 1);
		int proc_below = (rank + 1);
		MPI_Send(top_strip_image.data, data_count, MPI_BYTE, proc_above, 0, MPI_COMM_WORLD);
		MPI_Send(bottom_strip_image.data, data_count, MPI_BYTE, proc_below, 0, MPI_COMM_WORLD);

		Mat received_strip_for_top_padding(3, partial_padding_cols, CV_8UC3);
		Mat received_strip_for_bottom_padding(3, partial_padding_cols, CV_8UC3);
		MPI_Recv(received_strip_for_top_padding.data, data_count, MPI_BYTE,
			proc_above, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(received_strip_for_bottom_padding.data, data_count, MPI_BYTE,
			proc_below, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		Mat temp;
		vconcat(received_strip_for_top_padding, partial_image, temp);
		vconcat(temp, received_strip_for_bottom_padding, partial_padding_img);
	}

	imshow("partial img #" + std::to_string(rank) +" " + std::to_string(partial_padding_img.cols) + "x" + std::to_string(partial_padding_img.rows), partial_padding_img);
	waitKey(0);

	printf("\ntotal procs = %d, i'm number %d; pr = %d, pc = %d", procs_amount, rank, partial_rows, partial_cols);

	MPI_Finalize();
	return 0;
}