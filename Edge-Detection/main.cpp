#include <iostream>
#include <stdlib.h>
#include "BitmapRawConverter.h"

#include "tbb/task_group.h"
#include <tbb/tick_count.h>

using namespace std;
using namespace tbb;

#define __ARG_NUM__				6
#define THRESHOLD				128

#define NEIGHBOURHOOD_SIZE		5
#define CUTOFF					20

// Prewitt operators 3
//#define FILTER_SIZE				3
//int filterHor[FILTER_SIZE * FILTER_SIZE] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
//int filterVer[FILTER_SIZE * FILTER_SIZE] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };

// Prewitt operators 5
#define FILTER_SIZE				5
int filterHor[FILTER_SIZE * FILTER_SIZE] = { -1, -1, 0, 1, 1, -1, -1, 0, 1, 1, -1, -1, 0, 1, 1, -1, -1, 0, 1, 1 , -1, -1, 0, 1, 1 };
int filterVer[FILTER_SIZE * FILTER_SIZE] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

// Prewitt operators 5 [test]
//#define FILTER_SIZE				5
//int filterHor[FILTER_SIZE * FILTER_SIZE] = { 9, 9, 9, 9, 9, 9, 5, 5, 5, 9, -7, -3, 0, -3, -7, -7, -3, -3, -3, -7 , -7, -7, -7, -7, -7 };
//int filterVer[FILTER_SIZE * FILTER_SIZE] = { 9, 9, -7, -7, -7, 9, 5, -3, -3, -7, 9, 5, 0, -3, -7, 9, 5, -3, -3, -7, 9, 9, -7, -7, -7 };

// Prewitt operators 7
//#define FILTER_SIZE				7
//int filterHor[FILTER_SIZE * FILTER_SIZE] = { -1, -1, -1, 0, 1, 1, 1, -1, -1, -1, 0, 1, 1, 1, -1, -1, -1, 0, 1, 1, 1, -1, -1, -1, 0, 1, 1, 1, -1, -1, -1, 0, 1, 1, 1, -1, -1, -1, 0, 1, 1, 1, -1, -1, -1, 0, 1, 1, 1 };
//int filterVer[FILTER_SIZE * FILTER_SIZE] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1, 1, 1, 1, 1, 1, 1 };

#define SKIP_PREWITT			FILTER_SIZE / 2 + 1
#define SKIP_NEIGHBOURS			NEIGHBOURHOOD_SIZE / 2 + 1

unsigned int totalWidth, totalHeight;

/**
* @brief Scales value to 0 and 1
* 
* @param value value of image pixel
* 
* @return scaled value
*/
int scale(int value) {
	if (value <= THRESHOLD) {
		return 0;
	}
	return 1;
}

/**
* @brief Prewitt operator on input image single pixel
* 
* @param inBuffer buffer of input image
* @param x horizontal coordinate of pixel
* @param y vertical coordinate of pixel
* @param width image width
* 
* @return scaled value of operator result
*/
int filter(int* inBuffer, int x, int y) {

	int save = SKIP_PREWITT - 1;
	int G = 0, Gx = 0, Gy = 0, raw;
	for (int n = 0; n < FILTER_SIZE; n++) {
		for (int m = 0; m < FILTER_SIZE; m++) {
			raw = inBuffer[(x - save + m) + (y - save + n) * totalWidth];
			Gx += raw * filterHor[m + n * FILTER_SIZE];
			Gy += raw * filterVer[m + n * FILTER_SIZE];
		}
	}
	G = sqrt(Gx * Gx + Gy * Gy);
	return 255 * scale(G);
}


/**
* @brief Neighbourhood check around input image pixel
* 
* @param inBuffer buffer of input image
* @param x horizontal coordinate of pixel
* @param y vertical coordinate of pixel
* @param width image width
* 
* @return scaled value of neighbourhood result
*/
int checkNeighbours(int* outBuffer, int x, int y) {
	int P = 0, O = 1;
	int value;
	for (int i = 0; i < NEIGHBOURHOOD_SIZE; i++) {
		for (int j = 0; j < NEIGHBOURHOOD_SIZE; j++) {
			value = scale(outBuffer[(x - i + SKIP_NEIGHBOURS) + (y - j + SKIP_NEIGHBOURS) * totalWidth]);
			if (value == 1) P = 1;
			else O = 0;
		}
	}
	return 255 * abs(P - O);
}

/**
* @brief Serial version of edge detection algorithm implementation using Prewitt operator
* 
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_serial_prewitt(int* inBuffer, int* outBuffer, int width, int height, int col=0, int row=0)  //TODO obrisati
{
	int lowerX, lowerY, upperX, upperY;

	lowerX = (col < SKIP_PREWITT) ? SKIP_PREWITT : col;
	lowerY = (row < SKIP_PREWITT) ? SKIP_PREWITT : row;
	upperX = (col + width > totalWidth - (SKIP_PREWITT)) ? totalWidth - (SKIP_PREWITT) : col + width;
	upperY = (row + height > totalHeight - (SKIP_PREWITT)) ? totalHeight - (SKIP_PREWITT) : row + height;
	

	for (int x = lowerX; x < upperX; x++) {
		for (int y = lowerY; y < upperY; y++) {
			outBuffer[x + y * totalWidth] = filter(inBuffer, x, y);
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
void filter_parallel_prewitt(int* inBuffer, int* outBuffer, int width, int height, int col=0, int row=0)
{
	if (min(height, width) <= CUTOFF) {
		filter_serial_prewitt(inBuffer, outBuffer, width, height, col, row);
	}

	else {
		task_group t;
		int taskHeight = height / 2;
		int restHeight = height - taskHeight;
		int taskWidth = width / 2;
		int restWidth = width - taskWidth;
		t.run([&] {filter_parallel_prewitt(inBuffer, outBuffer, taskWidth, taskHeight, col, row); });
		t.run([&] {filter_parallel_prewitt(inBuffer, outBuffer, restWidth, taskHeight, col+taskWidth, row); });
		t.run([&] {filter_parallel_prewitt(inBuffer, outBuffer, taskWidth, restHeight, col, row+taskHeight); });
		t.run([&] {filter_parallel_prewitt(inBuffer, outBuffer, restWidth, restHeight, col+taskWidth, row+taskHeight); });
		t.wait();
	}

}

/**
* @brief Serial version of edge detection algorithm
* 
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_serial_edge_detection(int* inBuffer, int* outBuffer, int width, int height, int col=0, int row=0)	//TODO obrisati
{
	int stepHor = SKIP_NEIGHBOURS;

	int lowerX, lowerY, upperX, upperY;

	lowerX = (col < SKIP_NEIGHBOURS) ? SKIP_NEIGHBOURS : col;
	lowerY = (row < SKIP_NEIGHBOURS) ? SKIP_NEIGHBOURS : row;
	upperX = (col + width > totalWidth - (SKIP_NEIGHBOURS)) ? totalWidth - (SKIP_NEIGHBOURS) : col + width;
	upperY = (row + height > totalHeight - (SKIP_NEIGHBOURS)) ? totalHeight - (SKIP_NEIGHBOURS) : row + height;



	for (int x = lowerX; x < upperX; x++) {
		for (int y = lowerY; y < upperY; y++) {
			outBuffer[x + y * totalWidth] = checkNeighbours(inBuffer, x, y);
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
void filter_parallel_edge_detection(int* inBuffer, int* outBuffer, int width, int height, int col = 0, int row = 0)	//TODO obrisati
{
	if (min(height, width) <= CUTOFF) {
		filter_serial_edge_detection(inBuffer, outBuffer, width, height, col, row);
	}

	else {
		task_group t;
		int taskHeight = height / 2;
		int restHeight = height - taskHeight;
		int taskWidth = width / 2;
		int restWidth = width - taskWidth;
		t.run([&] {filter_parallel_edge_detection(inBuffer, outBuffer, taskWidth, taskHeight, col, row); });
		t.run([&] {filter_parallel_edge_detection(inBuffer, outBuffer, restWidth, taskHeight, col + taskWidth, row); });
		t.run([&] {filter_parallel_edge_detection(inBuffer, outBuffer, taskWidth, restHeight, col, row + taskHeight); });
		t.run([&] {filter_parallel_edge_detection(inBuffer, outBuffer, restWidth, restHeight, col + taskWidth, row + taskHeight); });
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
	tick_count startTime = tick_count::now();

	switch (testNr)
	{
	case 1:
		cout << "Running serial version of edge detection using Prewitt operator" << endl;
		filter_serial_prewitt(ioFile->getBuffer(), outBuffer, width, height);
		break;
	case 2:
		cout << "Running parallel version of edge detection using Prewitt operator" << endl;
		filter_parallel_prewitt(ioFile->getBuffer(), outBuffer, width, height);
		break;
	case 3:
		cout << "Running serial version of edge detection" << endl;
		filter_serial_edge_detection(ioFile->getBuffer(), outBuffer, width, height);
		break;
	case 4:
		cout << "Running parallel version of edge detection" << endl;
		filter_parallel_edge_detection(ioFile->getBuffer(), outBuffer, width, height);
		break;
	default:
		cout << "ERROR: invalid test case, must be 1, 2, 3 or 4!";
		break;
	}

	// TODO: end measure and display time
	tick_count endTime = tick_count::now();
	cout << "Elapsed time: " << (endTime - startTime).seconds() << " seconds\n\n";

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

	
	totalWidth = inputFile.getWidth();
	totalHeight = inputFile.getHeight();

	int* outBufferSerialPrewitt = new int[totalWidth * totalHeight];
	int* outBufferParallelPrewitt = new int[totalWidth * totalHeight];

	memset(outBufferSerialPrewitt, 0x0, totalWidth * totalHeight * sizeof(int));
	memset(outBufferParallelPrewitt, 0x0, totalWidth * totalHeight * sizeof(int));

	int* outBufferSerialEdge = new int[totalWidth * totalHeight];
	int* outBufferParallelEdge = new int[totalWidth * totalHeight];

	memset(outBufferSerialEdge, 0x0, totalWidth * totalHeight * sizeof(int));
	memset(outBufferParallelEdge, 0x0, totalWidth * totalHeight * sizeof(int));


	// serial version Prewitt
	run_test_nr(1, &outputFileSerialPrewitt, argv[2], outBufferSerialPrewitt, totalWidth, totalHeight);

	// parallel version Prewitt
	run_test_nr(2, &outputFileParallelPrewitt, argv[3], outBufferParallelPrewitt, totalWidth, totalHeight);

	// serial version special
	run_test_nr(3, &outputFileSerialEdge, argv[4], outBufferSerialEdge, totalWidth, totalHeight);

	// parallel version special
	run_test_nr(4, &outputFileParallelEdge, argv[5], outBufferParallelEdge, totalWidth, totalHeight);


	// verification
	int test;
	cout << "Verification: ";
	test = memcmp(outBufferSerialPrewitt, outBufferParallelPrewitt, totalWidth * totalHeight * sizeof(int));

	if (test != 0)
	{
		cout << "Prewitt FAIL!" << endl;
	}
	else
	{
		cout << "Prewitt PASS." << endl;
	}

	test = memcmp(outBufferSerialEdge, outBufferParallelEdge, totalWidth * totalHeight * sizeof(int));

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