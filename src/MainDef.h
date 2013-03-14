#ifndef MAINDEF_H
#define MAINDEF_H

#define SCE_SQUIGGLE_UNDERLINE_RED INDIC_CONTAINER + 1
#define CLEAN_AND_ZERO(x) do { if (x) {delete x; x = 0; }  } while (0);
#define CLEAN_AND_ZERO_ARR(x) do { if (x) {delete[] x; x = 0; }  } while (0);
#define countof(A) sizeof(A)/sizeof((A)[0])
#define DEFAULT_BUF_SIZE 4096
#define MBS(X) (unsigned char *)(X)
#define PMBS(X) (unsigned char **)(X)

// Menu IDs
#define MID_ADDTODICTIONARY 101
#define MID_IGNOREALL 102

// Custom WMs (Only for our windows and threads)
#define WM_SHOWANDRECREATEMENU WM_USER + 1000
#define TM_MODIFIED_ZONE_INFO  WM_USER + 1001
#define TM_SET_SETTING         WM_USER + 1002
#endif MAINDEF_H
