/*
    This file includes our solutions to reading and writing out image files.
    Header file image_library.h defines a struct to store required information.
*/

#include "image_library.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

int isNewLine(char c);
int skipWhiteSpaces(FILE **fptr);
int parseNumber(FILE **fptr, int *store);
int readNextChar(char *charStore, FILE **fptr);

// PPM PARSING/READ
// This function parses the file named "imageName" and stores in the imageFile struct "result"
int readPPMImage(imageFile* result, char* imageName) {
    FILE *fptr;
    char charRead;
    
    // Try to open the file, return if cannot open
    fptr = fopen(imageName, "rb");
    if(!fptr) {
        fprintf(stderr, "readPPMImage: Could not open file\n");
        return EXIT_FAILURE;
    }

    // Read first char and compare to P
    if(readNextChar(&charRead, &fptr) == EXIT_SUCCESS) {
        if (charRead != 'P') {
            fprintf(stderr, "readPPMImage: Image not in P6 format\n");
            fclose(fptr);
            return EXIT_FAILURE;
        }
    } 
    else {
        fprintf(stderr, "readPPMImage: Could not read first character of magic number\n");
        fclose(fptr);
        return EXIT_FAILURE;
    }

    // Read second char and compare to 6
    if(readNextChar(&charRead, &fptr) == EXIT_SUCCESS) {
        if (charRead != '6') {
            fprintf(stderr, "readPPMImage: Image not in P6 format\n");
            fclose(fptr);
            return EXIT_FAILURE;
        }
    }
    else {
        fprintf(stderr, "readPPMImage: Could not read second character of magic number\n");
        fclose(fptr);
        return EXIT_FAILURE;
    }

    // Skip whitespaces after P6
    if(skipWhiteSpaces(&fptr)) {
        fprintf(stderr, "readPPMImage: Could not read after P6\n");
        fclose(fptr);
        return EXIT_FAILURE;
    }

    // Read width
    if(parseNumber(&fptr, &(result->width))) {
        fprintf(stderr, "readPPMImage: Could not read width\n");
        fclose(fptr);
        return EXIT_FAILURE;
    }

    // Skip whitespaces after width;
    if(skipWhiteSpaces(&fptr)) {
        fprintf(stderr, "readPPMImage: Could not read after width\n");
        fclose(fptr);
        return EXIT_FAILURE;
    }

    // Read heigth
    if(parseNumber(&fptr, &(result->heigth))) {
        fprintf(stderr, "readPPMImage: Could not read heigth\n");
        fclose(fptr);
        return EXIT_FAILURE;
    }

    // Skip whitespaces after height;
    if(skipWhiteSpaces(&fptr)) {
        fprintf(stderr, "readPPMImage: Could not read after heigth\n");
        fclose(fptr);
        return EXIT_FAILURE;
    }

    // Read max value;
    int maxVal = 0;
    if(parseNumber(&fptr, &maxVal)) {
        fprintf(stderr, "readPPMImage: Could not read max value\n");
        fclose(fptr);
        return EXIT_FAILURE;
    }

    // Return if max value is not 255
    if(maxVal != 255) {
        fprintf(stderr, "readPPMImage: Max value is %d (needs to be 255)\n", maxVal);
        fclose(fptr);
        return EXIT_FAILURE;
    }

    // Read last whitespace character
    if(readNextChar(&charRead, &fptr) == EXIT_SUCCESS) {
        if (!isspace(charRead)) {
            fprintf(stderr, "readPPMImage: Last character is not a whitespace\n");
            fclose(fptr);
            return EXIT_FAILURE;
        }
    }
    else {
            fprintf(stderr, "readPPMImage: Could not read last whitespace\n");
            fclose(fptr);
            return EXIT_FAILURE;
    }

    // Allocate memory space for the content
    result->content = malloc(result->width * result->heigth * 3);
        if(!result->content) {
            fprintf(stderr, "readPPMImage: Malloc failed\n");
            fclose(fptr);
            return EXIT_FAILURE;
        }

    // Read data into allocated content
    int bytesRead = fread(result->content, sizeof(char), result->width * result->heigth * 3, fptr);

    // In case data read is smaller than defined in the header, return
    if(bytesRead < (result->width * result->heigth * 3)) {
        fprintf(stderr, "readPPMImage: Content smaller than defined\n");
        fclose(fptr);
        return EXIT_FAILURE;
    }

    // In case data read is larger than defined in the header, return
    if(fgetc(fptr) != EOF) {
        fprintf(stderr, "readPPMImage: Content larger than defined\n");
        fclose(fptr);
        return EXIT_FAILURE;
    }

    printf("readPPMImage: Data read successfull, bytes read: %d\n", bytesRead);

    fclose(fptr);
    return EXIT_SUCCESS;
}

// PGM OUTPUT/WRITE
// This writes out the PGM image stored in imageFile struct "output" with the name "outputName"
int writePGMImage(imageFile* output, char* outputName) {
    FILE *fptr;
    char buffer[1024];
    char firstLine[] = "P5\n";
    char thirdLine[] = "255\n";

    fptr = fopen(outputName, "wb");
    if(!fptr) {
        fprintf(stderr, "writePGMImage: Could not open file\n");
        return EXIT_FAILURE;
    }

    // Write magic number
    fwrite(firstLine, sizeof(char), strlen(firstLine) , fptr);

    // Write width height information
    sprintf(buffer, "%d %d\n", output->width, output->heigth);
    fwrite(buffer, sizeof(char) , strlen(buffer), fptr);

    // Write max value which is 255
    fwrite(thirdLine, sizeof(char), strlen(thirdLine) , fptr);

    // Write content stored in output
    fwrite(output->content, sizeof(char) , output->width * output->heigth, fptr);

    fclose(fptr);
    return EXIT_SUCCESS;
}

// Returns true if c is a newline character (CR or LF)
int isNewLine(char c) {
    return (c == 10) || (c == 13);
}

// Reads the next character in the file while skipping all comments starting with "#"
int readNextChar(char *charStore, FILE **fptr) {
    char temp[2];

    while(fread(temp, sizeof(char), 1, *fptr)) {    
        if((temp[0] == '#')) {
            while(!isNewLine(temp[0])) {
                if(!fread(temp, sizeof(char), 1, *fptr)) {
                    fclose(*fptr);
                    return EXIT_FAILURE;
                }
            }
        }
        else {
            *charStore = temp[0];
            return EXIT_SUCCESS;
        }   
    }

    return EXIT_FAILURE;
}

// Helper function: skips all connected whitespaces in the header
int skipWhiteSpaces(FILE **fptr) {
    char charRead;

    if(readNextChar(&charRead, fptr) == EXIT_SUCCESS) {
        while(isspace(charRead)) {
            if(readNextChar(&charRead, fptr) == EXIT_FAILURE) {
                fclose(*fptr);
                return EXIT_FAILURE;
            }
        }
        ungetc(charRead, *fptr);
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

// Helper function: parses a positive number in the header
int parseNumber(FILE **fptr, int *store) {
    char charRead;
    int result = 0;
    while(readNextChar(&charRead, fptr) == EXIT_SUCCESS) {  
        if(!isdigit(charRead) && result == 0) return EXIT_FAILURE;
        if(isdigit(charRead)) {
            if(result < (INT_MAX / 10))
                result *= 10;
            else
                return EXIT_FAILURE;
            result += charRead - '0';
        }
        else {
            ungetc(charRead, *fptr);
            *store = result;
            return EXIT_SUCCESS;
        }
    }
    return EXIT_FAILURE;
}

// Frees content if it is allocated
void freeImageFile(imageFile* imageFile) {
    if(imageFile->content != NULL)
        free(imageFile->content);
}