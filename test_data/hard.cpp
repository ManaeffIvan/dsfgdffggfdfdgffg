#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <omp.h>

using namespace std;

int main(int argc, char* argv[]) {
	if (argc != 4) {
		printf("There should be 4 arguments here!");
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

	FILE* fin;
	int width = -1, height = -1, shade_Max = -1, sheet_Size;
	vector<int> sheet;
	try {
		fin = fopen(argv[2], "rb");
		try {
			string input_format;
			fscanf(fin, "%s %d %d %d", input_format, width, height, shade_Max);
			if (input_format != "P5" || shade_Max != 255 || width <= 0 || height <= 0) {
				printf("Format file %s is not PNM!", string(argv[2]));
			}
			sheet_Size = width * height;
			sheet.resize(sheet_Size);
			for (int i = 0; i < sheet_Size; i++) {
				fscanf(fin, "%d", sheet[i]);
			}
		}
		catch (const exception& ex) {
			printf("Reading error from file %s!", string(argv[2]));
			return 0;
		}
		fclose(fin);
	}
	catch (const exception& ex) {
		printf("Can't open file %s!", string(argv[2]));
		return 0;
	}

	
	

	double startT = omp_get_wtime();
	int shade_Min = 300;
	shade_Max = -300;
	int shade[256]{ 0 };
	int count_Pref[257]{ 0 };
	int shade_Pref[257]{ 0 };
	int F0 = -1, F1 = -1, F2 = -1;

#pragma omp parallel num_threads(num_threads + (num_threads == -1? 2 : 0)) if (num_threads != -1)
	{
		double maxD = -1;
#pragma omp for schedule(static, 1)
		for (int i = 0; i < sheet_Size; i++) {
			shade[sheet[i]]++;
			shade_Min = min(shade_Min, sheet[i]);
			shade_Max = max(shade_Max, sheet[i]);
		}
#pragma omp barrier
#pragma omp single
		for (int i = 1; i < 256; i++) {
			count_Pref[i] = count_Pref[i - 1] + shade[i - 1];
			shade_Pref[i] = shade_Pref[i - 1] + (i - 1) * shade[i - 1];
		}
		int ans_f0 = shade_Min, ans_f1 = shade_Min + 1, ans_f2 = shade_Min + 2;
#pragma omp for schedule(dynamic)
		for (int f0 = shade_Min; f0 <= shade_Max - 2; f0++) {
			if (shade[f0] != 0) {
				for (int f1 = f0 + 1; f1 <= shade_Max - 1; f1++) {
					if (shade[f1] != 0) {
						for (int f2 = f1 + 1; f2 <= shade_Max; f2++) {
							if (shade[f2] != 0) {
								double d = ((double)(shade_Pref[f0 + 1] - shade_Pref[shade_Min])) / (count_Pref[f0 + 1] - count_Pref[shade_Min]) * ((double)(shade_Pref[f0 + 1] - shade_Pref[shade_Min]) / sheet_Size) +
									((double)(shade_Pref[f1 + 1] - shade_Pref[f0 + 1])) / (count_Pref[f1 + 1] - count_Pref[f0 + 1]) * ((double)(shade_Pref[f1 + 1] - shade_Pref[f0 + 1]) / sheet_Size) +
									((double)(shade_Pref[f2 + 1] - shade_Pref[f1 + 1])) / (count_Pref[f2 + 1] - count_Pref[f1 + 1]) * ((double)(shade_Pref[f2 + 1] - shade_Pref[f1 + 1]) / sheet_Size) +
									((double)(shade_Pref[shade_Max] - shade_Pref[f2 + 1])) / (count_Pref[shade_Max] - count_Pref[f2 + 1]) * ((double)(shade_Pref[shade_Max] - shade_Pref[f2 + 1]) / sheet_Size);
								if (d >= maxD) {
									maxD = d;
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
#pragma omp barrier
#pragma omp critical
		{
			F0 = ans_f0;
			F1 = ans_f1;
			F2 = ans_f2;
		}
#pragma omp for schedule(static)
		for (int i = 0; i < sheet_Size; ++i) {
			if (shade[i] <= ans_f0) {
				shade[i] = 0;
			}
			else if (shade[i] <= ans_f1) {
				shade[i] = 84;
			}
			else if (shade[i] <= ans_f2) {
				shade[i] = 170;
			}
			else {
				shade[i] = 255;
			}
		}
	}

	double endT = omp_get_wtime();
	printf("%u %u %u\nTime (%i thread(s)): %g ms\n", F0, F1, F2, num_threads, (endT - startT) * 1000);
	FILE* fout;
	try {
		fout = fopen(argv[3], "wb");
		try {
			string input_format;
			fprintf(fout, "P5\n%d  5d\n255\n", width, height);
			for (int i = 0; i < sheet_Size; i++) {
				fscanf(fin, "%d", sheet[i]);
			}
		}
		catch (const exception& ex) {
			printf("Writing error from file %s!", string(argv[3]));
			return 0;
		}
		fclose(fout);
	}
	catch(const exception& ex) {
		printf("Can't open file %s!", string(argv[3]));
		return 0;
	}
}
