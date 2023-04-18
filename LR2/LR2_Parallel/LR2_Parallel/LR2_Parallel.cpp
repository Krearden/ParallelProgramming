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
	double start_time, end_time, duration;

	MPI_Barrier(MPI_COMM_WORLD);
	start_time = MPI_Wtime();

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
		else if (input_image.rows % procs_amount != 0)
		{
			printf("\nНеподходящая высота изображения!");
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
	MPI_Barrier(MPI_COMM_WORLD);

	//imshow("partial img #" + std::to_string(rank) +" " + std::to_string(partial_padding_img.cols) + "x" + std::to_string(partial_padding_img.rows), partial_padding_img);
	//waitKey(0);




	// --Итак, теперь надо обработать каждую часть изображения с добавленным padding
	int extensionMatix[7][7] = {
		{0, 0, 0, 1, 0, 0, 0},
		{0, 0, 0, 1, 0, 0, 0},
		{0, 0, 0, 1, 0, 0, 0},
		{1, 1, 1, 1, 1, 1, 1},
		{0, 0, 0, 1, 0, 0, 0},
		{0, 0, 0, 1, 0, 0, 0},
		{0, 0, 0, 1, 0, 0, 0},
	};
	
	Mat partial_output_img(partial_rows, partial_cols, CV_8UC3);

	//цикл для применения матричного фильтра
	//проходимся по всем пикселям изображения, кроме padding
	Mat pixel_neighborhood;
	Vec3b temp_pixel(0, 0, 0);
	int r = 0, g = 0, b = 0;

	for (int i = PADDING; i < partial_padding_img.rows - PADDING; i++)
	{
		for (int j = PADDING; j < partial_padding_img.cols - PADDING; j++)
		{
			// Выбираем диапазон столбцов и строк в матрице изображения
			pixel_neighborhood = partial_padding_img.rowRange(i - PADDING, i + PADDING + 1)
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
			partial_output_img.at<Vec3b>(i - PADDING, j - PADDING) = temp_pixel;

			//обнуляем значения для следующей итерации
			temp_pixel = Vec3b(0, 0, 0);
			b = 0; g = 0; r = 0;
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);

	//imshow("partial img #" + std::to_string(rank) +" " + std::to_string(partial_output_img.cols) + "x" + std::to_string(partial_output_img.rows), partial_output_img);
	//waitKey(0);


	//Собираем выходное изображение по частям
	Mat output_image;

	if (rank == 0) {
		// выделяем память для массива на корневом процессе
		output_image = Mat::zeros(input_rows, input_cols, partial_image.type());
	}
	

	MPI_Gather(partial_output_img.data, partial_output_img.rows * partial_output_img.cols * channels, MPI_BYTE,
		output_image.data, partial_output_img.rows * partial_output_img.cols * channels, MPI_BYTE,
		0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);
	end_time = MPI_Wtime();
	duration = end_time - start_time;


	if (rank == 0)
	{
		

		
		
		printf("%dx%d\nDuration = %f seconds", input_rows, input_cols, duration);

		//imshow("Output #" + std::to_string(rank) + " " + std::to_string(output_image.cols) + "x" + std::to_string(output_image.rows), output_image);
		//waitKey(0);
	}
	

	MPI_Finalize();
	return 0;
}