#include "fft.h"
static const int64_t cOneFixed64 = 0x100000000;

CFFT::CFFT(const uint32_t size, bool isPositive)
{
    Init(size, isPositive);
    return;
}

bool CFFT::Init(const uint32_t transformSize, bool isPositive)
{
    // TODO: Sizeが2の累乗でなければエラー終了 (TODO: Errors out non-power-of-two case)
    transformSize_ = transformSize;
    isPositive_ = isPositive;
    windowFunction_ = 0;

    // ビット逆順のテーブル (Generate bit-reversing table)
    bitReversed_.resize(transformSize_);
    bitReversed_[0] = 0;
    uint32_t halfSize = transformSize_ >> 1;

    for (uint32_t i = 1; i < transformSize_; i <<= 1)
    {
        for (uint32_t j = 0; j < i; j++)
        {
            bitReversed_[i + j] = bitReversed_[j] + halfSize;
        }
        halfSize >>= 1;
    }

    return true;
}

CFFT::~CFFT()
{
    return;
}

// SetWindowFunction()
// 0 = none (for functional testing)
// 1 = hamming
// 2 = hanning
bool CFFT::SetWindowFunction(const uint32_t number)
{
    if (number > 2)
    {
        return false;
    }
    windowFunction_ = number;

    return true;
}

bool CFFT::ApplyWindowFunction(integer_vector_t input, integer_vector_t* output)
{
    const double theta = 2.0 * M_PI / transformSize_;

    switch (windowFunction_)
    {
    case 0: // none (for functional testing)
        std::copy(input.begin(), input.end(), output->begin());
        break;

    case 1: // Hamming
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*output)[i] = input[i] * std::complex<int64_t>((int64_t) ((cOneFixed64) *(0.54 - 0.46*cos(theta*i))), 0);
        }
        break;

    case 2: // Hanning
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*output)[i] = input[i] * std::complex<int64_t>((int64_t) ((cOneFixed64) *(0.5 - 0.5*cos(theta*i))), 0);
        }
        break;

    default:
        __debugbreak();
        break;
    }
    return true;
}

bool CFFT::ApplyWindowFunction(complex_vector_t input, complex_vector_t* output)
{
    const double theta = 2.0 * M_PI / transformSize_;

    switch (windowFunction_)
    {

    case 0: // none (for functional testing)
        std::copy(input.begin(), input.end(), output->begin());
        break;

    case 1: // Hamming
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*output)[i] = input[i] * std::complex<double>(0.54 - 0.46*cos(theta*i), 0);
        }
        break;

    case 2: // Hanning
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*output)[i] = input[i] * std::complex<double>(0.5 - 0.5*cos(theta*i), 0);
        }
        break;

    default:
        __debugbreak();
        break;
    }
    return true;
}

bool CFFT::ApplyWindowFunction(single_vector_t input, single_vector_t* output)
{
    const float theta = (float)(2.0 * M_PI / transformSize_);

    switch (windowFunction_)
    {

    case 0: // none (for functional testing)
        std::copy(input.begin(), input.end(), output->begin());
        break;

    case 1: // Hamming
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*output)[i] = input[i] * std::complex<float>((float)0.54 - (float)(0.46*cos(theta*i)), 0);
        }
        break;

    case 2: // Hanning
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*output)[i] = input[i] * std::complex<float>((float)0.5 - (float)(0.5*cos(theta*i)), 0);
        }
        break;

    default:
        __debugbreak();
        break;
    }
    return true;
}

// 固定小数点バージョン 64bit 符号付整数 = 32bit 整数部 + 32bit 小数部 (64bit fixed-point integer version - 32bit integer + 32bit decimal)
bool CFFT::Execute(integer_vector_t* signal)
{
    const uint32_t N = transformSize_;
    integer_vector_t s;
    s.resize(N);
    // 窓関数の適用 Apply window function
    ApplyWindowFunction(*signal, &s);
    const uint32_t numHalf = N / 2;
    const uint32_t numStage = (uint32_t) (log(N) / log(2)); // N=8 -> numStage=3 -> stage=0,1,2

    for (uint32_t stage = 0; stage < numStage; stage++) // 各段のループ (Stage loop)
    {
        uint32_t numPart = 1 << stage;  // N=8 -> (stage,numPart)=(0,1),(1,2),(2,4) 各段でのn点バタフライ演算する回数 number of times of n-point butterfly operation
        uint32_t partSize = N >> stage; // N=8 -> (stage,partSize)=(0,8),(1,4),(2,2) 各段でのn点バタフライ演算の要素数 number of elements of n-point butterfly operation
        uint32_t dx = numHalf >> stage; // N=8 -> (stage,dx)=(0,4),(1,2),(2,1) バタフライ演算する2要素の添え字の距離 distance of index of two elements of an n-point butterfly operation

        for (uint32_t part = 0; part < numPart; part++) // 各段でのn点バタフライ演算のループ loop for n-point butterfly operation
        {
            double theta = 2.0 * M_PI * (isPositive_ ? 1.0 : -1.0) / partSize;
            uint32_t x0 = partSize * part;

            for (uint32_t i = 0; i < dx; i++)
            {
                uint32_t x = x0 + i;
                uint32_t y = x + dx;
                std::complex<int64_t> tmp = s[x] - s[y];
                s[x] = s[x] + s[y];
                // cos()sin()の引数は時計回り。反時計回りではない。 the argument of cos()/sin() is clock wise, not counter clock wise.
                std::complex<int64_t> W = std::complex<int64_t>((int64_t) ((cOneFixed64) * cos(theta*i)), (int64_t) ((cOneFixed64) * (-sin(theta*i)))); 
                s[y] = tmp * W;
                s[y] = std::complex<int64_t>(s[y].real() >> 16 >> 16, s[y].imag() >> 16 >> 16); // (tmp*W)/(cOneFixed64); // 乗除算はスケーリングが必要（固定小数点）
            }
        }

        if (0)
        {
            std::cout << "\nStage " << stage << " -------- " << std::endl;
            for (uint32_t i = 0; i < N; i++)
            {
                std::cout << "s[" << i << "] " << s[i].real() << " " << s[i].imag() << std::endl;
            }
        }

    }

    if (isPositive_) // FFT
    {
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*signal)[i] = s[bitReversed_[i]];
        }
    }
    else // 逆FFT Inverse-FFT
    {
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*signal)[i] = s[bitReversed_[i]] / (int64_t) N;
        }
    }

    return true;
}

// 浮動小数点バージョン(64bit float version)
bool CFFT::Execute(complex_vector_t* signal)
{
    const uint32_t N = transformSize_;
    complex_vector_t s;
    s.resize(N);
    ApplyWindowFunction(*signal, &s); // 窓関数の適用 Apply window function
    const uint32_t numHalf = N / 2;
    const uint32_t numStage = (uint32_t) (log(N) / log(2)); // N=8 -> numStage=3 -> stage=0,1,2

    for (uint32_t stage = 0; stage < numStage; stage++) // 各段のループ (Stage loop)
    {
        uint32_t numPart = 1 << stage;  // N=8 -> (stage,numPart)=(0,1),(1,2),(2,4) 各段でのn点バタフライ演算する回数 number of times of n-point butterfly operation
        uint32_t partSize = N >> stage; // N=8 -> (stage,partSize)=(0,8),(1,4),(2,2) 各段でのn点バタフライ演算の要素数 number of elements of n-point butterfly operation
        uint32_t dx = numHalf >> stage; // N=8 -> (stage,dx)=(0,4),(1,2),(2,1) バタフライ演算する2要素の添え字の距離 distance of index of two elements of an n-point butterfly operation

        for (uint32_t part = 0; part < numPart; part++) // 各段でのn点バタフライ演算のループ loop for n-point butterfly operation
        {
            double theta = 2.0 * M_PI * (isPositive_ ? 1.0 : -1.0) / partSize;
            uint32_t x0 = partSize * part;

            for (uint32_t i = 0; i < dx; i++)
            {
                uint32_t x = x0 + i;
                uint32_t y = x + dx;
                std::complex<double> tmp = s[x] - s[y];
                s[x] = s[x] + s[y];
                // cos()sin()の引数は時計回り。反時計回りではなかった。 the argument of cos()/sin() is clock wise, not counter clock wise.
                std::complex<double> W = std::complex<double>(cos(theta * i), -sin(theta * i)); 
                s[y] = tmp * W;
            }
        }

        if (0)
        {
            std::cout << "\nStage " << stage << " -------- " << std::endl;
            for (uint32_t i = 0; i < N; i++)
            {
                std::cout << "s[" << i << "] " << s[i].real() << " " << s[i].imag() << std::endl;
            }
        }

    }

    if (isPositive_) // FFT
    {
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*signal)[i] = s[bitReversed_[i]];
        }
    }
    else // 逆FFT Inverse-FFT
    {
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*signal)[i] = s[bitReversed_[i]] / (double) N;
        }
    }

    return true;
}

// 浮動小数点バージョン(32bit float version)
bool CFFT::Execute(single_vector_t* signal)
{
    const uint32_t N = transformSize_;
    single_vector_t s;
    s.resize(N);
    ApplyWindowFunction(*signal, &s); // 窓関数の適用 Apply window function
    const uint32_t numHalf = N / 2;
    const uint32_t numStage = (uint32_t) (log(N) / log(2)); // N=8 -> numStage=3 -> stage=0,1,2

    for (uint32_t stage = 0; stage < numStage; stage++) // 各段のループ (Stage loop)
    {
        uint32_t numPart = 1 << stage;  // N=8 -> (stage,numPart)=(0,1),(1,2),(2,4) 各段でのn点バタフライ演算する回数 number of times of n-point butterfly operation
        uint32_t partSize = N >> stage; // N=8 -> (stage,partSize)=(0,8),(1,4),(2,2) 各段でのn点バタフライ演算の要素数 number of elements of n-point butterfly operation
        uint32_t dx = numHalf >> stage; // N=8 -> (stage,dx)=(0,4),(1,2),(2,1) バタフライ演算する2要素の添え字の距離 distance of index of two elements of an n-point butterfly operation

        for (uint32_t part = 0; part < numPart; part++) // 各段でのn点バタフライ演算のループ loop for n-point butterfly operation
        {
            float theta = (float)(2.0 * M_PI * (isPositive_ ? 1.0 : -1.0) / partSize);
            uint32_t x0 = partSize * part;

            for (uint32_t i = 0; i < dx; i++)
            {
                uint32_t x = x0 + i;
                uint32_t y = x + dx;
                std::complex<float> tmp = s[x] - s[y];
                s[x] = s[x] + s[y];
                // cos()sin()の引数は時計回り。反時計回りではなかった。 the argument of cos()/sin() is clock wise, not counter clock wise.
                std::complex<float> W = std::complex<float>(cos(theta * i), -sin(theta * i)); 
                s[y] = tmp * W;
            }
        }

        if (0)
        {
            std::cout << "\nStage " << stage << " -------- " << std::endl;
            for (uint32_t i = 0; i < N; i++)
            {
                std::cout << "s[" << i << "] " << s[i].real() << " " << s[i].imag() << std::endl;
            }
        }

    }

    if (isPositive_) // FFT
    {
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*signal)[i] = s[bitReversed_[i]];
        }
    }
    else // 逆FFT Inverse-FFT
    {
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*signal)[i] = s[bitReversed_[i]] / (float) N;
        }
    }

    return true;
}

// For functional test: set count=1;
// For performance test: set count>1;
bool CFFT::RunTestFixedPoint(uint32_t scenario, uint32_t count, uint32_t samplesize)
{
    integer_vector_t inp;
    integer_vector_t out1;
    integer_vector_t out2;
    integer_vector_t expected;
    // 期待値との許容誤差イプシロン。0.0001fだと一部Failする。 Acceptable error tolerance. 0.0001f is too small for some test cases.
    const int64_t tolerance = abs(std::complex<int64_t>((int64_t) (cOneFixed64 * 0.0002f), (int64_t) (cOneFixed64 * 0.0002f))); 
    inp.resize(samplesize);
    out1.resize(samplesize);
    out2.resize(samplesize);
    expected.resize(samplesize);

    // 入力値と期待値のセット Set input and expected values.
    SetTestVector(scenario, &inp, &expected);

    // FFTの実行 Execute FFT
    if (count > 1) { // Perf Test - FFT only
        uint64_t start = PrintTime();
        Init(samplesize, true);

        for (uint32_t i = 0; i < count; i++)
        {
            std::copy(inp.begin(), inp.end(), out1.begin());
            Execute(&out1);
            // 毎回入力を乱数で埋める。キャッシュ対策。 Fill out the input vector with random values to avoid too much cache hit for accurate performance measurement.
            SetTestVector(scenario, &inp, &expected); 
        }
        PrintTime(start);
    }
    else
    { // Func Test - FFT+IFFT
        Init(samplesize, true);
        std::copy(inp.begin(), inp.end(), out1.begin());
        Execute(&out1);
        Init(samplesize, false);
        std::copy(out1.begin(), out1.end(), out2.begin());
        Execute(&out2);
    }

    // 結果の検証 Verify FFT results
    bool isPassed = true;
    if (count == 1) // Funtional test の場合は出力値と期待値を比較する。Performance test のときはしない。For functional tests, we compare the actual output with the expected output.
    {

        for (uint32_t i = 0; i < samplesize; i++)
        {
            std::complex<int64_t> diff_fft = expected[i] - out1[i]; // FFT期待値と実際値の差分 Difference between the expected FFT value and actual FFT value
            std::complex<int64_t> diff_ifft = inp[i] - out2[i];     // IFFT期待値と実際値の差分 Difference between the expected IFFT value and actual IFFT value
            int64_t error_fft = abs(diff_fft);
            int64_t error_ifft = abs(diff_ifft);

            if (error_fft > tolerance) // FFT出力のチェック
            {
                isPassed = false;
            }

            if (error_ifft > tolerance) // IFFT出力のチェック
            {
                isPassed = false;
            }

        }
    }
    return isPassed;
}


// For functional test: set count=1;
// For performance test: set count>1;
bool CFFT::RunTestDouble(uint32_t scenario, uint32_t count, uint32_t samplesize)
{
    complex_vector_t inp;
    complex_vector_t out1;
    complex_vector_t out2;
    complex_vector_t expected;
    const double tolerance = abs(std::complex<double>(0.0002f, 0.0002f)); // 期待値との許容誤差イプシロン。0.0001fだと一部Failする。 Acceptable error tolerance. 0.0001f is too small for some test cases.
    inp.resize(samplesize);
    out1.resize(samplesize);
    out2.resize(samplesize);
    expected.resize(samplesize);

    // 入力値と期待値のセット Set input and expected values.
    SetTestVector(scenario, &inp, &expected);

    // FFTの実行 Execute FFT
    if (count > 1) { // Perf Test - FFT only
        uint64_t start = PrintTime();
        Init(samplesize, true);

        for (uint32_t i = 0; i < count; i++)
        {
            std::copy(inp.begin(), inp.end(), out1.begin());
            Execute(&out1);
            // 毎回入力を乱数で埋める。キャッシュ対策。 Fill out the input vector with random values to avoid too much cache hit for accurate performance measurement.
            SetTestVector(scenario, &inp, &expected);
        }
        PrintTime(start);
    }
    else
    { // Func Test - FFT+IFFT
        Init(samplesize, true);
        std::copy(inp.begin(), inp.end(), out1.begin());
        Execute(&out1);
        Init(samplesize, false);
        std::copy(out1.begin(), out1.end(), out2.begin());
        Execute(&out2);
    }

    // 結果の検証 Verify FFT results
    bool isPassed = true;
    if (count == 1) // Funtional test の場合は出力値と期待値を比較する。Performance test のときはしない。For functional tests, we compare the actual output with the expected output.
    {

        for (uint32_t i = 0; i < samplesize; i++)
        {
            std::complex<double> diffFft = expected[i] - out1[i]; // FFT期待値と実際値の差分 Difference between the expected FFT value and actual FFT value
            std::complex<double> diffIFft = inp[i] - out2[i];     // IFFT期待値と実際値の差分 Difference between the expected IFFT value and actual IFFT value
            double errorFft= abs(diffFft);
            double errorIFft = abs(diffIFft);

            if (errorFft > tolerance) // FFT出力のチェック
            {
                isPassed = false;
            }

            if (errorIFft > tolerance) // IFFT出力のチェック
            {
                isPassed = false;
            }

        }
    }

    return isPassed;
}

bool CFFT::RunTestSingle(uint32_t scenario, uint32_t count, uint32_t samplesize)
{
    single_vector_t inp;
    single_vector_t out1;
    single_vector_t out2;
    single_vector_t expected;
    // 期待値との許容誤差イプシロン。0.0001fだと一部Failする。 Acceptable error tolerance. 0.0001f is too small for some test cases.
    const float tolerance = abs(std::complex<float>(0.0002f, 0.0002f)); 
    inp.resize(samplesize);
    out1.resize(samplesize);
    out2.resize(samplesize);
    expected.resize(samplesize);

    // 入力値と期待値のセット Set input and expected values.
    SetTestVector(scenario, &inp, &expected);

    // FFTの実行 Execute FFT
    if (count > 1) { // Perf Test - FFT only
        uint64_t start = PrintTime();
        Init(samplesize, true);

        for (uint32_t i = 0; i < count; i++)
        {
            std::copy(inp.begin(), inp.end(), out1.begin());
            Execute(&out1);
            // 毎回入力を乱数で埋める。キャッシュ対策。 Fill out the input vector with random values to avoid too much cache hit for accurate performance measurement.
            SetTestVector(scenario, &inp, &expected);
        }
        PrintTime(start);
    }
    else
    { // Func Test - FFT+IFFT
        Init(samplesize, true);
        std::copy(inp.begin(), inp.end(), out1.begin());
        Execute(&out1);
        Init(samplesize, false);
        std::copy(out1.begin(), out1.end(), out2.begin());
        Execute(&out2);
    }

    // 結果の検証 Verify FFT results
    bool isPassed = true;
    if (count == 1) // Funtional test の場合は出力値と期待値を比較する。Performance test のときはしない。For functional tests, we compare the actual output with the expected output.
    {

        for (uint32_t i = 0; i < samplesize; i++)
        {
            std::complex<double> diffFft = expected[i] - out1[i]; // FFT期待値と実際値の差分 Difference between the expected FFT value and actual FFT value
            std::complex<double> diffIFft = inp[i] - out2[i];     // IFFT期待値と実際値の差分 Difference between the expected IFFT value and actual IFFT value
            double errorFft = abs(diffFft);
            double errorIFft = abs(diffIFft);

            if (errorFft > tolerance) // FFT出力のチェック
            {
                isPassed = false;
            }

            if (errorIFft > tolerance) // IFFT出力のチェック
            {
                isPassed = false;
            }

        }
    }
    return isPassed;
}

// テスト用データの生成
bool CFFT::SetTestVector(uint32_t scenario, integer_vector_t* input, integer_vector_t* expected)
{
    // いまのところは、double版を呼んだ後に固定小数点に変換することにする。
    complex_vector_t input0;
    complex_vector_t expected0;
    uint32_t size = input->size();
    input0.resize(size);
    expected0.resize(size);
    SetTestVector(scenario, &input0, &expected0);

    for (uint32_t i = 0; i < size; i++)
    {
        (*input)[i] = std::complex<int64_t>((int64_t) (cOneFixed64 * input0[i].real()), (int64_t) (cOneFixed64 * input0[i].imag()));
        (*expected)[i] = std::complex<int64_t>((int64_t) (cOneFixed64 * expected0[i].real()), (int64_t) (cOneFixed64 * expected0[i].imag()));
    }

    return true;
}

// テスト用データの生成
bool CFFT::SetTestVector(uint32_t scenario, complex_vector_t* vec, complex_vector_t* expected)
{
    switch (scenario)
    {
    case 0: // 乱数で埋める。性能テスト用。Fill out the input with random value for performance test.
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*vec)[i] = (double) ((rand() / ((double) RAND_MAX + 1.0)) * 65536);
        }
        break;

    case 1: // エクセルで計算した期待値。エクセルは本ソースコードと同じ FFTでは1/NしないでIFFTで1/Nする宗派なのでそのまま使える。This is from Excel.
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*vec)[i] = (double) i; // 0,1,2,3,...
        }

        if (expected)
        {
            (*expected)[0] = std::complex<double>(120, 0);
            (*expected)[1] = std::complex<double>(-7.99999999999997, 40.2187159370068);
            (*expected)[2] = std::complex<double>(-7.99999999999998, 19.3137084989848);
            (*expected)[3] = std::complex<double>(-7.99999999999998, 11.9728461013239);
            (*expected)[4] = std::complex<double>(-7.99999999999999, 8);
            (*expected)[5] = std::complex<double>(-7.99999999999999, 5.3454291033544);
            (*expected)[6] = std::complex<double>(-7.99999999999999, 3.31370849898477);
            (*expected)[7] = std::complex<double>(-7.99999999999997, 1.59129893903727);
            (*expected)[8] = std::complex<double>(-8, 0);
            (*expected)[9] = std::complex<double>(-8, -1.59129893903726);
            (*expected)[10] = std::complex<double>(-8, -3.31370849898476);
            (*expected)[11] = std::complex<double>(-8, -5.34542910335438);
            (*expected)[12] = std::complex<double>(-8.00000000000001, -8);
            (*expected)[13] = std::complex<double>(-8.00000000000002, -11.9728461013239);
            (*expected)[14] = std::complex<double>(-8.00000000000002, -19.3137084989848);
            (*expected)[15] = std::complex<double>(-8.00000000000007, -40.2187159370068);
        }
        break;

    case 2: // エクセル期待値その2 This is from Excel.
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*vec)[i] = (double) ((i % 2) * 10); // 0,10,0,10,...
        }

        if (expected)
        {
            (*expected)[0] = std::complex<double>(80, 0);
            (*expected)[1] = std::complex<double>(0, 0);
            (*expected)[2] = std::complex<double>(0, 0);
            (*expected)[3] = std::complex<double>(0, 0);
            (*expected)[4] = std::complex<double>(0, 0);
            (*expected)[5] = std::complex<double>(0, 0);
            (*expected)[6] = std::complex<double>(0, 0);
            (*expected)[7] = std::complex<double>(0, 0);
            (*expected)[8] = std::complex<double>(-80, 0);
            (*expected)[9] = std::complex<double>(0, 0);
            (*expected)[10] = std::complex<double>(0, 0);
            (*expected)[11] = std::complex<double>(0, 0);
            (*expected)[12] = std::complex<double>(0, 0);
            (*expected)[13] = std::complex<double>(0, 0);
            (*expected)[14] = std::complex<double>(0, 0);
            (*expected)[15] = std::complex<double>(0, 0);
        }
        break;

    case 3: // 教科書「C++による信号処理」に載ってた例。この教科書は本ソースコードと同じ FFTでは1/NしないでIFFTで1/Nする宗派なのでそのまま使える。 From a textbook.
        (*vec)[0] = std::complex<double>(-0.2639, -0.2214);
        (*vec)[1] = std::complex<double>(0.3195, 0.1679);
        (*vec)[2] = std::complex<double>(-0.1159, 0.1218);
        (*vec)[3] = std::complex<double>(-0.1563, 0.1400);
        (*vec)[4] = std::complex<double>(0.0078, 0.0818);
        (*vec)[5] = std::complex<double>(0.2967, 0.2376);
        (*vec)[6] = std::complex<double>(0.1138, 0.3851);
        (*vec)[7] = std::complex<double>(-0.4933, 0.0148);
        (*vec)[8] = std::complex<double>(0.2077, -0.1883);
        (*vec)[9] = std::complex<double>(-0.1160, 0.1447);
        (*vec)[10] = std::complex<double>(0.0249, 0.0417);
        (*vec)[11] = std::complex<double>(-0.0997, -0.2528);
        (*vec)[12] = std::complex<double>(-0.4876, -0.2346);
        (*vec)[13] = std::complex<double>(0.4836, 0.1122);
        (*vec)[14] = std::complex<double>(-0.0469, -0.3664);
        (*vec)[15] = std::complex<double>(-0.1569, -0.4778);

        if (expected)
        {

            (*expected)[0] = std::complex<double>(-0.4824, -0.2938);
            (*expected)[1] = std::complex<double>(1.6586, -1.1617);
            (*expected)[2] = std::complex<double>(0.1032, -0.2440);
            (*expected)[3] = std::complex<double>(0.4665, 0.3113);
            (*expected)[4] = std::complex<double>(0.7264, -2.6347);
            (*expected)[5] = std::complex<double>(-0.4270, 0.2024);
            (*expected)[6] = std::complex<double>(1.1867, -0.0119);
            (*expected)[7] = std::complex<double>(-1.6761, 0.5198);
            (*expected)[8] = std::complex<double>(-0.6379, -0.4668);
            (*expected)[9] = std::complex<double>(-1.2195, -0.8730);
            (*expected)[10] = std::complex<double>(1.0336, 0.0460);
            (*expected)[11] = std::complex<double>(-0.4400, 1.5345);
            (*expected)[12] = std::complex<double>(-1.7501, 1.1452);
            (*expected)[13] = std::complex<double>(-0.6333, -0.2816);
            (*expected)[14] = std::complex<double>(-0.6291, -0.8181);
            (*expected)[15] = std::complex<double>(-1.5025, -0.5166);
        }
        break;

    case 4:  // 教科書「やり直しの信号数学」からの手計算。これはDFTの定義で1/Nする派の本でこのソースコードとは異なるため、FFT期待値を8倍する。From another textbook.
        (*vec)[0] = std::complex<double>(0, 0);
        (*vec)[1] = std::complex<double>(2, 0);
        (*vec)[2] = std::complex<double>(2 * sqrt(2), 0);
        (*vec)[3] = std::complex<double>(6, 0);
        (*vec)[4] = std::complex<double>(0, 0);
        (*vec)[5] = std::complex<double>(-10, 0);
        (*vec)[6] = std::complex<double>(-2 * sqrt(2));
        (*vec)[7] = std::complex<double>(10, 0);

        if (expected)
        {
            (*expected)[0] = std::complex<double>(1, 0);
            (*expected)[1] = std::complex<double>(sqrt(2), -sqrt(2));
            (*expected)[2] = std::complex<double>(0, 3);
            (*expected)[3] = std::complex<double>(-sqrt(2), 0);
            (*expected)[4] = std::complex<double>(-1, 0);
            (*expected)[5] = std::complex<double>(-sqrt(2), 0);
            (*expected)[6] = std::complex<double>(0, -3);
            (*expected)[7] = std::complex<double>(sqrt(2), sqrt(2));
            for (uint32_t i = 0; i < 8; i++)
            {
                (*expected)[i] *= 8.0f;
            }
        }
        break;

    case 5: // 2KHz サイン波：設定は44.1KHz, 512サンプルを仮定
        for (uint32_t i = 0; i < 512; i++)
        {
            double theta = 2.0 * M_PI * 2000 * i / 44100;
            (*vec)[i] = 16384 * sin(theta);
        }
        break;

    default:
        break;
    }

    return true;
}

// テスト用データの生成
bool CFFT::SetTestVector(uint32_t scenario, single_vector_t* vec, single_vector_t* expected)
{
    switch (scenario)
    {

    case 0: // 乱数で埋める。性能テスト用。Fill out the input with random value for performance test.
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*vec)[i] = (float) ((rand() / ((float) RAND_MAX + 1.0)) * 65536);
        }
        break;

    case 1: // エクセルで計算した期待値。エクセルは本ソースコードと同じ FFTでは1/NしないでIFFTで1/Nする宗派なのでそのまま使える。This is from Excel.
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*vec)[i] = (float) i; // 0,1,2,3,...
        }

        if (expected)
        {
            (*expected)[0] = std::complex<float>(120, 0);
            (*expected)[1] = std::complex<float>(-7.99999999999997, 40.2187159370068);
            (*expected)[2] = std::complex<float>(-7.99999999999998, 19.3137084989848);
            (*expected)[3] = std::complex<float>(-7.99999999999998, 11.9728461013239);
            (*expected)[4] = std::complex<float>(-7.99999999999999, 8);
            (*expected)[5] = std::complex<float>(-7.99999999999999, 5.3454291033544);
            (*expected)[6] = std::complex<float>(-7.99999999999999, 3.31370849898477);
            (*expected)[7] = std::complex<float>(-7.99999999999997, 1.59129893903727);
            (*expected)[8] = std::complex<float>(-8, 0);
            (*expected)[9] = std::complex<float>(-8, -1.59129893903726);
            (*expected)[10] = std::complex<float>(-8, -3.31370849898476);
            (*expected)[11] = std::complex<float>(-8, -5.34542910335438);
            (*expected)[12] = std::complex<float>(-8.00000000000001, -8);
            (*expected)[13] = std::complex<float>(-8.00000000000002, -11.9728461013239);
            (*expected)[14] = std::complex<float>(-8.00000000000002, -19.3137084989848);
            (*expected)[15] = std::complex<float>(-8.00000000000007, -40.2187159370068);
        }
        break;

    case 2: // エクセル期待値その2 This is from Excel.
        for (uint32_t i = 0; i < transformSize_; i++)
        {
            (*vec)[i] = (float) ((i % 2) * 10); // 0,10,0,10,...
        }

        if (expected)
        {
            (*expected)[0] = std::complex<float>(80, 0);
            (*expected)[1] = std::complex<float>(0, 0);
            (*expected)[2] = std::complex<float>(0, 0);
            (*expected)[3] = std::complex<float>(0, 0);
            (*expected)[4] = std::complex<float>(0, 0);
            (*expected)[5] = std::complex<float>(0, 0);
            (*expected)[6] = std::complex<float>(0, 0);
            (*expected)[7] = std::complex<float>(0, 0);
            (*expected)[8] = std::complex<float>(-80, 0);
            (*expected)[9] = std::complex<float>(0, 0);
            (*expected)[10] = std::complex<float>(0, 0);
            (*expected)[11] = std::complex<float>(0, 0);
            (*expected)[12] = std::complex<float>(0, 0);
            (*expected)[13] = std::complex<float>(0, 0);
            (*expected)[14] = std::complex<float>(0, 0);
            (*expected)[15] = std::complex<float>(0, 0);
        }
        break;

    case 3: // 教科書「C++による信号処理」に載ってた例。この教科書は本ソースコードと同じ FFTでは1/NしないでIFFTで1/Nする宗派なのでそのまま使える。 From a textbook.
        (*vec)[0] = std::complex<float>(-0.2639, -0.2214);
        (*vec)[1] = std::complex<float>(0.3195, 0.1679);
        (*vec)[2] = std::complex<float>(-0.1159, 0.1218);
        (*vec)[3] = std::complex<float>(-0.1563, 0.1400);
        (*vec)[4] = std::complex<float>(0.0078, 0.0818);
        (*vec)[5] = std::complex<float>(0.2967, 0.2376);
        (*vec)[6] = std::complex<float>(0.1138, 0.3851);
        (*vec)[7] = std::complex<float>(-0.4933, 0.0148);
        (*vec)[8] = std::complex<float>(0.2077, -0.1883);
        (*vec)[9] = std::complex<float>(-0.1160, 0.1447);
        (*vec)[10] = std::complex<float>(0.0249, 0.0417);
        (*vec)[11] = std::complex<float>(-0.0997, -0.2528);
        (*vec)[12] = std::complex<float>(-0.4876, -0.2346);
        (*vec)[13] = std::complex<float>(0.4836, 0.1122);
        (*vec)[14] = std::complex<float>(-0.0469, -0.3664);
        (*vec)[15] = std::complex<float>(-0.1569, -0.4778);

        if (expected)
        {

            (*expected)[0] = std::complex<float>(-0.4824, -0.2938);
            (*expected)[1] = std::complex<float>(1.6586, -1.1617);
            (*expected)[2] = std::complex<float>(0.1032, -0.2440);
            (*expected)[3] = std::complex<float>(0.4665, 0.3113);
            (*expected)[4] = std::complex<float>(0.7264, -2.6347);
            (*expected)[5] = std::complex<float>(-0.4270, 0.2024);
            (*expected)[6] = std::complex<float>(1.1867, -0.0119);
            (*expected)[7] = std::complex<float>(-1.6761, 0.5198);
            (*expected)[8] = std::complex<float>(-0.6379, -0.4668);
            (*expected)[9] = std::complex<float>(-1.2195, -0.8730);
            (*expected)[10] = std::complex<float>(1.0336, 0.0460);
            (*expected)[11] = std::complex<float>(-0.4400, 1.5345);
            (*expected)[12] = std::complex<float>(-1.7501, 1.1452);
            (*expected)[13] = std::complex<float>(-0.6333, -0.2816);
            (*expected)[14] = std::complex<float>(-0.6291, -0.8181);
            (*expected)[15] = std::complex<float>(-1.5025, -0.5166);
        }
        break;

    case 4:  // 教科書「やり直しの信号数学」からの手計算。これはDFTの定義で1/Nする派の本でこのソースコードとは異なるため、FFT期待値を8倍する。From another textbook.
        (*vec)[0] = std::complex<float>(0, 0);
        (*vec)[1] = std::complex<float>(2, 0);
        (*vec)[2] = std::complex<float>(2 * sqrt(2), 0);
        (*vec)[3] = std::complex<float>(6, 0);
        (*vec)[4] = std::complex<float>(0, 0);
        (*vec)[5] = std::complex<float>(-10, 0);
        (*vec)[6] = std::complex<float>(-2 * sqrt(2));
        (*vec)[7] = std::complex<float>(10, 0);

        if (expected)
        {
            (*expected)[0] = std::complex<float>(1, 0);
            (*expected)[1] = std::complex<float>(sqrt(2), -sqrt(2));
            (*expected)[2] = std::complex<float>(0, 3);
            (*expected)[3] = std::complex<float>(-sqrt(2), 0);
            (*expected)[4] = std::complex<float>(-1, 0);
            (*expected)[5] = std::complex<float>(-sqrt(2), 0);
            (*expected)[6] = std::complex<float>(0, -3);
            (*expected)[7] = std::complex<float>(sqrt(2), sqrt(2));
            for (uint32_t i = 0; i < 8; i++)
            {
                (*expected)[i] *= 8.0f;
            }
        }
        break;

    case 5: // 2KHz サイン波：設定は44.1KHz, 512サンプルを仮定
        for (uint32_t i = 0; i < 512; i++)
        {
            float theta = 2.0 * M_PI * 2000 * i / 44100;
            (*vec)[i] = 16384 * sin(theta);
        }
        break;

    default:
        break;
    }

    return true;
}

// 時間計測：開始時刻の表示用
uint64_t CFFT::PrintTime()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::cout << "Started  = " << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "." << st.wMilliseconds << std::endl;

    uint64_t current = ((st.wHour * 60 + st.wMinute) * 60 + st.wSecond) * 1000 + st.wMilliseconds;
    return current;
}

// 時間計測：終了時刻と差分の表示用
uint64_t CFFT::PrintTime(uint64_t start)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::cout << "Finished = " << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "." << st.wMilliseconds << std::endl;

    uint64_t current = ((st.wHour * 60 + st.wMinute) * 60 + st.wSecond) * 1000 + st.wMilliseconds;
    uint64_t diff = current - start;

    st.wMilliseconds = diff % 1000;
    st.wSecond = (diff / 1000) % 60;
    st.wMinute = (diff / 1000 / 60) % 60;
    st.wHour = (diff / 1000 / 60 / 60) % 65536;

    std::cout << "Diff     = " << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "." << st.wMilliseconds << std::endl;
    return diff;
}

