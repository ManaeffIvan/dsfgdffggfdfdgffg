#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <omp.h>


using namespace std;

int main(int argc, char* argv[]) {
	if (argc != 4) {
		printf("There should be 3 arguments here!");
		return 0;
	}

	int num_threads;
	try {
		num_threads = stoi(string(argv[1]));
	}
	catch (const exception& ex) {
		printf("Num Threads must be int and >= -1 !");
		return 0;
	}
	if (num_threads < -1) {
		printf("Num Threads must be >= -1 !");
		return 0;
	} else if (num_threads == 0) {
		num_threads = omp_get_max_threads();
	}	

	ifstream in;
	int width = -1, height = -1, shade_Max = -1, sheet_Size;
	uint8_t* sheet;
	try {
		in = std::ifstream(argv[2], std::ios_base::binary);
		try {
			string input_format;
			in >> input_format >> width >> height >> shade_Max;
			if (input_format != "P5" || shade_Max != 255 || width <= 0 || height <= 0) {
				printf("Format file %s is not PNM!", argv[2]);
				return 0;
			}
			sheet_Size = width * height;
			sheet = new uint8_t[sheet_Size];
			in.read(reinterpret_cast<char*>(sheet), sheet_Size);
		}
		catch (const exception& ex) {
			printf("Reading error from file %s!", argv[2]);
			return 0;
		}
		in.close();
	}
	catch (const exception& ex) {
		printf("Can't open file %s!", argv[2]);
		return 0;
	}


	double startT = omp_get_wtime();
	int shade_Min = 255;
	shade_Max = 0;
	int shade[256]{ 0 };
	int count_Pref[257]{ 0 };
	int shade_Pref[257]{ 0 };
	int F0 = -1, F1 = -1, F2 = -1;
double max_D = -1;
	

#pragma omp parallel num_threads(num_threads + (num_threads == -1? 2 : 0)) if (num_threads != -1)
	{
#pragma omp for schedule(static, 1)
		for (int i = 0; i < sheet_Size; i++) {
			shade[sheet[i]]++;
			shade_Min = min(shade_Min, (int)sheet[i]);
			shade_Max = max(shade_Max, (int)sheet[i]);
		}
#pragma omp barrier
#pragma omp single
		for (int i = 1; i < 256; i++) {
			count_Pref[i] = count_Pref[i - 1] + shade[i - 1];
			shade_Pref[i] = shade_Pref[i - 1] + (i - 1) * shade[i - 1];
		}
		int ans_f0 = shade_Min, ans_f1 = shade_Min + 1, ans_f2 = shade_Min + 2;
		double ans_max_D = 0;
#pragma omp for schedule(dynamic)
		for (int f0 = shade_Min; f0 <= shade_Max - 2; f0++) {
			if (shade_Pref[f0] != shade_Pref[f0 + 1]) {
				for (int f1 = f0 + 1; f1 <= shade_Max - 1; f1++) {
					if (shade[f1] != 0) {
						for (int f2 = f1 + 1; f2 <= shade_Max; f2++) {
							if (shade[f2] != 0) {
								double d = ((double)(shade_Pref[f0 + 1] - shade_Pref[shade_Min])) / (count_Pref[f0 + 1] - count_Pref[shade_Min]) * ((double)(shade_Pref[f0 + 1] - shade_Pref[shade_Min]) / sheet_Size) +
									((double)(shade_Pref[f1 + 1] - shade_Pref[f0 + 1])) / (count_Pref[f1 + 1] - count_Pref[f0 + 1]) * ((double)(shade_Pref[f1 + 1] - shade_Pref[f0 + 1]) / sheet_Size) +
									((double)(shade_Pref[f2 + 1] - shade_Pref[f1 + 1])) / (count_Pref[f2 + 1] - count_Pref[f1 + 1]) * ((double)(shade_Pref[f2 + 1] - shade_Pref[f1 + 1]) / sheet_Size) +
									((double)(shade_Pref[shade_Max] - shade_Pref[f2 + 1])) / (count_Pref[shade_Max] - count_Pref[f2 + 1]) * ((double)(shade_Pref[shade_Max] - shade_Pref[f2 + 1]) / sheet_Size);
								if (d >= ans_max_D) {
									ans_max_D = d;
									ans_f0 = f0;
									ans_f1 = f1;
									ans_f2 = f2;
								}
							}
						}
					}
				}
			}
		}
#pragma omp critical
		{
			if (ans_max_D >= max_D) {
				max_D = ans_max_D;
				F0 = ans_f0;
				F1 = ans_f1;
				F2 = ans_f2;
			}
		}
#pragma omp barrier
#pragma omp for schedule(static)
		for (int i = 0; i < sheet_Size; ++i) {
			if (sheet[i] <= F0) {
				sheet[i] = 0;
			}
			else if (sheet[i] <= F1) {
				sheet[i] = 84;
			}
			else if (sheet[i] <= F2) {
				sheet[i] = 170;
			}
			else {
				sheet[i] = 255;
			}
		}
	}

	double endT = omp_get_wtime();
	printf("%u %u %u\nTime (%i thread(s)): %g ms\n", F0, F1, F2, num_threads, (endT - startT) * 1000);
	ofstream out;
	try {
		out = std::ofstream(argv[3], std::ios_base::binary);
		try {
			out << "P5\n" << width << " " << height << "\n255\n";;
			out.write(reinterpret_cast<char*>(sheet), sheet_Size);
		}
		catch (const exception& ex) {
			printf("Writing error from file %s!", argv[3]);
			return 0;
		}
		out.close();
	}
	catch(const exception& ex) {
		printf("Can't open file %s!", argv[3]);
		return 0;
	}
}

