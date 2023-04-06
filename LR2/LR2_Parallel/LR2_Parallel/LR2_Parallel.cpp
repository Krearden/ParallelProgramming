//������������ ������ 2, ������� 21
//�������� ������� 3-�� ����� ����������� ���������� ������������ ������ (��-11)
//2023 �.


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
	int channels;
	int partial_rows, partial_cols;
	Mat input_image, temp_image;

	if (rank == 0)
	{
		// ��������� ����������� � ����������
		input_image = imread("C:\\Users\\User\\Documents\\ParallelProgramming\\LR2\\LR2_Step_by_step\\images\\input_1024x1024.png");

		//��������
		if (input_image.empty()) {
			printf("\nCould not read the image");
			MPI_Finalize();
			return 1;
		}

		// ���������� ������� ��������� �����������
		input_rows = input_image.rows;
		input_cols = input_image.cols;
		
		//������� ���������� ����������� � ����������� padding border replicate
		temp_rows = input_rows + PADDING * 2;
		temp_cols = input_cols + PADDING * 2;

		//������� ��������� ����������� � ����������� ��������� 
		temp_image = Mat::zeros(input_image.size(), input_image.type());
		copyMakeBorder(input_image, temp_image, PADDING, PADDING, PADDING, PADDING, BORDER_REPLICATE);

		//����� ������� �����������, � �������� ������� ������
		channels = temp_image.channels();

		//������� ������� �� partial images
		partial_rows = temp_rows / procs_amount;
		partial_cols = temp_cols;

		printf("Channels = %d", input_image.channels());
	}

	//�������� ������ ��� ����� ����������� �� ������ ��������
	Mat partial_image(partial_rows, partial_cols, CV_8UC3);

	// Scatter the sub-images to each process
	MPI_Scatter(temp_image.data, partial_rows * partial_cols * channels, MPI_BYTE,
		partial_image.data, partial_rows * partial_cols * channels, MPI_BYTE,
		0, MPI_COMM_WORLD);
	
	MPI_Barrier;

	//imshow("Partial image", partial_image);
	//waitKey(0);



	printf("\ntotal procs = %d, i'm number %d; parial_size = %dx%d", procs_amount, rank, partial_image.rows, partial_image.cols);

	MPI_Finalize();
	return 0;
}
