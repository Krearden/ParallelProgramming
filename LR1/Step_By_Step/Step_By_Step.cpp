#include <iostream>
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

	return h / 3 * (f_x0 + 2 * sum_even + 4 * sum_uneven);
}


int main()
{
	//STEP BY STEP METHOD
	int N = 100000;
	double a = 4;
	double b = 8;
	double error, ethalone = 2.86376645469595;

	//201 итерация
	for (N = 100000; N <= 2100000; N += 10000)
	{
		clock_t t;
		t = clock();
		double result_stepbystep = simpsonMethod(a, b, N);
		t = clock() - t;
		error = fabs(result_stepbystep - ethalone) / ethalone;
		printf("\n%d %f %.14f %e", N, (float)t / CLOCKS_PER_SEC, result_stepbystep, error);
	}


	return 0;
}