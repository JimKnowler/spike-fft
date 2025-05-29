#pragma once

#include <vector>

struct ComplexNumber 
{
    float real = 0.0f;
    float imaginary = 0.0f;

    ComplexNumber operator*(const ComplexNumber& other) const;
    ComplexNumber operator+(const ComplexNumber& other) const;
    float magnitude() const;
};

std::vector<ComplexNumber> fft(float* realSignal, int N);
