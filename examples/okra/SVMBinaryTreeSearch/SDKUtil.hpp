/**********************************************************************
Copyright ©2014 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/


#ifndef SDKUTIL_HPP_
#define SDKUTIL_HPP_

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>
#include <cmath>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <malloc.h>
#include <math.h>
#include <numeric>
#include <stdint.h>


#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#define _aligned_malloc __mingw_aligned_malloc
#define _aligned_free  __mingw_aligned_free
#endif // __MINGW32__  and __MINGW64_VERSION_MAJOR

#ifndef _WIN32
#if defined(__INTEL_COMPILER)
#pragma warning(disable : 1125)
#endif
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <linux/limits.h>
#include <unistd.h>
#endif

/******************************************************************************
* Defined macros                                                              *
******************************************************************************/
#define SDK_SUCCESS 0
#define SDK_FAILURE 1
#define SDK_EXPECTED_FAILURE 2

#define CHECK_ALLOCATION(actual, msg) \
    if(actual == NULL) \
    { \
        error(msg); \
        std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
        return SDK_FAILURE; \
    }

#define CHECK_ERROR(actual, reference, msg) \
    if(actual != reference) \
    { \
        error(msg); \
        std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
        return SDK_FAILURE; \
    }

#define FREE(ptr) \
    { \
        if(ptr != NULL) \
        { \
            free(ptr); \
            ptr = NULL; \
        } \
    }

#ifdef _WIN32
#define ALIGNED_FREE(ptr) \
    { \
        if(ptr != NULL) \
        { \
            _aligned_free(ptr); \
            ptr = NULL; \
        } \
    }
#endif


/******************************************************************************
* namespace boltsdk                                                           *
******************************************************************************/
namespace appsdk
{

/**************************************************************************
* CmdArgsEnum                                                             *
* Enum for datatype of CmdArgs                                            *
**************************************************************************/
enum CmdArgsEnum
{
    CA_ARG_INT,
    CA_ARG_FLOAT,
    CA_ARG_DOUBLE,
    CA_ARG_STRING,
    CA_NO_ARGUMENT
};

/**
 * error
 * constant function, Prints error messages
 * @param errorMsg std::string message
 */
static void error(std::string errorMsg)
{
    std::cout<<"Error: "<<errorMsg<<std::endl;
}

/**
 * expectedError
 * constant function, Prints error messages
 * @param errorMsg char* message
 */
static void expectedError(const char* errorMsg)
{
    std::cout<<"Expected Error: "<<errorMsg<<std::endl;
}

/**
 * expectedError
 * constant function, Prints error messages
 * @param errorMsg string message
 */
static void expectedError(std::string errorMsg)
{
    std::cout<<"Expected Error: "<<errorMsg<<std::endl;
}


/**
* compare template version
* compare data to check error
* @param refData templated input
* @param data templated input
* @param length number of values to compare
* @param epsilon errorWindow
*/
static bool compare(const float *refData, const float *data,
             const int length, const float epsilon = 1e-6f)
{
    float error = 0.0f;
    float ref = 0.0f;
    for(int i = 1; i < length; ++i)
    {
        float diff = refData[i] - data[i];
        error += diff * diff;
        ref += refData[i] * refData[i];
    }
    float normRef =::sqrtf((float) ref);
    if (::fabs((float) ref) < 1e-7f)
    {
        return false;
    }
    float normError = ::sqrtf((float) error);
    error = normError / normRef;
    return error < epsilon;
}
static bool compare(const double *refData, const double *data,
             const int length, const double epsilon = 1e-6)
{
    double error = 0.0;
    double ref = 0.0;
    for(int i = 1; i < length; ++i)
    {
        double diff = refData[i] - data[i];
        error += diff * diff;
        ref += refData[i] * refData[i];
    }
    double normRef =::sqrt((double) ref);
    if (::fabs((double) ref) < 1e-7)
    {
        return false;
    }
    double normError = ::sqrt((double) error);
    error = normError / normRef;
    return error < epsilon;
}

bool compare(int *refData, int *data, int length) {

    int i;
    for (i=0;i<length;i++)
	if (refData[i] != data[i])
		return 0;
    return 1;
}

/**
 * strComparei
 * Case insensitive compare of 2 strings
 * returns true when strings match(case insensitive), false otherwise
 */
static bool strComparei(std::string a, std::string b)
{
    int sizeA = (int)a.size();
    if (b.size() != sizeA)
    {
        return false;
    }
    for (int i = 0; i < sizeA; i++)
    {
        if (tolower(a[i]) != tolower(b[i]))
        {
            return false;
        }
    }
    return true;
}

/**
 * toString
 * convert a T type to string
 */
template<typename T>
std::string toString(T t, std::ios_base & (*r)(std::ios_base&) = std::dec)
{
    std::ostringstream output;
    output << r << t;
    return output.str();
}

/**
 * filetoString
 * converts any file into a string
 * @param file string message
 * @param str string message
 * @return 0 on success Positive if expected and Non-zero on failure
 */
static int fileToString(std::string &fileName, std::string &str)
{
    size_t      size;
    char*       buf;
    // Open file stream
    std::fstream f(fileName.c_str(), (std::fstream::in | std::fstream::binary));
    // Check if we have opened file stream
    if (f.is_open())
    {
        size_t  sizeFile;
        // Find the stream size
        f.seekg(0, std::fstream::end);
        size = sizeFile = (size_t)f.tellg();
        f.seekg(0, std::fstream::beg);
        buf = new char[size + 1];
        if (!buf)
        {
            f.close();
            return  SDK_FAILURE;
        }
        // Read file
        f.read(buf, sizeFile);
        f.close();
        str[size] = '\0';
        str = buf;
        return SDK_SUCCESS;
    }
    else
    {
        error("Converting file to string. Cannot open file.");
        str = "";
        return SDK_FAILURE;
    }
}

/**
*******************************************************************
* @fn printArray
* @brief displays a array on std::out
******************************************************************/
template<typename T>
void printArray(
    const std::string header,
    const T * data,
    const int width,
    const int height)
{
    std::cout<<"\n"<<header<<"\n";
    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            std::cout<<data[i*width+j]<<" ";
        }
        std::cout<<"\n";
    }
    std::cout<<"\n";
}

/**
*******************************************************************
* @fn printArray
* @brief displays a array of opencl vector types on std::out
******************************************************************/
template<typename T>
void printArray(
    const std::string header,
    const T * data,
    const int width,
    const int height,
	int veclen)
{
    std::cout<<"\n"<<header<<"\n";
    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            std::cout << "(";
			for(int k=0; k<veclen; k++)
			{
				std::cout<<data[i*width+j].s[k]<<", ";
			}
			std::cout << ") ";
        }
        std::cout<<"\n";
    }
    std::cout<<"\n";
}


/**
*******************************************************************
* @fn printArray overload function
* @brief displays a array on std::out
******************************************************************/
template<typename T>
void printArray(
    const std::string header,
    const std::vector<T>& data,
    const int width,
    const int height)
{
    std::cout<<"\n"<<header<<"\n";
    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            std::cout<<data[i*width+j]<<" ";
        }
        std::cout<<"\n";
    }
    std::cout<<"\n";
}

/**
 * printstats
 * Print the results from the test
 * @param stdStr Parameter
 * @param stats Statistic value of parameter
 * @param n number
 */
static void printStatistics(std::string *statsStr, std::string * stats, int n)
{
    int *columnWidth = new int[n];
    if(columnWidth == NULL)
    {
        return;
    }
    std::cout << std::endl << "|";
    for(int i=0; i<n; i++)
    {
        columnWidth[i] = (int) ((statsStr[i].length() > stats[i].length())?
                                statsStr[i].length() : stats[i].length());
        std::cout << " " << std::setw(columnWidth[i]+1) << std::left << statsStr[i] <<
                  "|";
    }
    std::cout << std::endl << "|";
    for(int i=0; i<n; i++)
    {
        for(int j=0; j<(columnWidth[i]+2); j++)
        {
            std::cout << "-";
        }
        std::cout << "|";
    }
    std::cout << std::endl << "|";
    for(int i=0; i<n; i++)
    {
        std::cout << " " << std::setw(columnWidth[i]+1) << std::left << stats[i] << "|";
    }
    std::cout << std::endl;
    if(columnWidth)
    {
        delete[] columnWidth;
    }
}

/**
 * fillRandom
 * fill array with random values
 */
template<typename T>
int fillRandom(
    T * arrayPtr,
    const int width,
    const int height,
    const T rangeMin,
    const T rangeMax,
    unsigned int seed=123)
{
    if(!arrayPtr)
    {
        error("Cannot fill array. NULL pointer.");
        return SDK_FAILURE;
    }
    if(!seed)
    {
        seed = (unsigned int)time(NULL);
    }
    srand(seed);
    double range = double(rangeMax - rangeMin) + 1.0;
    /* random initialisation of input */
    for(int i = 0; i < height; i++)
        for(int j = 0; j < width; j++)
        {
            int index = i*width + j;
            arrayPtr[index] = rangeMin + T(range*rand()/(RAND_MAX + 1.0));
        }
    return SDK_SUCCESS;
}

/**
 * fillPos
 * fill the specified positions
 */
template<typename T>
int fillPos(
    T * arrayPtr,
    const int width,
    const int height)
{
    if(!arrayPtr)
    {
        error("Cannot fill array. NULL pointer.");
        return SDK_FAILURE;
    }
    /* initialisation of input with positions*/
    for(T i = 0; i < height; i++)
        for(T j = 0; j < width; j++)
        {
            T index = i*width + j;
            arrayPtr[index] = index;
        }
    return SDK_SUCCESS;
}

/**
 * fillConstant
 * fill the array with constant value
 */
template<typename T>
int fillConstant(
    T * arrayPtr,
    const int width,
    const int height,
    const T val)
{
    if(!arrayPtr)
    {
        error("Cannot fill array. NULL pointer.");
        return SDK_FAILURE;
    }
    /* initialisation of input with constant value*/
    for(int i = 0; i < height; i++)
        for(int j = 0; j < width; j++)
        {
            int index = i*width + j;
            arrayPtr[index] = val;
        }
    return SDK_SUCCESS;
}


/**
 * roundToPowerOf2
 * rounds to a power of 2
 */
template<typename T>
T roundToPowerOf2(T val)
{
    int bytes = sizeof(T);
    val--;
    for(int i = 0; i < bytes; i++)
    {
        val |= val >> (1<<i);
    }
    val++;
    return val;
}

/**
 * isPowerOf2
 * checks if input is a power of 2
 */
template<typename T>
int isPowerOf2(T val)
{
    long long _val = val;
    if((_val & (-_val))-_val == 0 && _val != 0)
    {
        return SDK_SUCCESS;
    }
    else
    {
        return SDK_FAILURE;
    }
}

/**
        * getPath
        * @return path of the current directory
        */
static std::string getPath()
{
#ifdef _WIN32
    char buffer[MAX_PATH];
#ifdef UNICODE
    if(!GetModuleFileName(NULL, (LPWCH)buffer, sizeof(buffer)))
    {
        throw std::string("GetModuleFileName() failed!");
    }
#else
    if(!GetModuleFileName(NULL, buffer, sizeof(buffer)))
    {
        throw std::string("GetModuleFileName() failed!");
    }
#endif
    std::string str(buffer);
    /* '\' == 92 */
    int last = (int)str.find_last_of((char)92);
#else
    char buffer[PATH_MAX + 1];
    ssize_t len;
    if((len = readlink("/proc/self/exe",buffer, sizeof(buffer) - 1)) == -1)
    {
        throw std::string("readlink() failed!");
    }
    buffer[len] = '\0';
    std::string str(buffer);
    /* '/' == 47 */
    int last = (int)str.find_last_of((char)47);
#endif
    return str.substr(0, last + 1);
}

