#include <iostream>
#include <stdlib.h>
#include "BitmapRawConverter.h"
#include "tbb/task_group.h"

using namespace std;
using namespace tbb;

#define __ARG_NUM__				6
#define THRESHOLD				128
#define NEIGHBOURHOOD			5
#define GRAIN_ROWS				100

// Prewitt operators 7
//#define FILTER_SIZE				7
//int filterHor[FILTER_SIZE * FILTER_SIZE] = { -1, -1, -1, 0, 1, 1, 1, -1, -1, -1, 0, 1, 1, 1, -1, -1, -1, 0, 1, 1, 1, -1, -1, -1, 0, 1, 1, 1, -1, -1, -1, 0, 1, 1, 1, -1, -1, -1, 0, 1, 1, 1, -1, -1, -1, 0, 1, 1, 1 };
//int filterVer[FILTER_SIZE * FILTER_SIZE] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1, 1, 1, 1, 1, 1, 1 };

// Prewitt operators 5
//#define FILTER_SIZE				5
//int filterHor[FILTER_SIZE * FILTER_SIZE] = { -1, -1, 0, 1, 1, -1, -1, 0, 1, 1, -1, -1, 0, 1, 1, -1, -1, 0, 1, 1 , -1, -1, 0, 1, 1 };
//int filterVer[FILTER_SIZE * FILTER_SIZE] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

// Prewitt operators 5
#define FILTER_SIZE				5
int filterHor[FILTER_SIZE * FILTER_SIZE] = { 9, 9, 9, 9, 9, 9, 5, 5, 5, 9, -7, -3, 0, -3, -7, -7, -3, -3, -3, -7 , -7, -7, -7, -7, -7 };
int filterVer[FILTER_SIZE * FILTER_SIZE] = { 9, 9, -7, -7, -7, 9, 5, -3, -3, -7, 9, 5, 0, -3, -7, 9, 5, -3, -3, -7, 9, 9, -7, -7, -7 };

// Prewitt operators 3
//#define FILTER_SIZE				3
//int filterHor[FILTER_SIZE * FILTER_SIZE] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
//int filterVer[FILTER_SIZE * FILTER_SIZE] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};


int scale(int value) {
	if (value <= THRESHOLD) {
		return 0;
	}
	return 1;
}

int filter(int* inBuffer, int x, int y, int width) {

	int save = FILTER_SIZE / 2;
	int G = 0, Gx = 0, Gy = 0, raw;
	for (int n = 0; n < FILTER_SIZE; n++) {
		for (int m = 0; m < FILTER_SIZE; m++) {
			raw = inBuffer[(x - save + m) + (y - save + n) * width];
			Gx += raw * filterHor[m + n * FILTER_SIZE];
			Gy += raw * filterVer[m + n * FILTER_SIZE];
		}
	}
	G = sqrt(Gx * Gx + Gy * Gy);
	return 255 * scale(G);
}

int checkNeighbours(int* outBuffer, int x, int y, int width) {
	int P = 0, O = 1;
	int step = NEIGHBOURHOOD / 2 + 1;
	int value;
	for (int i = 0; i < NEIGHBOURHOOD; i++) {
		for (int j = 0; j < NEIGHBOURHOOD; j++) {
			value = scale(outBuffer[(x - i + step) + (y - j + step) * width]);
			if (value == 1) P = 1;
			else O = 0;
		}
	}
	return 255 * abs(P - O);
}

/**
* @brief Serial version of edge detection algorithm implementation using Prewitt operator
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_serial_prewitt(int* inBuffer, int* outBuffer, int width, int height, int stepVer = FILTER_SIZE / 2 + 1)  //TODO obrisati
{
	int stepHor = FILTER_SIZE / 2 + 1;
	for (int x = stepHor; x < width - stepHor; x++) {
		for (int y = stepVer; y < height - stepVer; y++) {
			outBuffer[x + y * width] = filter(inBuffer, x, y, width);
		}
	}
}


/**
* @brief Parallel version of edge detection algorithm implementation using Prewitt operator
*
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_parallel_prewitt(int* inBuffer, int* outBuffer, int width, int height)
{
	if (height <= GRAIN_ROWS) {
		filter_serial_prewitt(inBuffer, outBuffer, width, height, 0);
	}

	else {
		task_group t;
		int taskHeight = height / 4;
		int restHeight = height - 3 * taskHeight;
		t.run([&] {filter_parallel_prewitt(inBuffer + width * (0 * taskHeight), outBuffer + width * (0 * taskHeight), width, taskHeight); });
		t.run([&] {filter_parallel_prewitt(inBuffer + width * (1 * taskHeight), outBuffer + width * (1 * taskHeight), width, taskHeight); });
		t.run([&] {filter_parallel_prewitt(inBuffer + width * (2 * taskHeight), outBuffer + width * (2 * taskHeight), width, taskHeight); });
		t.run([&] {filter_parallel_prewitt(inBuffer + width * (3 * taskHeight), outBuffer + width * (3 * taskHeight), width, restHeight); });
		t.wait();
	}

}

/**
* @brief Serial version of edge detection algorithm
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_serial_edge_detection(int* inBuffer, int* outBuffer, int width, int height, int stepVer = NEIGHBOURHOOD / 2 + 1)	//TODO obrisati
{
	int stepHor = NEIGHBOURHOOD / 2 + 1;
	for (int x = stepHor; x < width - stepHor; x++) {
		for (int y = stepVer; y < height - stepVer; y++) {
			outBuffer[x + y * width] = checkNeighbours(inBuffer, x, y, width);
		}
	}
}

/**
* @brief Parallel version of edge detection algorithm
*
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_parallel_edge_detection(int* inBuffer, int* outBuffer, int width, int height)
{
	if (height <= GRAIN_ROWS) {
		filter_serial_edge_detection(inBuffer, outBuffer, width, height, 0);
	}

	else {
		task_group t;
		int taskHeight = height / 4;
		int restHeight = height - 3 * taskHeight;
		t.run([&] {filter_parallel_edge_detection(inBuffer + width * (0 * taskHeight), outBuffer + width * (0 * taskHeight), width, taskHeight); });
		t.run([&] {filter_parallel_edge_detection(inBuffer + width * (1 * taskHeight), outBuffer + width * (1 * taskHeight), width, taskHeight); });
		t.run([&] {filter_parallel_edge_detection(inBuffer + width * (2 * taskHeight), outBuffer + width * (2 * taskHeight), width, taskHeight); });
		t.run([&] {filter_parallel_edge_detection(inBuffer + width * (3 * taskHeight), outBuffer + width * (3 * taskHeight), width, restHeight); });
		t.wait();
	}

}

/**
* @brief Function for running test.
*
* @param testNr test identification, 1: for serial version, 2: for parallel version
* @param ioFile input/output file, firstly it's holding buffer from input image and than to hold filtered data
* @param outFileName output file name
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/


void run_test_nr(int testNr, BitmapRawConverter* ioFile, char* outFileName, int* outBuffer, unsigned int width, unsigned int height)
{

	// TODO: start measure
	int stepPrewitt = FILTER_SIZE / 2 + 1;
	int stepEdge = NEIGHBOURHOOD / 2 + 1;

	switch (testNr)
	{
	case 1:
		cout << "Running serial version of edge detection using Prewitt operator" << endl;
		filter_serial_prewitt(ioFile->getBuffer(), outBuffer, width, height);
		break;
	case 2:
		cout << "Running parallel version of edge detection using Prewitt operator" << endl;
		filter_parallel_prewitt(ioFile->getBuffer() + width * stepPrewitt, outBuffer + width * stepPrewitt, width, height - 2*stepPrewitt);
		break;
	case 3:
		cout << "Running serial version of edge detection" << endl;
		filter_serial_edge_detection(ioFile->getBuffer(), outBuffer, width, height);
		break;
	case 4:
		cout << "Running parallel version of edge detection" << endl;
		filter_parallel_edge_detection(ioFile->getBuffer() + width * stepEdge, outBuffer + width * stepEdge, width, height - 2*stepEdge);
		break;
	default:
		cout << "ERROR: invalid test case, must be 1, 2, 3 or 4!";
		break;
	}
	// TODO: end measure and display time

	ioFile->setBuffer(outBuffer);
	ioFile->pixelsToBitmap(outFileName);
}

/**
* @brief Print program usage.
*/
void usage()
{
	cout << "\n\ERROR: call program like: " << endl << endl;
	cout << "ProjekatPP.exe";
	cout << " input.bmp";
	cout << " outputSerialPrewitt.bmp";
	cout << " outputParallelPrewitt.bmp";
	cout << " outputSerialEdge.bmp";
	cout << " outputParallelEdge.bmp" << endl << endl;
}

int main(int argc, char* argv[])
{

	if (argc != __ARG_NUM__)
	{
		usage();
		return 0;
	}

	BitmapRawConverter inputFile(argv[1]);
	BitmapRawConverter outputFileSerialPrewitt(argv[1]);
	BitmapRawConverter outputFileParallelPrewitt(argv[1]);
	BitmapRawConverter outputFileSerialEdge(argv[1]);
	BitmapRawConverter outputFileParallelEdge(argv[1]);

	unsigned int width, height;

	int test;

	width = inputFile.getWidth();
	height = inputFile.getHeight();

	int* outBufferSerialPrewitt = new int[width * height];
	int* outBufferParallelPrewitt = new int[width * height];

	memset(outBufferSerialPrewitt, 0x0, width * height * sizeof(int));
	memset(outBufferParallelPrewitt, 0x0, width * height * sizeof(int));

	int* outBufferSerialEdge = new int[width * height];
	int* outBufferParallelEdge = new int[width * height];

	memset(outBufferSerialEdge, 0x0, width * height * sizeof(int));
	memset(outBufferParallelEdge, 0x0, width * height * sizeof(int));

	// serial version Prewitt
	run_test_nr(1, &outputFileSerialPrewitt, argv[2], outBufferSerialPrewitt, width, height);

	// parallel version Prewitt
	run_test_nr(2, &outputFileParallelPrewitt, argv[3], outBufferParallelPrewitt, width, height);

	// serial version special
	run_test_nr(3, &outputFileSerialEdge, argv[4], outBufferSerialEdge, width, height);

	// parallel version special
	run_test_nr(4, &outputFileParallelEdge, argv[5], outBufferParallelEdge, width, height);



	/*for (int i = 0; i <= width*height; i++) {
		if (outputFileSerialEdge.getBuffer()[i] - outputFileParallelEdge.getBuffer()[i] != 0) {
			cout << i / width << endl;
		}
	}*/

	// verification
	cout << "Verification: ";
	test = memcmp(outBufferSerialPrewitt, outBufferParallelPrewitt, width * height * sizeof(int));

	if (test != 0)
	{
		cout << "Prewitt FAIL!" << endl;
	}
	else
	{
		cout << "Prewitt PASS." << endl;
	}

	test = memcmp(outBufferSerialEdge, outBufferParallelEdge, width * height * sizeof(int));

	if (test != 0)
	{
		cout << "Edge detection FAIL!" << endl;
	}
	else
	{
		cout << "Edge detection PASS." << endl;
	}

	// clean up
	delete outBufferSerialPrewitt;
	delete outBufferParallelPrewitt;

	delete outBufferSerialEdge;
	delete outBufferParallelEdge;

	return 0;
}