#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "gamma_correct.h"
#include "image_library.h"
#include "test.h"
#include <getopt.h>
#include <time.h>
#include <math.h>

double gamma_correct_generic(int iterations, void (*function)(uint8_t*, 
    int, int, float, float, float, float, uint8_t*), 
    uint8_t* inputContent, int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);

/**
 * Print a helpful bit of text for the user. Helper Method to main()
*/
void print_help() {
    printf("-----[Help Desk]-----\n\n[OPTIONS:]\n \n");
    printf("-V <int> what implementation to use. If this is not set, will run implementation 0.\n");
    printf("Implementations are :\n0 = asm_hash_simd\n1 = c_hash_sse\n2 = asm_simd\n3 = c_sse\n4 = asm_basic\n5 = c_basic\n6 = c_hash\n7 = asm_hash\n8 = c_library\n\n");
    printf("-B measure execution time. a value > 0 will result in the program running multiple times.\n\n");
    printf("<string> path for the input file. If this is not given, the program terminates. Make sure not to have multiple of these.\n \n");
    printf("-o <string> path for the output file. This has to be a .pgm file. This is a required option.\n \n");
    printf("--coeffs <float>,<float>,<float> used for gray scaling weights (a, b, c). Uses 0.3f, 0.59f, 0.11f as default. All must be > 0.\n \n");
    printf("--gamma <float> the gamma used for gamma correction. \nMust be > 0, else the default is used.\nThis a required option.\n \n");
    printf("-h / --help open the Help Desk.\n \n");
    printf("[USAGE:]\n");
    printf("./main.out -V [0,8] -B [uint] input.ppm -o output.pgm --coeffs [float],[float],[float] --gamma [0, inf)\n");
    printf("[EXAMPLE USAGE:]\n");
    printf("./main.out -V0 -B10 input.ppm -o output.pgm --coeffs 0.3,0.59,0.11 --gamma 2.5\n");
}

void exit_help() {
    printf("\nUse ./main -h|--help for usage.\n\n");
    exit(EXIT_FAILURE);
}

/*
 * Tells you if a given string is a number in its entirety (no extra chars). Helper Method to main()
*/
bool is_string_number(char* string) {
    for (int i = 0; string[i] != '\0'; i++) {
        if (string[i] < '0' || string[i] > '9') {
            return false;
        }
    }
    return true;
}

bool is_string_float(char* string) {
    int dot = 0;
    for (int i = 0; string[i] != '\0'; i++) {
        if (string[i] < '0' || string[i] > '9') {
            if (string[i] == '.' && dot == 0) {
                dot = 1;
            } else {
                return false;
            }
        }
    }
    return true;
}

int main(int argc, char *argv[]) {

    int implementation = 0; //what implementation is used?
    int benchmarking = 0; // is time measured?
    int measureTime = 1; // if so, how many times will the code run?
    char* filename = NULL;
    char* outputfile = NULL;
    float a = 0.3f;
    float b = 0.59f;
    float c = 0.11f;
    float gamma = NAN;

    int opt; //this stores the option you actually get ('g', 'c', 'B' etc.)
    static struct option options_long[] = {
        {"gamma", required_argument, 0, 'g'},
        {"coeffs", required_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'}, //double mapping --help to -h
        {"test", no_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    // parses options and checks vor validity
    while ((opt = getopt_long(argc, argv, "-V:B::tho:g:c:", options_long, NULL)) != -1)
    {
        switch (opt) {
            case 'V':
                implementation = atoi(optarg);
                if(implementation > 8 || implementation < 0 || !is_string_number(optarg)) {
                    fprintf(stderr, "Invalid -V %s. Can only be 0, 1, 2, 3, 4, 5, 6, 7, 8 or unset option. Exiting.\n", optarg);
                    exit_help();
                }
                break;
            case 'B':
                benchmarking = 1;
                int measureTimeTemp = 0;
                if(optarg != NULL) {
                    measureTimeTemp = atoi(optarg);
                    if (!is_string_number(optarg) || measureTimeTemp < 0) {
                        fprintf(stderr, "Invalid -B %s. Has to be positiv number or no argument. Exiting.\n", optarg);
                        exit_help();
                    }
                } else if (optind < argc && is_string_number(argv[optind])) {
                    measureTimeTemp = atoi(argv[optind]);
                    if (measureTimeTemp < 0) {
                        fprintf(stderr, "Invalid -B %s. Has to be positiv number or no argument. Exiting.\n", optarg);
                        exit_help();
                    }
                }
                measureTime += measureTimeTemp;
                break;
            case 'o':
                outputfile = optarg;
                break;
            case 'c':
                float abc[] = {a, b, c};
                const char comma[2] = ",";
                int count = 0;
                char* token = strtok(optarg, comma);
                while(token != NULL)
                {
                    abc[count] = atof(token);
                    if(!is_string_float(token)) {
                        fprintf(stderr, "Invalid --coeffs %s. Has to be 3 not-negativ floats. Their sum must not be 0. Exiting\n", token);
                        exit_help();
                    }
                    token = strtok(NULL, comma);
                    count++;
                }
                if (count != 3 || abc[0] < 0 || abc[1] < 0 || abc[2] < 0 || abc[0] + abc[1] + abc[2] == 0) {
                    fprintf(stderr, "Invalid --coeffs %s. Has to be 3 not-negativ floats. Their sum must not be 0. Exiting\n", optarg);
                    exit_help();
                }
                a = abc[0];
                b = abc[1];
                c = abc[2];
                break;
            case 'g':
                if(is_string_float(optarg)) {
                    gamma = atof(optarg);
                } else {
                    fprintf(stderr, "Invalid --gamma %s. Has to be one not-negativ float. Exiting\n", optarg);
                    exit_help();
                }
                break;
            case 't':
                test();
                exit(EXIT_SUCCESS);
                break;
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
                break;
            case 1:
                if (is_string_number(optarg)) {

                    if (strcmp(argv[optind - 2], "-B") != 0) {
                        fprintf(stderr, "Invalid second positional argument %s. Write -B<x> without Whitespace. Exiting.\n", optarg);
                        exit_help();
                    }
                    break;
                } else if (filename == NULL) {
                    filename = optarg;
                } else {
                    fprintf(stderr, "Invalid second positional argument %s. Write -B<x> without Whitespace. Exiting.\n", optarg);
                    exit_help();
                }
                break;
            default:
                fprintf(stderr, "Invalid option %s. Exiting.\n", argv[optind - 1]);
                exit_help();
                break;
        }
    }

    // check for valid gamma
    if(isnan(gamma) || gamma < 0) {
        fprintf(stderr, "Invalid or unset --gamma. Has to be number in [0, inf). Exiting\n");
        exit_help();
    }
    printf("INFO: Gamma is %f\n", gamma);

    // check for valid input filename ending
    if (filename == NULL || strlen(filename) <= 4 || strcmp(filename + strlen(filename)-4, ".ppm")) {
        fprintf(stderr, "No input file name was given/incorrect input file name or formatting. Quitting.\n");
        exit_help();
    }
    // check for valid output filename ending
    if (outputfile == NULL || strlen(outputfile) <= 4 || strcmp(outputfile + strlen(outputfile)-4, ".pgm")) {
        fprintf(stderr, "No output file name was given/incorrect output file name or formatting. Quitting.\n");
        exit_help();
    }

    // normalize coeffs
    float abc = a + b + c;
    a = a/abc;
    b = b/abc;
    c = c/abc;
    printf("INFO: Normalized coeffs to %f, %f, %f\n", a, b, c);
    printf("INFO: Measuring %d times\n", measureTime);
    printf("INFO: Output file is %s\n", outputfile);
    printf("INFO: Input file is %s\n", filename);
    printf("INFO: Using implementation %d\n", implementation);

    printf("\n");

    // read input file
    imageFile input = {0};
    if(readPPMImage(&input, filename) != 0) {
        return 0;
    }

    // prep output file
    imageFile output = {0};
    output.content = malloc(input.width * input.heigth);
    if(output.content == NULL) {
        fprintf(stderr, "Malloc failed\n");
        exit(EXIT_FAILURE);
    }
    output.width = input.width;
    output.heigth = input.heigth;

    double overallTime = 0.0;
    double averageTime = 0.0;

    // run selected implementation with specified options
    switch (implementation) {
        case 0:
            printf("Using gamma_correct_asm_hash_simd\n");
            overallTime = gamma_correct_generic(measureTime, &gamma_correct_asm_hash_simd, 
            input.content, input.width, input.heigth, a, b, c, gamma, output.content);
            break;
        case 1:
            printf("Using gamma_correct_c_hash_SSE\n");
            overallTime = gamma_correct_generic(measureTime, &gamma_correct_c_hash_SSE, 
            input.content, input.width, input.heigth, a, b, c, gamma, output.content);
            break;
        case 2:
            printf("Using gamma_correct_asm_simd\n");
            overallTime = gamma_correct_generic(measureTime, &gamma_correct_asm_simd, 
            input.content, input.width, input.heigth, a, b, c, gamma, output.content);
            break;
        case 3:
            printf("Using gamma_correct_c_SSE\n");
            overallTime = gamma_correct_generic(measureTime, &gamma_correct_c_SSE, 
            input.content, input.width, input.heigth, a, b, c, gamma, output.content);
            break;
        case 4:
            printf("Using gamma_correct_asm\n");
            overallTime = gamma_correct_generic(measureTime, &gamma_correct_asm,  
            input.content, input.width, input.heigth, a, b, c, gamma, output.content);
            break;
        case 5:
            printf("Using gamma_correct_c\n");
            overallTime = gamma_correct_generic(measureTime, &gamma_correct_c, 
            input.content, input.width, input.heigth, a, b, c, gamma, output.content);
            break;
        case 6:
            printf("Using gamma_correct_asm_hash\n");
            overallTime = gamma_correct_generic(measureTime, &gamma_correct_asm_hash, 
            input.content, input.width, input.heigth, a, b, c, gamma, output.content);
            break;
        case 7:
            printf("Using gamma_correct_c_hash\n");
            overallTime = gamma_correct_generic(measureTime, &gamma_correct_c_hash, 
            input.content, input.width, input.heigth, a, b, c, gamma, output.content);
            break;
        case 8:
            printf("Using gamma_correct_c_naiv\n");
            printf("This uses powf(float, float) from math.h for gamma corection\n");
            overallTime = gamma_correct_generic(measureTime, &gamma_correct_c_naiv, 
            input.content, input.width, input.heigth, a, b, c, gamma, output.content);
            break;
        default:
            fprintf(stderr, "Invalid -V %s. Can only be 0, 1, 2, 3, 4 or unset option. Exiting.\n", optarg);
            freeImageFile(&input);
            freeImageFile(&output);
            exit_help();
            break;
    }

    averageTime = overallTime / measureTime;

    // print out measured time if -B was set
    if (benchmarking == 1) {
        printf("Ran %d times. Took %f seconds with an average of %f seconds.\n", measureTime, overallTime, averageTime);
    }

    // write output to pgm file
    writePGMImage(&output, outputfile);

    // free malloced pointers
    freeImageFile(&input);
    freeImageFile(&output);

    printf("Done doing. Have a nice day : ^)\n");
    exit(EXIT_SUCCESS);
}

// generic function so that we dont have to repeat the same code 5 times
double gamma_correct_generic(int iterations, void (*function)(uint8_t*, 
    int, int, float a, float, float, float, uint8_t*), 
    uint8_t* inputContent, int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent) {

        struct timespec start;
        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int i = 0; i < iterations; i++) {
            (*function)(inputContent, width, height, a, b, c, gamma, outputContent);
        }

        struct timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time = end.tv_sec - start.tv_sec + 1e-9 * (end.tv_nsec - start.tv_nsec);
        return time;
}