#ifndef MAINDEF_H
#define MAINDEF_H

#define SCE_SQUIGGLE_UNDERLINE_RED INDIC_CONTAINER + 1
#define CLEAN_AND_ZERO(x) do { if (x) {delete x; x = 0; }  } while (0);
#define CLEAN_AND_ZERO_ARR(x) do { if (x) {delete[] x; x = 0; }  } while (0);
#define countof(A) sizeof(A)/sizeof((A)[0])
#define DEFAULT_BUF_SIZE 4096

#endif MAINDEF_H
