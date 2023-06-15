//������������ ������ 2, ������� 21 - ����� � ���������������� ����������
//�������� ������� 3-�� ����� ����������� ���������� ������������ ������ (��-11)
//2023 �.


#include <iostream>
#include <opencv2/opencv.hpp>
#include <omp.h>
#include <chrono>


#define PADDING 3
#define NORM_COEFICIENT 13


using namespace std;
using namespace cv;


int main() {
    system("chcp 65001");
    omp_set_num_threads(16);

    

    // ��������� ����������� � ����������
    Mat input_image = imread("C:\\Users\\User\\Documents\\ParallelProgramming\\LR2\\LR2_Step_by_step\\images\\input_4096x4096.png");
    auto start = std::chrono::high_resolution_clock::now();

    //��������
    if (input_image.empty()) {
        cout << "Could not read the image" << endl;
        return 1;
    }

    // ������� ������� ����������� 7x7
    int extensionMatix[7][7] = {
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0},
        {1, 1, 1, 1, 1, 1, 1},
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0},
    };

    //������� ��������� �����������
    int rows = input_image.rows;
    int cols = input_image.cols;

    //������� ���������� �����������, ����. ��� ����. ���������� ������� �����������
    int temp_rows = rows + PADDING * 2;
    int temp_cols = cols + PADDING * 2;

    //������� ��������� ����������� � ����������� ��������� 
    Mat temp_image = Mat::zeros(input_image.size(), input_image.type());
    copyMakeBorder(input_image, temp_image, PADDING, PADDING, PADDING, PADDING, BORDER_REPLICATE);

    //������� ����� ����������� 
    Mat output_image = Mat::zeros(input_image.size(), input_image.type());


    //���� ��� ���������� ���������� �������
    //���������� �� ���� �������� �����������, ����� padding

    #pragma omp parallel for
    for (int i = PADDING; i < temp_rows - PADDING; i++)
    {
        for (int j = PADDING; j < temp_cols - PADDING; j++)
        {
            int r_local = 0, g_local = 0, b_local = 0;

            // �������� �������� �������� � ����� � ������� �����������
            Mat pixel_neighborhood = temp_image.rowRange(i - PADDING, i + PADDING + 1)
                .colRange(j - PADDING, j + PADDING + 1);

            //��������� �������� �������
            for (int k = 0; k < 7; k++)
            {
                for (int l = 0; l < 7; l++)
                {
                    b_local += pixel_neighborhood.at<Vec3b>(k, l)[0] * extensionMatix[k][l];
                    g_local += pixel_neighborhood.at<Vec3b>(k, l)[1] * extensionMatix[k][l];
                    r_local += pixel_neighborhood.at<Vec3b>(k, l)[2] * extensionMatix[k][l];
                }
            }

            //���������
            r_local /= NORM_COEFICIENT;
            g_local /= NORM_COEFICIENT;
            b_local /= NORM_COEFICIENT;

            //���������� ��������� ������� � ����� �����������
            output_image.at<Vec3b>(i - PADDING, j - PADDING) = Vec3b(b_local, g_local, r_local);
        }
    }


    auto end = std::chrono::high_resolution_clock::now();
    auto duration_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double duration_seconds = (float)duration_microseconds.count() / 1000000;

    printf("%dx%d\nDuration (seconds) = %f", rows, cols, duration_seconds);

    imwrite("C:\\Users\\User\\Documents\\ParallelProgramming\\LR2\\LR2_Step_by_step\\images\\openmp_output\\" + std::to_string(rows) + "x" + std::to_string(cols) + ".png", output_image);

    //// ���������� ������������ �����������
    //imshow("Output Image", output_image);
    //waitKey(0);

    return 0;
}