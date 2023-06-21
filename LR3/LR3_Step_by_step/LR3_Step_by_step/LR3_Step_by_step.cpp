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

//Значение функции на левом конце отрезка (x = 0) для краевого условия 𝑢(0, 𝑡) = 𝑓(0)
double f_left(double x)
{
	return f(0);
}

//Значение функции на правом конце отрезка (x = 1) для краевого условия 𝑢(1, 𝑡) = 𝑓(1)
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

//Явная трехточечная конечно-разностная схема
double L(double previousEl, double currentEl, double nextEl, double step_in_space, double step_in_time)
{
	double q = step_in_time / (step_in_space * step_in_space);
	return q * (previousEl + nextEl) + (1 - 2 * q) * currentEl;
}

//Решение уравнения методом конечных разнсотей
vector<vector<double>> solveEquation(vector<double>& init, vector<double>& left, vector<double>& right, double step_in_space, double step_in_time, int N, int T)
{
	//инициализируем двумерный вектор, представляющий сетку значений функции u(x, t)
	vector<vector<double>> result_grid = vector<vector<double>>(T, vector<double>(N, 0));
	
	//начальные условия
	for (int j = 0; j < N; j++) 
	{
		result_grid[0][j] = init[j]; 
	}
	//граничные условия
	for (int i = 0; i < T; i++) 
	{
		result_grid[i][0] = left[i];  
		result_grid[i][N - 1] = right[i]; 
	}
	
	for (int i = 0; i < T - 1; i++) 
	{
		for (int j = 1; j < N - 1; j++) {
			result_grid[i + 1][j] = L(result_grid[i][j - 1], result_grid[i][j], result_grid[i][j + 1], step_in_space, step_in_time);  // Вычисление нового значения функции
		}
	}

	return result_grid;
}

void writeGridToFile(const string& filename, int N, int T, double step_in_space, double step_in_time, const vector<vector<double>>& result_grid)
{
	ofstream fout(filename);
	fout << 0;
	for (size_t i = 0; i < N; i++) {
		fout << "," << i * step_in_space;
	}
	fout << endl;
	for (size_t i = 0; i < T; i++) {
		fout << i * step_in_time;
		for (size_t j = 0; j < N; j++) {
			fout << "," << result_grid[i][j];
		}
		fout << endl;
	}
}


int main()
{
	//кол-во точек в пространственной области
	int N = 10; 
	//кол-во моментов времени, на котор. делится интервал от 0 до T
	int T = 100;
	string output_filename = "output_N" + to_string(N) + "_T" + to_string(T) + ".txt";
	vector<double> init;
	vector<double> left;
	vector<double> right;
	//заполняем вектор init значениями начального распределения функции u(x, 0)
	double step_in_space = getInitValues(f, N, 1, init);
	//заполняем вектора left & right - краевые условия  
	double step_in_time = getInitValues(f_left, T, 0.4, left);
	getInitValues(f_right, T, 0.4, right);

	vector<vector<double>> result_grid = solveEquation(init, left, right, step_in_space, step_in_time, T, N);

	writeGridToFile(output_filename, N, T, step_in_space, step_in_time, result_grid);
}
