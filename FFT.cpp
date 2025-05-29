#include "FFT.h"

#include <math.h>
#include <raylib.h>

float SignalSin(int n, int f, int N)
{
    return sin(2.0f * PI * float(f) * float(n) / float(N));
}

float SignalCos(int n, int f, int N)
{
    return cos(2.0 * PI * float(f) * float(n) / float(N));
}

ComplexNumber ComplexNumber::operator*(const ComplexNumber& other) const 
{
    ComplexNumber result;

    result.real = (real * other.real) - (imaginary * other.imaginary);
    result.imaginary = (real * other.imaginary) + (imaginary * other.real);

    return result;
}

ComplexNumber ComplexNumber::operator+(const ComplexNumber& other) const
{
    ComplexNumber result;

    result.real = real + other.real;
    result.imaginary = imaginary + other.imaginary;

    return result;
}

float ComplexNumber::magnitude() const 
{
    return sqrtf(powf(real, 2) + powf(imaginary, 2));
}

std::vector<ComplexNumber> RecursiveFFT(std::vector<ComplexNumber> signal)
{
    const size_t N = signal.size();

    if (N == 1)
    {
        return signal;
    }

    size_t half_N = N / 2;
    
    std::vector<ComplexNumber> signalEven(half_N);
    std::vector<ComplexNumber> signalOdd(half_N);

    for (size_t i=0; i<half_N; i++)
    {
        signalEven[i] = signal[2 * i];
        signalOdd[i] = signal[(2 * i) + 1];
    }

    std::vector<ComplexNumber> frequenciesEven = RecursiveFFT(signalEven);
    std::vector<ComplexNumber> frequenciesOdd = RecursiveFFT(signalOdd);

    std::vector<ComplexNumber> frequencies(N);

    for (size_t k = 0; k < N; k++)
    {
        // given periodicity in W in the half_N sub regions
        size_t mod_k = k % half_N;

        ComplexNumber even = frequenciesEven[mod_k];
        ComplexNumber odd = frequenciesOdd[mod_k];
        
        // TODO: take advantage of complex conjugate symmetry [ pow(W, N-n) = pow(W, -n) ]
        float theta = 2 * PI * float(k) / float(N);
        ComplexNumber w = {
            .real = cos(theta),
            .imaginary = -sin(theta)
        };

        // butterfly operator
        ComplexNumber result = even + (odd * w);

        frequencies[k] = result;
    }
    
    return frequencies;
}

// calculate Hann window weight for ith sample in buffer of L samples
// https://en.wikipedia.org/wiki/Hann_function
float Hann(int n, int N) 
{
    float invN = 1.0f / float(N);
    
    float result = powf(sinf(PI * float(n) * invN), 2);

    return result;
}

std::vector<ComplexNumber> fft(float* realSignal, int N) 
{
    std::vector<ComplexNumber> complexSignal(N);
    for (int i=0; i<N; i++) 
    {
        complexSignal[i].real = realSignal[i] * Hann(i, N);
    }

    std::vector<ComplexNumber> frequencies = RecursiveFFT(complexSignal);
    
    return frequencies;
}
