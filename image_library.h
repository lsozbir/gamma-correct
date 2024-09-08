#include <stdint.h>
// Defines a struct which holds essentials of an image file
typedef struct imageFile {
  unsigned int width;
  unsigned int heigth;
  uint8_t* content;
}imageFile;

int readPPMImage(imageFile* imageFile, char* imageName);
int writePGMImage(imageFile* imageName, char* outputName);
void freeImageFile(imageFile* imageFile);