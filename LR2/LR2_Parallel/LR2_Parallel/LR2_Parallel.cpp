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

	MPI_Barrier(MPI_COMM_WORLD);

	//полоска части изобаржения сверху высоты PADDING
	Rect top_strip(0, 0, partial_cols, 3);
	Mat top_strip_image = partial_image(top_strip);
	
	//полоска части изобаржения снизу высоты PADDING
	Rect bottom_strip(0, partial_rows - 3, partial_cols, 3);
	Mat bottom_strip_image = partial_image(bottom_strip);

	//размер полосок изображения в байтах
	int strip_size = top_strip_image.total()* top_strip_image.elemSize();


	if (rank == 0)
	{
		//send to 1
		int proccess_to_communicate = 1;
		// Отправить нижнюю полоску 1му процессу с помощью функции MPI_Send
		MPI_Send(bottom_strip_image.data, strip_size, MPI_BYTE, proccess_to_communicate, 0, MPI_COMM_WORLD);
		
		//recieve from 1
		// Выделить память для получения полоски
		Mat received_top_strip_image_of_proccess_one(3, partial_cols, CV_8UC3);
		// Принять полоску с помощью функции MPI_Recv
		MPI_Recv(received_top_strip_image_of_proccess_one.data, strip_size, MPI_BYTE, proccess_to_communicate, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		imshow("received_top_strip_image_of_proccess_one", received_top_strip_image_of_proccess_one);
		waitKey(0);
		
	}
	else if (rank == (procs_amount - 1))
	{
		//send to (rank - 1)
		//send to (rank + 1)

		//recieve from (rank - 1)
		//revieve from (rank + 1)
	}
	else
	{
		//send to (rank - 1)
		//recieve from (rank - 1)
	}

	imshow("Partial img", partial_image);
	waitKey(0);

	printf("\ntotal procs = %d, i'm number %d; pr = %d, pc = %d, ch = %d, img = %d", procs_amount, rank, partial_rows, partial_cols, channels, partial_image.at<Vec3b>(0, 0)[0]);

	MPI_Finalize();
	return 0;
}
