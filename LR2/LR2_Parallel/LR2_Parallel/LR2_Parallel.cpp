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

		input_cols = input_image.cols;
		input_rows = input_image.rows;

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
	MPI_Bcast(&input_cols, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&input_rows, 1, MPI_INT, 0, MPI_COMM_WORLD);

	//выделяем память для части изображения на каждом процессе
	Mat partial_image(partial_rows, partial_cols, CV_8UC3);
	Mat partial_image_with_padding;

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
	int strip_size = top_strip_image.total() * top_strip_image.elemSize();


	//если нулевой процесс
	if (rank == 0)
	{
		//send to 1
		int proccess_to_communicate = 1;
		// Отправить нижнюю полоску 1му процессу с помощью функции MPI_Send
		MPI_Send(bottom_strip_image.data, strip_size, MPI_BYTE, proccess_to_communicate, 0, MPI_COMM_WORLD);
		
		//recieve from 1
		// Выделить память для получения полоски
		Mat received_strip_for_bottom_padding(3, partial_cols, CV_8UC3);

		// Принять полоску с помощью функции MPI_Recv
		MPI_Recv(received_strip_for_bottom_padding.data, strip_size, MPI_BYTE, proccess_to_communicate, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
		//объединяем
		vconcat(partial_image, received_strip_for_bottom_padding, partial_image_with_padding);
	}
	//если последний процесс
	else if (rank == (procs_amount - 1))
	{
		int proccess_to_communicate = (rank - 1);

		//send to (rank - 1)
		MPI_Send(top_strip_image.data, strip_size, MPI_BYTE, proccess_to_communicate, 0, MPI_COMM_WORLD);

		//recieve from (rank - 1)
		Mat received_strip_for_top_padding(3, partial_cols, CV_8UC3);
		MPI_Recv(received_strip_for_top_padding.data, strip_size, MPI_BYTE, proccess_to_communicate, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		//объединяем
		vconcat(received_strip_for_top_padding, partial_image, partial_image_with_padding);
	}
	//между 0 и последним
	else
	{
		int rank_minus_one = (rank - 1);
		int rank_plus_one = (rank + 1);

		//send to (rank - 1) top strip img
		MPI_Send(top_strip_image.data, strip_size, MPI_BYTE, rank_minus_one, 0, MPI_COMM_WORLD);

		//send to (rank + 1) bottom strip img
		MPI_Send(bottom_strip_image.data, strip_size, MPI_BYTE, rank_plus_one, 0, MPI_COMM_WORLD);

		//recieve from (rank - 1)
		Mat received_strip_for_top_padding(3, partial_cols, CV_8UC3);
		MPI_Recv(received_strip_for_top_padding.data, strip_size, MPI_BYTE, rank_minus_one, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
		//revieve from (rank + 1)
		Mat received_strip_for_bottom_padding(3, partial_cols, CV_8UC3);
		MPI_Recv(received_strip_for_bottom_padding.data, strip_size, MPI_BYTE, rank_plus_one, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		//объеденяем полученные полоски размера PADDING и часть изображения
		vconcat(received_strip_for_top_padding, partial_image, partial_image_with_padding);
		vconcat(partial_image_with_padding, received_strip_for_bottom_padding, partial_image_with_padding);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	//Теперь у каждого процесса есть своя часть изображения с добавленным padding, можно обрабатывать
	
	// Создаем матрицу наращивания 7x7
	int extensionMatix[7][7] = {
		{0, 0, 0, 1, 0, 0, 0},
		{0, 0, 0, 1, 0, 0, 0},
		{0, 0, 0, 1, 0, 0, 0},
		{1, 1, 1, 1, 1, 1, 1},
		{0, 0, 0, 1, 0, 0, 0},
		{0, 0, 0, 1, 0, 0, 0},
		{0, 0, 0, 1, 0, 0, 0},
	};

	//создаем новое изображение 
	int partial_output_cols = partial_image_with_padding.cols - PADDING * 2;
	int partial_output_rows = partial_image_with_padding.rows - PADDING * 2;
	Mat partial_output_image = Mat::zeros(partial_output_rows, partial_output_cols, partial_image_with_padding.type());

	//цикл для применения матричного фильтра
	//проходимся по всем пикселям изображения, кроме padding
	Mat pixel_neighborhood;
	Vec3b temp_pixel(0, 0, 0);
	int r = 0, g = 0, b = 0;

	for (int i = PADDING; i < partial_image_with_padding.rows - PADDING; i++)
	{
		for (int j = PADDING; j < partial_image_with_padding.cols - PADDING; j++)
		{
			// Выбираем диапазон столбцов и строк в матрице изображения
			pixel_neighborhood = partial_image_with_padding.rowRange(i - PADDING, i + PADDING + 1)
				.colRange(j - PADDING, j + PADDING + 1);

			//выполняем операцию свертки
			for (int k = 0; k < 7; k++)
			{
				for (int l = 0; l < 7; l++)
				{
					b += pixel_neighborhood.at<Vec3b>(k, l)[0] * extensionMatix[k][l];
					g += pixel_neighborhood.at<Vec3b>(k, l)[1] * extensionMatix[k][l];
					r += pixel_neighborhood.at<Vec3b>(k, l)[2] * extensionMatix[k][l];
				}
			}

			//нормируем
			r /= NORM_COEFICIENT;
			g /= NORM_COEFICIENT;
			b /= NORM_COEFICIENT;

			//присваеваем значения каналов пикселю
			temp_pixel[0] = b;
			temp_pixel[1] = g;
			temp_pixel[2] = r;

			//записываем найденный пиксель в новое изображение
			partial_output_image.at<Vec3b>(i - PADDING, j - PADDING) = temp_pixel;

			//обнуляем значения для следующей итерации
			temp_pixel = Vec3b(0, 0, 0);
			b = 0; g = 0; r = 0;
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);

	//Mat output_image;

	//if (rank == 0) {
	//	// выделяем память для массива на корневом процессе
	//	output_image = Mat::zeros(input_rows, input_cols, CV_8UC3);
	//}
	//MPI_Barrier(MPI_COMM_WORLD);
	//MPI_Gather(partial_output_image.data, partial_output_image.cols* partial_output_image.rows* channels, MPI_BYTE,
	//	output_image.data, output_image.rows* output_image.cols* channels, MPI_BYTE, 0, MPI_COMM_WORLD);


	imshow("Processed img no padding", partial_output_image);
	//imshow("Output", output_image);
	waitKey(0);

	printf("\ntotal procs = %d, i'm number %d; pr = %d, pc = %d, ch = %d, img = %d", procs_amount, rank, partial_rows, partial_cols, channels, partial_image.at<Vec3b>(0, 0)[0]);

	MPI_Finalize();
	return 0;
}
