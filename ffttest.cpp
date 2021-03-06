#include "ffttest.h"
#include "fft.h"

// ゆるい感じのテストコード
//
// RunTest()の中ではテストケースに応じてサイズや正逆フラグを再設定しているので実は CFFT()コンストラクタの引数はなくてもよい。気分の問題、、、
// In RunTest(), we actually reconfigure positive/negative flag and FFT size depending on each test case spec, so there is no need to specify arguments of CFFT() constructor.
//
int main()
{
    bool ExecuteWindowFunctionTest = true;
    bool ExecuteFixedPoint16 = true;
    bool ExecuteFixedPoint8 = true;
    bool ExecuteDouble16 = true;
    bool ExecuteDouble8 = true;
    bool ExecutePerfFixedPoint4096 = true;
    bool ExecutePerfDouble4096 = true;
    bool ExecutePerfSingle4096 = true;

    // Window function test
    if (ExecuteWindowFunctionTest)
    {
        wprintf(L"Window Function Test\n");
        std::unique_ptr<CFFT> fft(new CFFT(16, true));
        bool result = false;
        complex_vector_t inp;
        complex_vector_t out;
        inp.resize(16);
        out.resize(16);

        for (uint32_t i = 0; i < 16; i++)
        {
            inp[i] = std::complex<double>(100.0, 100.0);
        }

        std::copy(inp.begin(), inp.end(), out.begin());
        result = fft->SetWindowFunction(1);
        result = fft->Execute(&out);
        std::copy(inp.begin(), inp.end(), out.begin());
        result = fft->SetWindowFunction(2);
        result = fft->Execute(&out);
    }

    // Fixed Floating-point test
    // Functional Test - 16-point FFT/IFFT
    if (ExecuteFixedPoint16)
    {
        std::unique_ptr<CFFT> fft(new CFFT(16, true));
        bool result = true;

        for (uint32_t i = 1; i < 4; i++)
        {
            wprintf(L"Func Test (Fixed Floating-point 16-point) %d ", i);
            result = fft->RunTestFixedPoint(i, 1, 16);

            if (result)
            {
                wprintf(L"Pass\n");
            }
            else
            {
                wprintf(L"Fail\n");
            }
        }
    }

    // Fixed Floating-point test
    // Functional Test - 8-point FFT/IFFT
    if (ExecuteFixedPoint8)
    {
        std::unique_ptr<CFFT> fft(new CFFT(8, true));
        bool result = true;
        {
            wprintf(L"Func Test (Fixed Floating-point 8-point) 4 ");
            result = fft->RunTestFixedPoint(4, 1, 8);

            if (result)
            {
                wprintf(L"Pass\n");
            }
            else
            {
                wprintf(L"Fail\n");
            }
        }
    }

    // Fixed Floating-point test
    // Performance Test - 4096-point FFT - 1000 iterations - random input at every iteration
    if (ExecutePerfFixedPoint4096)
    {
        wprintf(L"Perf Test - 4096-point FFT - 1000 iterations - random input at every iteration\n");
        std::unique_ptr<CFFT> fft(new CFFT(4096, true));
        bool result = false;
        result = fft->RunTestFixedPoint(0, 1000, 4096);
    }

    // Functional Test (double) - 16-point FFT/IFFT
    if (ExecuteDouble16)
    {
        std::unique_ptr<CFFT> fft(new CFFT(16, true));
        bool result = true;
        for (uint32_t i = 1; i < 4; i++)
        {
            wprintf(L"Func Test (Double) 16-point FFT/IFFT - %d ", i);
            result = fft->RunTestDouble(i, 1, 16);
            if (result)
            {
                wprintf(L"Pass\n");
            }
            else
            {
                wprintf(L"Fail\n");
            }
        }
    }

    // Functional Test (double) - 8-point FFT/IFFT
    if (ExecuteDouble8)
    {
        std::unique_ptr<CFFT> fft(new CFFT(8, true));
        bool result = true;
        {
            wprintf(L"Func Test (Double) 8-point FFT/IFFT - 4 ");
            result = fft->RunTestDouble(4, 1, 8);
            if (result)
            {
                wprintf(L"Pass\n");
            }
            else
            {
                wprintf(L"Fail\n");
            }
        }
    }

    // Performance Test (double) - 4096-point FFT - 1000 iteration - random input at every iteration
    if (ExecutePerfDouble4096)
    {
        wprintf(L"Perf Test (Double) - 4096-point FFT - 1000 iteration - random input at every iteration\n");
        std::unique_ptr<CFFT> fft(new CFFT(4096, true));
        bool result = false;
        result = fft->RunTestDouble(0, 1000, 4096);
    }

    // Performance Test (single) - 4096-point FFT - 1000 iteration - random input at every iteration
    if (ExecutePerfSingle4096)
    {
        wprintf(L"Perf Test (Single) - 4096-point FFT - 1000 iteration - random input at every iteration\n");
        std::unique_ptr<CFFT> fft(new CFFT(4096, true));
        bool result = false;
        result = fft->RunTestSingle(0, 1000, 4096);
    }

    wprintf(L"Hit Enter Key to Exit\n");
    getchar();

#ifdef DEBUG_
    const uint32_t samplesize=8;
    complex_vector_t inp;
    complex_vector_t out1;
    complex_vector_t out2;
    inp.resize(samplesize);
    out1.resize(samplesize);
    out2.resize(samplesize);
    for(uint32_t i=0;i<samplesize;i++)
    {
    //inp[i]=(double)i; // 0,1,2,3,...
    //inp[i]=std::complex<double>(cos(2.0*M_PI*i/360),0);
    }
    //setvalue1(&inp);
    setvalue2(&inp);
    std::unique_ptr<CFFT> fft(new CFFT(samplesize,true));
    std::unique_ptr<CFFT> ifft(new CFFT(samplesize,false));
    for(uint32_t i=0;i<1;i++)
    {
    std::copy(inp.begin(),inp.end(),out1.begin());
    fft->Execute(&out1);
    std::copy(out1.begin(),out1.end(),out2.begin());
    ifft->Execute(&out2);
    }

    for(uint32_t i=0;i<cSampleSize;i++)
    {
    std::cout << out1[i].real() << " " << out1[i].imag() << std::endl;
    }
#endif

    return 0;
}
