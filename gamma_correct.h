#include <stdint.h>
#include <immintrin.h>

//-------------------------------------------------------------------
// NAIVE C FUNCTIONS
//-------------------------------------------------------------------
float calculateLn(float x);
float calculateExponentalFunction(float x);
float power(float a, float b);
float gamma_correct_pixel(float grayscalePixel, float gamma);
float convert_pixel_to_grayscale(int red, int green, int blue, 
    float a, float b, float c);
void gamma_correct_c(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);
void gamma_correct_c_naiv(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);
void gamma_correct_c_hash(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);

//-------------------------------------------------------------------
// ASM FUNCTIONS
//-------------------------------------------------------------------
void gamma_correct_asm(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);
void gamma_correct_asm_simd(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);
void gamma_correct_asm_hash(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);

//gamma_correct_asm_hash_simd IS OUR MAIN IMPLEMENTATION
void gamma_correct_asm_hash_simd(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);

//-------------------------------------------------------------------
// SSE C FUNCTIONS
//-------------------------------------------------------------------
void gamma_correct_c_SSE(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);
void gamma_correct_c_hash_SSE(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);
__m128 gamma_correct_pixel_SSE(__m128 grayscalePixel, float gamma);
__m128 convert_pixel_to_grayscale_SSE(__m128 red, __m128 green, __m128 blue, 
    float a, float b, float c);
__m128 calculateLn_SSE(__m128 x);
__m128 calculateExponentalFunction_SSE(__m128 x);
__m128 power_SSE(__m128 a, __m128 b);