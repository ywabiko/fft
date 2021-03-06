#pragma once

#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <memory>
#include <stdint.h>
#include <complex>
#include <vector>
#include <iomanip>
#include <ctime>
#include <string>
#include <stdio.h>
#include <Windows.h>
#include <stdint.h>
typedef std::vector<std::complex<float>>   single_vector_t;
typedef std::vector<std::complex<double>>  double_vec_t;
typedef std::vector<std::complex<double>>  complex_vector_t;
typedef std::vector<std::complex<int64_t>> integer_vector_t;

class CFFT
{
public:
    CFFT(uint32_t Size, bool isPositive);
    ~CFFT();
    bool Init(const uint32_t Size, bool isPositive);
    bool SetWindowFunction(const uint32_t number);
    bool Execute(complex_vector_t* Signal);
    bool Execute(integer_vector_t* Signal);
    bool Execute(single_vector_t* Signal);
    bool SetTestVector(uint32_t scenario, complex_vector_t* vec, complex_vector_t* expected);
    bool SetTestVector(uint32_t scenario, integer_vector_t* vec, integer_vector_t* expected);
    bool SetTestVector(uint32_t scenario, single_vector_t* vec, single_vector_t* expected);
    bool RunTestDouble(uint32_t scenario, uint32_t count, uint32_t size);
    bool RunTestFixedPoint(uint32_t scenario, uint32_t count, uint32_t size);
    bool RunTestSingle(uint32_t scenario, uint32_t count, uint32_t size);

private:
    bool ApplyWindowFunction(complex_vector_t input, complex_vector_t *output);
    bool ApplyWindowFunction(integer_vector_t input, integer_vector_t *output);
    bool ApplyWindowFunction(single_vector_t input, single_vector_t *output);
    uint64_t PrintTime();
    uint64_t PrintTime(uint64_t start);

    unsigned int transformSize_; // FFTサイズ (FFT Size)
    uint32_t windowFunction_; // 窓関数ID (0=Hamming, 1=Hanning)
    bool isPositive_; // FFTかIFFTかのフラグ。FFTならtrue、IFFTならfalse (FFT/IFFT flag. FFT=true, IFFT=false)
    std::vector<uint32_t> bitReversed_; // ビット反転テーブル (Bit-reversing table)
};
