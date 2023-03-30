#include <iostream>
#include <opencv2/opencv.hpp>


using namespace std;
using namespace cv;


//������������ ������ 2, ������� 21
//�������� ������� 3-�� ����� ����������� ���������� ������������ ������ (��-11)
//2023 �.


int main() {
    // ��������� ����������� � ����������
    Mat input_image = imread("C:\\Users\\User\\Documents\\ParallelProgramming\\LR2\\input2.jpg");

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
    int temp_rows = rows + 3 * 2;
    int temp_cols = cols + 3 * 2;

    //������� ��������� ����������� � ����������� ��������� 
    Mat temp_image;
    RNG rng(12345);
    Scalar value(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
    copyMakeBorder(input_image, temp_image, 3, 3, 3, 3, BORDER_REPLICATE, value);

    //������� ����� �����������
    Mat output_image = Mat::ones(rows, cols, CV_8UC3);


    //���� ��� ���������� ���������� �������
    //���������� �� ���� �������� �����������, ����� padding
    Vec3b temp_pixel(0, 0, 0);
    Mat pixel_neighborhood;
    for (int i = 3; i < temp_rows - 3; i++)
    {
        for (int j = 3; j < temp_cols - 3; j++)
        {
            // �������� �������� �������� � ����� � ������� �����������
            pixel_neighborhood = temp_image.rowRange(i - 3, i + 3 + 1)
                .colRange(j - 3, j + 3 + 1);

            for (int k = 0; k < 7; k++)
            {
                for (int l = 0; l < 7; l++)
                {
                    temp_pixel += pixel_neighborhood.at<Vec3b>(k, l) * extensionMatix[k][l];
                }
            }

            //���������� �������, ���������� � ���������� �������� �������, � ����� ��������
            printf("i = %d, j = %d", i, j);
            output_image.at<Vec3b>(i - 3, j - 3) = temp_pixel;
            temp_pixel = Vec3b(0, 0, 0);
        }
    }


    // ���������� �������� � ������������ �����������
    imshow("Original Image", input_image);
    //imshow("Temp Original Image", temp_image);
    imshow("Output Image", output_image);
    waitKey(0);

    return 0;
}