/*
    This file includes all our C implementations to solve the defined problem.
    Header file gamma_correct.h defines all of assembly and C implementations.
    "gamma_correct_asm_hash_simd" is the main implementation used for this problem.
*/

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "gamma_correct.h"

//-------------------------------------------------------------------
// START NAIVE CODE
//-------------------------------------------------------------------

// Gamma correction using the naive C Implementation
void gamma_correct_c(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent) {
        // 1: Read and convert each pixel to greyscale using coefficients
        // 2: Apply gamma correction to each pixel using gamma
        // 3: Write out the result
        for(int i = 0; i < width * height * 3; i += 3)
            *(outputContent + i/3) = gamma_correct_pixel(
                convert_pixel_to_grayscale(
                    *(inputContent + i), 
                    *(inputContent + i + 1), 
                    *(inputContent + i + 2), a, b, c), gamma);
}

// Gamma correction of a greyscale pixel
float gamma_correct_pixel(float grayscalePixel, float gamma) {
    return power(grayscalePixel/255.0f, gamma) * 255.0f;   
}

// Convert a pixel to greyscale with normalized a,b,c coefficients
float convert_pixel_to_grayscale(int red, int green, int blue, 
    float a, float b, float c) {
    return (red*a + green*b + blue*c);
}

// Approximate ln(x) for 0 <= x <= 1 using Taylor series
float calculateLn(float x) {
    x = 1.0 - x;

    float sum = 0.0;
    float upper = 1.0;

    for(int i = 1; i < 15; i++) {
        upper *= x;
        sum -= upper/i;
    }

    return sum;
}

// Approximate e^x using Taylor series
float calculateExponentalFunction(float x) {
    float sum = 1;
    float term = 1;

    for(int i = 1; i < 18; i++) {
        term *= x;
        term /= i;
        sum += term;
    }

    if(sum < 0)
        return 0;

    return sum;
}

// Approximate a^b using Taylor series
float power(float a, float b) {
    return calculateExponentalFunction(b * calculateLn(a));
}

//-------------------------------------------------------------------
// START SSE CODE
//-------------------------------------------------------------------

// Gamma correction using the C SSE Implementation
void gamma_correct_c_SSE(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent) {

        float toWrite[4] = {0, 0, 0, 0}; // Buffer to write out result
        int leftOver = (width * height * 3) % 12; // How many bytes will naive implementation handle
        int pixelsToExecute = (width * height * 3) - leftOver; // How many bytes will SSE handle
        
        for(int i = 0; i < pixelsToExecute; i += 12) {
            // 1: Read RGB values of 4 pixels into 3 vectors
            float red1 = *(inputContent + i + 0);
            float green1 = *(inputContent + i + 1);
            float blue1 = *(inputContent + i + 2);

            float red2 = *(inputContent + i + 3);
            float green2 = *(inputContent + i + 4);
            float blue2 = *(inputContent + i + 5);

            float red3 = *(inputContent + i + 6);
            float green3 = *(inputContent + i + 7);
            float blue3 = *(inputContent + i + 8);

            float red4 = *(inputContent + i + 9);
            float green4 = *(inputContent + i + 10);
            float blue4 = *(inputContent + i + 11);

            __m128 redVector = _mm_set_ps(red1, red2, red3, red4);
            __m128 blueVector = _mm_set_ps(blue1, blue2, blue3, blue4);
            __m128 greenVector = _mm_set_ps(green1, green2, green3, green4);

            // 2: Convert pixel vectors to greyscale using coefficients
            // 3: Apply gamma correction to pixel vectors using gamma
            __m128 result = gamma_correct_pixel_SSE(
                convert_pixel_to_grayscale_SSE(redVector, greenVector, blueVector, a, b, c), gamma);

            // 4: Write out the result
            _mm_storeu_ps(toWrite, result);
            *(outputContent + i/3 + 0) = toWrite[3];
            *(outputContent + i/3 + 1) = toWrite[2];
            *(outputContent + i/3 + 2) = toWrite[1];
            *(outputContent + i/3 + 3) = toWrite[0];
        }

        //USE NORMAL IMPLEMENTATION FOR LEFTOVERS
        gamma_correct_c(inputContent + pixelsToExecute, leftOver, 1, 
            a, b, c, gamma, 
            outputContent + pixelsToExecute / 3);
}

// Gamma correction of a greyscale pixel vector
__m128 gamma_correct_pixel_SSE(__m128 grayscalePixel, float gamma) {
    float maxVal = 255.0f;
    __m128 vector255 = _mm_load1_ps(&maxVal);
    __m128 gammaVector = _mm_load1_ps(&gamma);
    //return power(grayscalePixel/255.0f, gamma) * 255.0f;
    return _mm_mul_ps(power_SSE(_mm_div_ps(grayscalePixel, vector255), gammaVector), vector255);
}

// Convert a pixel vector to greyscale vector with normalized a,b,c coefficients
__m128 convert_pixel_to_grayscale_SSE(__m128 red, __m128 green, __m128 blue, 
    float a, float b, float c) {
        
    //return (red*a + green*b + blue*c);
    red = _mm_mul_ps(red, _mm_load1_ps(&a));
    green = _mm_mul_ps(green, _mm_load1_ps(&b));
    blue = _mm_mul_ps(blue, _mm_load1_ps(&c));
    return _mm_add_ps(_mm_add_ps(red, green), blue);
}

// Approximate ln(x) for 0 <= x[n] <= 1
__m128 calculateLn_SSE(__m128 x) {
    float zero = 0.0;
    float one = 1.0;

    // x = 1.0 - x;
    x = _mm_sub_ps(_mm_load1_ps(&one), x);
    
    //float sum = 0.0;
    __m128 sum = _mm_load1_ps(&zero);

    //float upper = 1.0;
    __m128 upper = _mm_load1_ps(&one);

    /**for(int i = 1; i < 30; i++) {
        upper *= x;
        sum -= upper/i;
    }*/
    for(int i = 1; i < 15; i++) {
        float iFloat = i;
        upper = _mm_mul_ps(upper, x);
        sum = _mm_sub_ps(sum, _mm_div_ps(upper, _mm_load1_ps(&iFloat)));
    }

    return sum;
}

// Approximate e^x[n]
__m128 calculateExponentalFunction_SSE(__m128 x) {
    float one = 1.0;

    //float sum = 1;
    __m128 sum = _mm_load1_ps(&one);

    //float term = 1;
    __m128 term = _mm_load1_ps(&one);

    /**for(int i = 1; i < 30; i++) {
        term *= x;
        term /= i;
        sum += term;
    }*/

    for(int i = 1; i < 18; i++) {
        float iFloat = i;
        term = _mm_mul_ps(term, x);
        term = _mm_div_ps(term, _mm_load1_ps(&iFloat));
        sum =  _mm_add_ps(sum, term);
    }

    return _mm_max_ps (sum, _mm_set_ps(0, 0, 0, 0));
}

// Approximate a[n]^b[n]
__m128 power_SSE(__m128 a, __m128 b) {
    // return calculateExponentalFunction(b * calculateLn(a));
    return calculateExponentalFunction_SSE(_mm_mul_ps(b, calculateLn_SSE(a)));
}

//-------------------------------------------------------------------
// START C LIBRARY CODE
//-------------------------------------------------------------------

// Gamma correction using the libary powf function
// NOTE: This is included just to compare it with our own implementation
void gamma_correct_c_naiv(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent) {
        for (int i = 0; i < width * height * 3; i += 3) {
            *(outputContent + i/3) = powf(
                convert_pixel_to_grayscale(
                    *(inputContent + i), 
                    *(inputContent + i + 1), 
                    *(inputContent + i + 2), a, b, c)  / 255.0f, gamma) * 255.0f;
        }
}

//-------------------------------------------------------------------
// START HASH CODE
//-------------------------------------------------------------------

// Gamma correction using the hash C Implementation
void gamma_correct_c_hash(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent) {
        // hash table for storing results
        uint8_t hash[256] = {0};
        
        uint8_t tempKey = 0;

        // gamma correct all possible values
        for (int i = 0; i < 256; i++) 
        {
            hash[i] = gamma_correct_pixel(i, gamma);
        }

        for (int i = 0; i < width * height * 3; i += 3) {
            // convert pixel to grayscale
            tempKey = convert_pixel_to_grayscale(
                    *(inputContent + i), 
                    *(inputContent + i + 1), 
                    *(inputContent + i + 2), a, b, c);
            // use grayscale value as key for the hashtable
            *(outputContent + i/3) = hash[tempKey];
        }
}

// Gamma correction using the hash C SSE Implementation
void gamma_correct_c_hash_SSE(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent) {
        // hash table for storing results
        uint8_t tempKey;
        uint8_t hash[256] = {0};
        float toWrite[4] = {0, 0, 0, 0};

        // gamma correct all possible values
        for (int i = 0; i < 256; i+=4) 
        {
            _mm_storeu_ps(toWrite, gamma_correct_pixel_SSE(_mm_set_ps(i, i+1, i+2, i+3), gamma));
            hash[i] = toWrite[3];
            hash[i+1] = toWrite[2];
            hash[i+2] = toWrite[1];
            hash[i+3] = toWrite[0];
        }

        for (int i = 0; i < width * height * 3; i += 3) {
            // convert pixel to grayscale
            tempKey = convert_pixel_to_grayscale(
                    *(inputContent + i), 
                    *(inputContent + i + 1), 
                    *(inputContent + i + 2), a, b, c);
            // use grayscale value as key for the hashtable
            *(outputContent + i/3) = hash[tempKey];
        }
}