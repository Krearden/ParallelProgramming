// LR3_Step_by_step.cpp : Запорожченко Кирилл (ФЗ-11). вар. 19.
//

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>


using f_function = double(double x);
using namespace std;


//Значение функции
double f(double x)
{
	if (x >= 0 && x <= 0.3)
	{
		return -20 * x + 6;
	}
	else if (x >= 0.3 && x <= 0.5)
	{
		return 0;
	}
	else
	{
		return 32 * x - 16;
	}
}

//Значение функции на левом конце отрезка (x = 0)
double f_left(double x)
{
	return f(0);
}

//Значение функции на правом конце отрезка (x = 1)
double f_right(double x)
{
	return f(1);
}

//Заполняет вектор начальных значений, возвращает шаг по пространству
double getInitValues(f_function f, int N, double interval, vector<double>& result)
{
	result = vector<double>(N, 0);
	double step = interval / (N - 1);
	for (int i = 0; i < N; i++)
	{
		result[i] = f(step * i);
	}
	
	return step;
}


int main()
{
	string output_filename = "output.txt";
	//кол-во точек в пространственной области
	int N = 10; 
	//кол-во моментов времени, на котор. делится интервал от 0 до T
	int T = 100;
	vector<double> init;
	vector<double> left;
	vector<double> right;
	//заполняем вектор init значениями начального распределения функции u(x, 0)
	double step_in_space = getInitValues(f, N, 1, init);
	//заполняем вектора left & right - краевые условия  
	double step_in_time = getInitValues(f_left, T, 0.4, left);
	getInitValues(f_right, T, 0.4, right);

}
