/*
    This file holds automated tests to catch errors and debug our program.
    Header file test.h defines the function test so it can be called in main.
*/

#include "test.h"
#include "image_library.h"
#include "gamma_correct.h"
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <immintrin.h>

#define NTSC_A 0.3f
#define NTSC_B 0.59f
#define NTSC_C 0.11f
#define GAMMA 0.1f
#define FUNC gamma_correct_c

int genericInvalidTestCase(int testCaseNumber, char *inputName,
        int *tTests, int *sTests, int *fTests);
int genericValidTestCase(int testCaseNumber, char *inputName,
        void (*function)
        (uint8_t*, int, int, float a, float, float, float, uint8_t*),
        int *tTests, int *sTests, int *fTests);

void test() {

    void (*functionToUse)(uint8_t*, int, int, float a, float, float, 
        float, uint8_t*) = FUNC;

    int totalTests = 0;
    int successfulTests = 0;
    int failedTests = 0;
    int result;

    //INVALID TEST CASES
    genericInvalidTestCase(1, "Inputs/Invalid/ppm_junk_at_end.ppm", 
        &totalTests, &successfulTests, &failedTests);

    genericInvalidTestCase(2, "Inputs/Invalid/ppm_no_magic_number.ppm", 
        &totalTests, &successfulTests, &failedTests);

    genericInvalidTestCase(3, "Inputs/Invalid/ppm_no_maxval.ppm", 
        &totalTests, &successfulTests, &failedTests);

    genericInvalidTestCase(4, "Inputs/Invalid/ppm_no_width_height.ppm", 
        &totalTests, &successfulTests, &failedTests);

    genericInvalidTestCase(5, "Inputs/Invalid/ppm_nothing.ppm", 
        &totalTests, &successfulTests, &failedTests);

    genericInvalidTestCase(6, "Inputs/Invalid/ppm_large_numbers.ppm", 
        &totalTests, &successfulTests, &failedTests);
    
    genericInvalidTestCase(7, "Inputs/Invalid/THIS_FILE_DOESNT_EXIST.ppm", 
        &totalTests, &successfulTests, &failedTests);

    genericInvalidTestCase(8, "Inputs/Invalid/sus.png", 
        &totalTests, &successfulTests, &failedTests);

    //VALID TEST CASES
    genericValidTestCase(1, "Inputs/Valid/input1_1920x1372.ppm", functionToUse,
        &totalTests, &successfulTests, &failedTests);

    genericValidTestCase(2, "Inputs/Valid/input2_4x4_lots_of_comments.ppm", functionToUse,
        &totalTests, &successfulTests, &failedTests);
    
    genericValidTestCase(3, "Inputs/Valid/input3_25x24.ppm", functionToUse,
        &totalTests, &successfulTests, &failedTests);

    genericValidTestCase(4, "Inputs/Valid/input4_1x1.ppm", functionToUse,
        &totalTests, &successfulTests, &failedTests);

    genericValidTestCase(5, "Inputs/Valid/input5_2x2.ppm", functionToUse,
        &totalTests, &successfulTests, &failedTests);

    genericValidTestCase(6, "Inputs/Valid/input6_3x3.ppm", functionToUse,
        &totalTests, &successfulTests, &failedTests);

    genericValidTestCase(7, "Inputs/Valid/input7_1x33.ppm", functionToUse,
        &totalTests, &successfulTests, &failedTests);

    genericValidTestCase(8, "Inputs/Valid/input8_33x1.ppm", functionToUse,
        &totalTests, &successfulTests, &failedTests);

    printf("Ran %d tests\n", totalTests);
    printf("Successful tests: %d\n", successfulTests);
    printf("Failed tests: %d\n", failedTests);
}

int genericInvalidTestCase(int testCaseNumber, char *inputName,
        int *tTests, int *sTests, int *fTests) {
    (*tTests)++;
    imageFile input = {0};
    if(readPPMImage(&input, inputName) == 0) {
        printf("invalidTestCase%d failed.\n", testCaseNumber);
        (*fTests)++;
        return 1;
    }
    (*sTests)++;
    return 0;
}

int genericValidTestCase(int testCaseNumber, char *inputName,
        void (*function)
        (uint8_t*, int, int, float a, float, float, float, uint8_t*),
        int *tTests, int *sTests, int *fTests) {
    (*tTests)++;
    imageFile input = {0};
    imageFile output = {0};
    if(readPPMImage(&input, inputName) != 0) {
        printf("validTestCase%d failed.\n", testCaseNumber);
        (*fTests)++;
        return 1;
    }
    else {
        output.content = malloc(input.width * input.heigth);
        output.width = input.width;
        output.heigth = input.heigth;
        function(input.content, input.width, input.heigth, NTSC_A, NTSC_B, NTSC_C, GAMMA, output.content);
        char outputName[50];
        snprintf(outputName, 50, "Outputs/output%d_%.2f_%.2f_%.2f_%.2f.pgm", testCaseNumber, NTSC_A, NTSC_B, NTSC_C, GAMMA);
        writePGMImage(&output, outputName);
    }
    freeImageFile(&input);
    freeImageFile(&output);
    (*sTests)++;
    return 0;
}
