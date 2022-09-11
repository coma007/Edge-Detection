# Edge-Detection
Parallelization of image edge detection using Prewitt operator and neighbourhood analysis. (Project for course Parallel programing)

## Problem analysis
Digital image processing is based on the two-dimensional discrete convolution. With given two functions, mathematical convolution produces a third function, which describes how the behavior of the first function changes due to second function. This operation is based on a kernel (a small dimensions square matrix) that slides over all submatrices of input image (submatrices have kernel's dimensions) and performs arithmetic operations on the elements of the submatrix, placing the results in the output matrix. Image matrix elements contain color values of individual points (pixels) of image.  

The two algorithms that will be used for parallelization are explained bellow.

## Algorithms

### Algorithm with Prewitt operator
In image processing, the Prewitt operator is a kernel, or filter, that approximates the gradient. A gradient represents a directional change in color intensity between pixels of an image.   
This filter multiplies the two corresponding elements of two matrices and summarizes the result, which is then placed in the output matrix. The following formula describes the filtering of the input image contained in the input matrix X, by the kernel filter F with dimensions f×f, with resulting output matrix Y:  

![PrewittFormula](https://user-images.githubusercontent.com/76025555/189482763-4f7f0b8b-2d7a-40f9-84f2-d34202612546.gif)  

The input image is filtered in two both horizontal and vertical direction. Therefore, operator is applied in one direction, but in the other direction, the inverted operator is applied. This results in two vector components, horizontal and vertical. The final result of submatrix filtering is the value of the vector sum of the horizontal and vertical filtering components.  
Examples of operators in horizontal and vertical processing, respectively:  

![PrewittGx](https://user-images.githubusercontent.com/76025555/189482976-79e1ae1b-bfe0-4e0c-bd91-a862ffb5606c.gif)
   ![PrewittGy](https://user-images.githubusercontent.com/76025555/189483013-d9e6510b-e5c9-493d-9d38-c92240d7813b.gif)

### Algorithm with neighbourhood analysis
The neighbourhood of a pixel represents the neighbouring points of the pixel: left, right, up, down and diagonally in all directions.  
Firstly, gray color is removed by scaling the values of the input matrix to the values 0 and 1, relative to a threshold of 128). This results in b&w image with no higlighted edges.  
The neighbourhood of each pixel is then processed as follows:
* _P(i,j) = 1_ if there is a point with value 1 in the neighbourhood of the point
* _P(i,j) = 0_ if there is a no point with value 1 in the neighbourhood of the point
* _O(i,j) = 0_ if there is a point with value 0 in the neighbourhood of the point
* _O(i,j) = 1_ if there is a no point with value 0 in the neighbourhood of the point  

The value that is stored in output matrix is | _P(i,j) - O(i,j)_ | returned in the range 0-255.

## Requirements

**Language**: C++  
**Libraries used:** TBB, EasyBMP  
**Image format**: BMP with RGB  
**Command line arguments**: _input file path, serial output file path, parallel output file path, submatrix size_


## Test examples and results

Testing was performed on Intel i7-1065G7 quad core processor (with eight logical cores and base frequency of 1.50 GHz) and Windows 10 Education OS.

Program was tested on four different input images. Parameters that were varied: value of filter (_filterHor_ i _filterVer_), _FILTER_SIZE_, _NEIGHBOURHOOD_SIZE_ and trashold for parallelisation (_CUTOFF_). Elapsed time was captured.

The following examples show the best results of testing, with various parameters values.

### Architecture
**Image size**: 3888×2592  
**Prewitt operator size**: 5  
**Neighbourhood size**: 5  

Image does not have many details, so it is noticeable that both algorithms perform great job. While testing, it seemed that with the larger submatrix size many redudant details are shown.  

![arch](https://user-images.githubusercontent.com/76025555/189537621-343f766b-6bd5-4f93-be49-77ee7c5bad49.png)

### Kamenko
**Image size**: 736×1279  
**Prewitt operator size**: 3  
**Neighbourhood size**: 9  

This image is animated and its size is smaller, therefore edge detection is not a difficult job. Testing showed that all operator sizes perform well, but only smaller neighbourhood sizes detect acceptable edges (bigger the neighbourhood, thicker the edge).  

![kmnk](https://user-images.githubusercontent.com/76025555/189537749-123ecf32-2ce1-4d27-bc14-7f4e2591ad68.png)

### Dandelion
**Image size**: 1765×1413  
**Prewitt operator size**: 7  
**Neighbourhood size**: 5

This image has many details, and again, algorithm with Prewitt operator found more details, maybe too much, but the other algorithm has found essential details.   

![dand](https://user-images.githubusercontent.com/76025555/189537847-c9311ec2-5be6-4e36-8538-d9549806de01.png)

### Di Caprio
**Image size**: 959×1200  
**Prewitt operator size**: 5  
**Neighbourhood size**: 8

With larger submatrix size, output image shows much more than edges, which is not optimal.

![dcap](https://user-images.githubusercontent.com/76025555/189537963-f5b03904-8f2a-4b84-892f-5f237330c317.png)

## Comparison and conclusion

![coff](https://user-images.githubusercontent.com/76025555/189537096-97078594-aaec-4d8a-83a2-d9a27b3f6279.png)

![pWn](https://user-images.githubusercontent.com/76025555/189537087-bc52455a-08b0-48f8-af25-0d8ae8eae67a.png)

![pWs](https://user-images.githubusercontent.com/76025555/189537003-278636cc-27c1-4db2-8422-0729abe967c8.png)


As it can be seen, parallelization effectiveness increases with image size, but does not progress infinitely.  
Significant acceleration (up to 10 times faster) has happened with more smaller parallel tasks only, even with a small trashold.

These results were expected. But, there is still room for improvement !

