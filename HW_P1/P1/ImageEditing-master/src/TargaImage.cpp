///////////////////////////////////////////////////////////////////////////////
//
//      TargaImage.cpp                          Author:     Stephen Chenney
//                                              Modified:   Eric McDaniel
//                                              Date:       Fall 2004
//
//      Implementation of TargaImage methods.  You must implement the image
//  modification functions.
//
///////////////////////////////////////////////////////////////////////////////

#include "Globals.h"
#include "TargaImage.h"
#include "libtarga.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <direct.h>

using namespace std;

const unsigned char BACKGROUND[3]   = { 0, 0, 0 };      // background color
// Computes n choose s, efficiently
double Binomial(int n, int s)
{
    double        res;

    res = 1;
    for (int i = 1 ; i <= s ; i++)
        res = (n - i + 1) * res / i ;

    return res;
}// Binomial


///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage() : width(0), height(0), data(NULL)
{}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h) : width(w), height(h)
{
   data = new unsigned char[width * height * 4];
   ClearToBlack();
}// TargaImage



///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables to values given.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h, unsigned char *d)
{
    int i;

    width = w;
    height = h;
    data = new unsigned char[width * height * 4];

    for (i = 0; i < width * height * 4; i++)
	    data[i] = d[i];
}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Copy Constructor.  Initialize member to that of input
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(const TargaImage& image) 
{
   width = image.width;
   height = image.height;
   data = NULL; 
   if (image.data != NULL) {
      data = new unsigned char[width * height * 4];
      memcpy(data, image.data, sizeof(unsigned char) * width * height * 4);
   }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Destructor.  Free image memory.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::~TargaImage()
{
    if (data)
        delete[] data;
}// ~TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Converts an image to RGB form, and returns the rgb pixel data - 24 
//  bits per pixel. The returned space should be deleted when no longer 
//  required.
//
///////////////////////////////////////////////////////////////////////////////
unsigned char* TargaImage::To_RGB(void)
{
    unsigned char   *rgb = new unsigned char[width * height * 3];
    int		    i, j;

    if (! data)
	    return NULL;

    // Divide out the alpha
    for (i = 0 ; i < height ; i++)
    {
	    int in_offset = i * width * 4;
	    int out_offset = i * width * 3;

	    for (j = 0 ; j < width ; j++)
        {
	        RGBA_To_RGB(data + (in_offset + j*4), rgb + (out_offset + j*3));
	    }
    }

    return rgb;
}// TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Save the image to a targa file. Returns 1 on success, 0 on failure.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Save_Image(const char *filename)
{
    char* CWD;
    CWD = getcwd(NULL, 0);
    static const char* path = "\\..\\Images\\saved\\";
    char* fullPath = (char*)malloc(strlen(CWD) + strlen(path) + strlen(filename) + 1);
    strcpy(fullPath, CWD);
    strcat(fullPath, path);
    strcat(fullPath, filename);
    //cout << fullPath << endl;
    TargaImage	*out_image = Reverse_Rows();

    if (! out_image)
	    return false;

    if (!tga_write_raw(fullPath, width, height, out_image->data, TGA_TRUECOLOR_32))
    {
	    cout << "TGA Save Error: %s\n", tga_error_string(tga_get_last_error());
	    return false;
    }
    free(fullPath);
    free(CWD);
    delete out_image;

    return true;
}// Save_Image
///////////////////////////////////////////////////////////////////////////////
//
//      Load a targa image from a file.  Return a new TargaImage object which 
//  must be deleted by caller.  Return NULL on failure.
//
///////////////////////////////////////////////////////////////////////////////


TargaImage* TargaImage::Load_Image(char *filename)
{

    unsigned char   *temp_data;
    TargaImage	    *temp_image;
    TargaImage	    *result;
    int		        width, height;

    char* CWD;
    CWD = getcwd(NULL, 0);
    static const char* path = "\\..\\Images\\load\\";
    if (!filename)
    {
        cout << "No filename given." << endl;
        return NULL;
    }// if

    char* fullPath = (char*)malloc(strlen(CWD) + strlen(path) + strlen(filename) + 1);
    strcpy(fullPath,CWD);
    strcat(fullPath, path);
    strcat(fullPath, filename);

    //printf("file name %s\n", FilePath);

    temp_data = (unsigned char*)tga_load(fullPath, &width, &height, TGA_TRUECOLOR_32);
    if (!temp_data)
    {
        cout << "TGA Error : %s\n" << tga_error_string(tga_get_last_error()) << endl;
	    width = height = 0;
	    return NULL;
    }
    temp_image = new TargaImage(width, height, temp_data);
    free(temp_data);
    result = temp_image->Reverse_Rows();
    delete temp_image;
    free(fullPath);
    free(CWD);
    return result;
}// Load_Image


void Debugging(char* filename, int line, string messenge) {
    cout << "debugging from " << line << " line of " << filename << " messenge : " << messenge << endl;
}

///////////////////////////////////////////////////////////////////////////////
//
//      Convert image to grayscale.  Red, green, and blue channels should all 
//  contain grayscale value.  Alpha channel shoould be left unchanged.  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::To_Grayscale()
{ 
    int channelOffet = width * height;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int pixelIndex = (i * width + j) * 4;
            float grayValue = RGBAtoGray(pixelIndex);
            data[pixelIndex + RED] = data[pixelIndex + GREEN] = data[pixelIndex + BLUE] = grayValue;
        }
    }
	return true;
}// To_Grayscale


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using uniform quantization.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Uniform()
{
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int pixelIndex = (i * width + j) * 4;
            unsigned char quantizedRed = (data[pixelIndex + RED] >> 5);
            unsigned char quantizedGreen = (data[pixelIndex + GREEN] >> 5);
            unsigned char quantizedBlue = (data[pixelIndex + BLUE] >> 6);
            // for visualization, we should convert back to 8bit
            data[pixelIndex + RED] = quantizedRed << 5;
            data[pixelIndex + GREEN] = quantizedGreen << 5;
            data[pixelIndex + BLUE] = quantizedBlue << 6;
        }
    }
    return true;
}// Quant_Uniform


///////////////////////////////////////////////////////////////////////////////
//
//      Convert the image to an 8 bit image using populosity quantization.  
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Populosity()
{
    auto Flattening = [](unsigned char R, unsigned char G, unsigned char B, int scale) {
        return scale * scale * (int)R + scale * (int)G + (int)B;
    };
    auto BacktoSpace = [](int& r, int& g, int& b,int colorValue) {
        r = (colorValue >> 10); colorValue -= (r << 10); r <<= 3;
        g = (colorValue >> 5); colorValue -= (g << 5); g <<= 3;
        b = colorValue; b <<= 3;
    };
    pair<int, int> Histogram[40000];
    for (int i = 0; i < 40000; i++) Histogram[i] = pair<int, int>(0, i);

    for (int i = 0; i < width * height; i++) {
        int pixelIndex = i * 4;
        unsigned char quantizedRed = (data[pixelIndex + RED] >> 3);
        unsigned char quantizedGreen = (data[pixelIndex + GREEN] >> 3);
        unsigned char quantizedBlue = (data[pixelIndex + BLUE] >> 3);
        Histogram[Flattening(quantizedRed, quantizedGreen, quantizedBlue, 32)].first++;
    }
    sort(Histogram, Histogram + 32768, greater<pair<int,int>>());
    int PopularSize = 0,cnt = 0;
    //for (int i = 0; i < 32768; i++) if (Histogram[i].first != 0) cnt++;
    //Debugging("TargalImage.cpp", 301, "Size : " + to_string(cnt));
    for (; PopularSize < 256; PopularSize++) if (Histogram[PopularSize].first == 0) break;
    for (int i = 0; i < width * height; i++) {
        int pixelIndex = i * 4;
        unsigned char origRed = data[pixelIndex];
        unsigned char origGreen = data[pixelIndex + 1];
        unsigned char origBlue = data[pixelIndex + 2];
        int minIndex = 0, minDis = 2147483640;
        for(int c = 0;c < PopularSize;c++) {
            int Dis = 0,colorValue = Histogram[c].second,r,g,b;
            BacktoSpace(r, g, b, colorValue);
            Dis = (origRed - r) * (origRed - r) + (origGreen - g) * (origGreen - g) + (origBlue - b) * (origBlue - b);
            if (Dis < minDis) {
                minDis = Dis;
                minIndex = c;
            }
        }
        int newR, newG, newB;
        BacktoSpace(newR, newG, newB, Histogram[minIndex].second);
        data[pixelIndex + RED] = newR;
        data[pixelIndex + GREEN] = newG;
        data[pixelIndex + BLUE] = newB;
    }
    return true;
}// Quant_Populosity


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image using a threshold of 1/2.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Threshold()
{
    for (int i = 0; i < width * height; i++) {
        int pixelIndex = i * 4;
        unsigned char grayscale = RGBAtoGray(pixelIndex);
        unsigned char newValue = (grayscale > 128) ? 255 : 0;
        //cout << newValue / 255;
        data[pixelIndex + RED] = data[pixelIndex + GREEN] = data[pixelIndex + BLUE] = newValue;
    }
    return true;
}// Dither_Threshold


///////////////////////////////////////////////////////////////////////////////
//
//      Dither image using random dithering.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Random()
{
    srand((unsigned int)time(0));
    for (int i = 0; i < width * height; i++) {
        int pixelIndex = i * 4;
        unsigned char grayscale = RGBAtoGray(pixelIndex);
        float randomNoise = ((float)rand() / RAND_MAX) * 0.4f - 0.2f;
        unsigned char randomThreshold = (0.5f - randomNoise) * 255.0f;
        unsigned char newValue = (grayscale >= randomThreshold) ? 255 : 0;
        data[pixelIndex + RED] = data[pixelIndex + GREEN] = data[pixelIndex + BLUE] = newValue;
    }
    return true;
}// Dither_Random


///////////////////////////////////////////////////////////////////////////////
//
//      Perform Floyd-Steinberg dithering on the image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_FS()
{
    this->To_Grayscale();
    vector<float> oldData(width * height), newData(width * height);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int pixelIndex = (i * width + j);
            oldData[pixelIndex] = RGBAtoGray(pixelIndex * 4); 
        }
    }
    bool flag = 0;
    for (int i = 0; i < height; i++, flag ^= 1) {
        for (int j = (flag ? (width - 1) : 0); j >= 0 && j < width; j += (flag ? -1 : 1)) {
            int pixelIndex = (i * width + j);
            float oldValue = oldData[pixelIndex];
            float newValue = (oldValue > 128) ? 255 : 0;
            newData[pixelIndex] = newValue;
            float error = oldValue - newValue;
            if (!flag) {
                if (j + 1 < width) {
                    int rightPixelIndex = (i * width + (j + 1));
                    oldData[rightPixelIndex] += (error * 7.0f / 16.0f);
                }
            }
            else {
                if (j - 1 >= 0) {
                    int leftPixelIndex = (i * width + (j - 1));
                    oldData[leftPixelIndex] += (error * 7.0f / 16.0f);
                }
            }
            if (i + 1 < height) { // Bottom neighbor
                int bottomPixelIndex = ((i + 1) * width + j);
                oldData[bottomPixelIndex] += (error * 5.0f / 16.0f);
                if (j + 1 < width) { // Bottom-right neighbor
                    int bottomRightPixelIndex = ((i + 1) * width + (j + 1));
                    oldData[bottomRightPixelIndex] += (error * 1.0f / 16.0f);
                }
            }
            if (i + 1 < height && j > 0) { // Bottom-left neighbor
                int bottomLeftPixelIndex = ((i + 1) * width + (j - 1));
                oldData[bottomLeftPixelIndex] += (error * 3.0f / 16.0f);
            }
        }
    }
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int pixelIndex = (i * width + j);
            data[pixelIndex*4 + RED] = data[pixelIndex*4 + GREEN] = data[pixelIndex*4 + BLUE] = newData[pixelIndex];
        }
    }
    return true;
}// Dither_FS


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image while conserving the average brightness.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Bright()
{
    float avgBrightness = 0.0;
    float* Gray = (float*)malloc(width * height * sizeof(float));
    for (int i = 0; i < width * height; i++) {
        int pixelIndex = i * 4;
        Gray[i] = RGBAtoGray(pixelIndex) / 256.0f;
        avgBrightness += Gray[i];
    }
    avgBrightness /= (width * height);
    //Debugging("Targalimage.cpp", 431, "avg = " + to_string(avgBrightness));
    sort(Gray, Gray + (width * height), greater<float>());
    float Thresh = Gray[(int)(avgBrightness * (width * height))];
    //Debugging("Targalimage.cpp", 431, "Thr = " + to_string(Thresh));
    for (int i = 0; i < width * height; i++) {
        int pixelIndex = i * 4;
        data[pixelIndex + RED] = data[pixelIndex + GREEN] = data[pixelIndex + BLUE] = ((RGBAtoGray(pixelIndex) / 256.0)  > Thresh ? 255 : 0);
    }
    free(Gray);
    return true;
}// Dither_Bright


///////////////////////////////////////////////////////////////////////////////
//
//      Perform clustered differing of the image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Cluster()
{
    const float ditherMatrix[4][4] = {
        {0.7059f, 0.3529f, 0.5882f, 0.2353f},
        {0.0588f, 0.9412f, 0.8235f, 0.4118f},
        {0.4706f, 0.7647f, 0.8824f, 0.1176f},
        {0.1765f, 0.5294f, 0.2941f, 0.6471f}
    };
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

            int pixelIndex = (i * width + j) * 4;
            unsigned char grayScale = RGBAtoGray(pixelIndex);
            unsigned char threshold = ditherMatrix[i % 4][j % 4] * 255.0f;
            unsigned char newValue = (grayScale >= threshold) ? 255 : 0;
            data[pixelIndex + RED] = data[pixelIndex + GREEN] = data[pixelIndex + BLUE] = newValue;
        }
    }
    return true;
}// Dither_Cluster


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using Floyd-Steinberg dithering over
//  a uniform quantization - the same quantization as in Quant_Uniform.
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Color()
{
    ClearToBlack();
    return false;
}// Dither_Color


///////////////////////////////////////////////////////////////////////////////
//
//      Composite the current image over the given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Over(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout <<  "Comp_Over: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Over


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "in" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_In(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_In: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_In


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "out" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Out(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Out: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Out


///////////////////////////////////////////////////////////////////////////////
//
//      Composite current image "atop" given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Atop(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Atop: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Atop


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image with given image using exclusive or (XOR).  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Xor(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Xor: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Xor


///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the difference bewteen this imag and the given one.  Image 
//  dimensions must be equal.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Difference(TargaImage* pImage)
{
    if (!pImage)
        return false;

    if (width != pImage->width || height != pImage->height)
    {
        cout << "Difference: Images not the same size\n";
        return false;
    }// if

    for (int i = 0 ; i < width * height * 4 ; i += 4)
    {
        unsigned char        rgb1[3];
        unsigned char        rgb2[3];

        RGBA_To_RGB(data + i, rgb1);
        RGBA_To_RGB(pImage->data + i, rgb2);

        data[i] = abs(rgb1[0] - rgb2[0]);
        data[i+1] = abs(rgb1[1] - rgb2[1]);
        data[i+2] = abs(rgb1[2] - rgb2[2]);
        data[i+3] = 255;
    }

    return true;
}// Difference

///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 box filter on this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////

enum paddingType {
    ZERO_PADDING = 1,
    MIRROR_PADDING = 2
    //REPEAT_EDGE = 3
};

bool applyFilter(const std::vector<std::vector<float>>& filter, int filterSize, paddingType padding, unsigned char origData[],int width, int height) {
    auto Clamp = [](float Min, float Max, float val) {
        return std::min(Max, std::max(Min, val));
    };
    int halfSize = filterSize / 2;
    int offSet = (filterSize % 2 == 0) ? 1 : 0;
    std::vector<unsigned char> newData(width * height * 4, 0);
    //cout << "start filtering ; size ="<< filter.size()<<" "<<filter[0].size() << endl;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float redSum = 0.0f, greenSum = 0.0f, blueSum = 0.0f;

            for (int dy = -halfSize; dy <= halfSize - offSet; dy++) {
                for (int dx = -halfSize; dx <= halfSize - offSet; dx++) {
                    int nX = x + dx;
                    int nY = y + dy;
 
                    switch (padding) {
                    case ZERO_PADDING:
                        if (nX < 0 || nX >= width || nY < 0 || nY >= height) {
                            // If out of bounds, use zero for zero padding
                            continue;
                        }
                        break;

                    case MIRROR_PADDING:
                        if (nX < 0) nX = -nX;
                        if (nX >= width) nX = 2 * width - nX - 1;
                        if (nY < 0) nY = -nY;
                        if (nY >= height) nY = 2 * height - nY - 1;
                        break;
                    }
                    int pixelIndex = (nY * width + nX) * 4;
                    float filterValue = filter[dy + halfSize][dx + halfSize];
                    // Accumulate the weighted sum for each color channel
                    redSum += origData[pixelIndex + RED] * filterValue;
                    greenSum += origData[pixelIndex + GREEN] * filterValue;
                    blueSum += origData[pixelIndex + BLUE] * filterValue;

                }
            } 
            int currentIndex = (y * width + x) * 4;
            newData[currentIndex + RED] = static_cast<unsigned char>(Clamp(0.0f, 255.0f, redSum));
            newData[currentIndex + GREEN] = static_cast<unsigned char>(Clamp(0.0f, 255.0f, greenSum));
            newData[currentIndex + BLUE] = static_cast<unsigned char>(Clamp(0.0f, 255.0f, blueSum));
            newData[currentIndex + ALPHA] = origData[currentIndex + ALPHA];
        }
    }
    std::copy(newData.begin(), newData.end(), origData);
    return true;
}

std::vector<std::vector<float>> Tofilter(const float* filterData, int filterSize) {
    std::vector<std::vector<float>> Filter(filterSize,std::vector<float>(filterSize));
    float sum = 0.0f;
    for (int i = 0; i < filterSize * filterSize; i++) {
        sum += filterData[i];
    }
    if (sum != 0.0f) {
        for (int i = 0; i < filterSize; i++) {
            for (int j = 0; j < filterSize; j++) {
                Filter[i][j] = filterData[i * filterSize + j] / sum;
            }
        }
    }
    return Filter;
}

std::vector<std::vector<float>> BionomialFilter(int filterSize) {
    std::vector<float> binomialCoeffs(filterSize);
    for (int i = 0; i < filterSize; i++) 
        binomialCoeffs[i] = Binomial(filterSize - 1, i);
    float* filterdata = new float[filterSize * filterSize];
    for (int i = 0; i < filterSize; i++) 
        for (int j = 0; j < filterSize; j++) 
            filterdata[i * filterSize + j] = binomialCoeffs[i] * binomialCoeffs[j];
    std::vector<std::vector<float>> result = Tofilter(filterdata, filterSize);
    delete[] filterdata;
    return result;
}

bool TargaImage::Filter_Box()
{
    static float filterData[25] = {
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1
    };
    applyFilter(Tofilter(filterData, 5), 5, paddingType::ZERO_PADDING, this->data, width, height);
    return true;
}// Filter_Box


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Bartlett filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Bartlett()
{
    float filterData[25] = {
        1, 2, 3, 2, 1,
        2, 4, 6, 4, 2,
        3, 6, 9, 6, 3,
        2, 4, 6, 4, 2,
        1, 2, 3, 2, 1
    };
    applyFilter(Tofilter(filterData, 5), 5, paddingType::ZERO_PADDING, this->data, width, height);
    return true;
}// Filter_Bartlett


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Gaussian()
{
    applyFilter(BionomialFilter(5), 5, paddingType::ZERO_PADDING, this->data, width, height);
    return true;
}// Filter_Gaussian

///////////////////////////////////////////////////////////////////////////////
//
//      Perform NxN Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////

bool TargaImage::Filter_Gaussian_N( unsigned int N )
{
    applyFilter(BionomialFilter(N), N, paddingType::ZERO_PADDING, this->data, width, height);
    return true;
}// Filter_Gaussian_N


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 edge detect (high pass) filter on this image.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Edge()
{
    ClearToBlack();
    return false;
}// Filter_Edge


///////////////////////////////////////////////////////////////////////////////
//
//      Perform a 5x5 enhancement filter to this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Enhance()
{
    ClearToBlack();
    return false;
}// Filter_Enhance


///////////////////////////////////////////////////////////////////////////////
//
//      Run simplified version of Hertzmann's painterly image filter.
//      You probably will want to use the Draw_Stroke funciton and the
//      Stroke class to help.
// Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::NPR_Paint()
{
    ClearToBlack();
    return false;
}



///////////////////////////////////////////////////////////////////////////////
//
//      Halve the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Half_Size()
{
    ClearToBlack();
    return false;
}// Half_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Double the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Double_Size()
{
    ClearToBlack();
    return false;
}// Double_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Scale the image dimensions by the given factor.  The given factor is 
//  assumed to be greater than one.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////


bool TargaImage::Resize(float scale)
{
    int newHeight = static_cast<int>(height * scale);
    int newWidth = static_cast<int>(width * scale);
    static std::vector<std::vector<float>> filterEvenEven = {
        { 1 / 16.0f, 1 / 8.0f, 1 / 16.0f, 0 },
        { 1 / 8.0f,  1 / 4.0f, 1 / 8.0f, 0  },
        { 1 / 16.0f, 1 / 8.0f, 1 / 16.0f, 0 },
        { 0, 0, 0, 0 }
    };
    static std::vector<std::vector<float>> filterOddOdd = {
        { 1 / 64.0f, 3 / 64.0f, 3 / 64.0f, 1 / 64.0f },
        { 3 / 64.0f, 9 / 64.0f, 9 / 64.0f, 3 / 64.0f },
        { 3 / 64.0f, 9 / 64.0f, 9 / 64.0f, 3 / 64.0f },
        { 1 / 64.0f, 3 / 64.0f, 3 / 64.0f, 1 / 64.0f }
    };
    static std::vector<std::vector<float>> filterEvenOdd = {
        { 1 / 32.0f, 2 / 32.0f, 1 / 32.0f, 0},
        { 3 / 32.0f, 6 / 32.0f, 3 / 32.0f, 0},
        { 3 / 32.0f, 6 / 32.0f, 3 / 32.0f, 0},
        { 1 / 32.0f, 2 / 32.0f, 1 / 32.0f, 0}
    };
    static std::vector<std::vector<float>> filterOddEven = {
        { 1 / 64.0f, 3 / 64.0f, 3 / 64.0f, 1 / 64.0f },
        { 2 / 64.0f, 6 / 64.0f, 6 / 64.0f, 2 / 64.0f },
        { 1 / 64.0f, 3 / 64.0f, 3 / 64.0f, 1 / 64.0f },
        { 0, 0, 0, 0 }
    };
    
    unsigned char* resizedData = new unsigned char[newHeight * newWidth * 4];
    for (int i = 0; i < newHeight; i++) {
        for (int j = 0; j < newWidth; j++) {
            // Map the pixel to the original image
            int srcX = static_cast<int>(j / scale);
            int srcY = static_cast<int>(i / scale);
            // Select the correct filter based on the pixel position (even/odd)
            std::vector<std::vector<float>> selectedFilter;
            if (i % 2 == 0 && j % 2 == 0) 
                selectedFilter = filterEvenEven;
            else if (i % 2 == 1 && j % 2 == 1) 
                selectedFilter = filterOddOdd;
            else if (i % 2 == 0 && j % 2 == 1) 
                selectedFilter = filterEvenOdd;
            else if (i % 2 == 1 && j % 2 == 0) 
                selectedFilter = filterOddEven;
            // Apply the filter
            for (int c = 0; c < 4; c++) {
                float sum = 0.0f;
                for (int dy = 0; dy < 4; dy++) {
                    for (int dx = 0; dx < 4; dx++) {
                        int neighborX = srcX + dx - 1;
                        int neighborY = srcY + dy - 1;
                        if (neighborX >= 0 && neighborX < width && neighborY >= 0 && neighborY < height) {
                            sum += data[(neighborY * width + neighborX) * 4 + c] * selectedFilter[dy][dx];
                        }
                    }
                }
                // Set the filtered pixel value in resized data
                resizedData[(i * newWidth + j) * 4 + c] = static_cast<unsigned char>(sum);
            }
        }
    }
    delete[] data;
    data = resizedData;
    //std::copy(resizedData, resizedData + newHeight * newWidth * 4, data);
    width = newWidth;
    height = newHeight;
    return true;
}// Resize


//////////////////////////////////////////////////////////////////////////////
//
//      Rotate the image clockwise by the given angle.  Do not resize the 
//  image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Rotate(float angleDegrees)
{
    ClearToBlack();
    return false;
}// Rotate


//////////////////////////////////////////////////////////////////////////////
//
//      Given a single RGBA pixel return, via the second argument, the RGB
//      equivalent composited with a black background.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::RGBA_To_RGB(unsigned char *rgba, unsigned char *rgb)
{
    const unsigned char	BACKGROUND[3] = { 0, 0, 0 };

    unsigned char  alpha = rgba[3];

    if (alpha == 0)
    {
        rgb[0] = BACKGROUND[0];
        rgb[1] = BACKGROUND[1];
        rgb[2] = BACKGROUND[2];
    }
    else
    {
	    float	alpha_scale = (float)255 / (float)alpha;
	    int	val;
	    int	i;

	    for (i = 0 ; i < 3 ; i++)
	    {
	        val = (int)floor(rgba[i] * alpha_scale);
	        if (val < 0)
		    rgb[i] = 0;
	        else if (val > 255)
		    rgb[i] = 255;
	        else
		    rgb[i] = val;
	    }
    }
}// RGA_To_RGB


///////////////////////////////////////////////////////////////////////////////
//
//      Copy this into a new image, reversing the rows as it goes. A pointer
//  to the new image is returned.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Reverse_Rows(void)
{
    unsigned char   *dest = new unsigned char[width * height * 4];
    TargaImage	    *result;
    int 	        i, j;

    if (! data)
    	return NULL;

    for (i = 0 ; i < height ; i++)
    {
	    int in_offset = (height - i - 1) * width * 4;
	    int out_offset = i * width * 4;

	    for (j = 0 ; j < width ; j++)
        {
	        dest[out_offset + j * 4] = data[in_offset + j * 4];
	        dest[out_offset + j * 4 + 1] = data[in_offset + j * 4 + 1];
	        dest[out_offset + j * 4 + 2] = data[in_offset + j * 4 + 2];
	        dest[out_offset + j * 4 + 3] = data[in_offset + j * 4 + 3];
        }
    }

    result = new TargaImage(width, height, dest);
    delete[] dest;
    return result;
}// Reverse_Rows


///////////////////////////////////////////////////////////////////////////////
//
//      Clear the image to all black.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::ClearToBlack()
{
    memset(data, 0, width * height * 4);
}// ClearToBlack


///////////////////////////////////////////////////////////////////////////////
//
//      Helper function for the painterly filter; paint a stroke at
// the given location
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::Paint_Stroke(const Stroke& s) {
   int radius_squared = (int)s.radius * (int)s.radius;
   for (int x_off = -((int)s.radius); x_off <= (int)s.radius; x_off++) {
      for (int y_off = -((int)s.radius); y_off <= (int)s.radius; y_off++) {
         int x_loc = (int)s.x + x_off;
         int y_loc = (int)s.y + y_off;
         // are we inside the circle, and inside the image?
         if ((x_loc >= 0 && x_loc < width && y_loc >= 0 && y_loc < height)) {
            int dist_squared = x_off * x_off + y_off * y_off;
            if (dist_squared <= radius_squared) {
               data[(y_loc * width + x_loc) * 4 + 0] = s.r;
               data[(y_loc * width + x_loc) * 4 + 1] = s.g;
               data[(y_loc * width + x_loc) * 4 + 2] = s.b;
               data[(y_loc * width + x_loc) * 4 + 3] = s.a;
            } else if (dist_squared == radius_squared + 1) {
               data[(y_loc * width + x_loc) * 4 + 0] = 
                  (data[(y_loc * width + x_loc) * 4 + 0] + s.r) / 2;
               data[(y_loc * width + x_loc) * 4 + 1] = 
                  (data[(y_loc * width + x_loc) * 4 + 1] + s.g) / 2;
               data[(y_loc * width + x_loc) * 4 + 2] = 
                  (data[(y_loc * width + x_loc) * 4 + 2] + s.b) / 2;
               data[(y_loc * width + x_loc) * 4 + 3] = 
                  (data[(y_loc * width + x_loc) * 4 + 3] + s.a) / 2;
            }
         }
      }
   }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke() {}

///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke(unsigned int iradius, unsigned int ix, unsigned int iy,
               unsigned char ir, unsigned char ig, unsigned char ib, unsigned char ia) :
   radius(iradius),x(ix),y(iy),r(ir),g(ig),b(ib),a(ia)
{
}

