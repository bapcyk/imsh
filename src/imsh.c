#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <siod.h>
#include <freeimage.h>

#ifdef __GNUC__
#define MIN(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _b : _a; })
#define MAX(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
#else
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#define sizeofarray(A) (sizeof(A)/sizeof(A[0]))

// TODO check all return values of FreeImage_*()

typedef FIBITMAP IMG;
typedef struct pt2_t pt2_t;
typedef struct dithpt_t dithpt_t;

/// Point coordinates in 2D
struct pt2_t {
    int x;
    int y;
};

/// Dithering point coefficient (for dithering matrix)
struct dithpt_t {
    int x, y;
    float k;
};

/// Type of IMG Lisp object
long tc_img = 0;

/// Some functions' declarations
static IMG* LISP2IMG(LISP ptr);
static void polygon_coords(unsigned int n, int cx, int cy, unsigned int r, int angle, pt2_t *coords);

static void init_imsh_version(void) {
    setvar(cintern("*imsh-version*"),
            cintern("$Id: imsh.c,v 1.0 2013/08/26 py-dev $"),
            NIL);
}

int random_range(int a, int b)
{
    time_t sec;
    time(&sec);
    srand((unsigned int)sec);
    return (rand() % (b - a + 1) + a);
}

// Image processing algorithms {{{
// ---------------------------------------------------------------------------

#define RGB(R, G, B) (RGBQUAD){(uint8_t)R, (uint8_t)G, (uint8_t)B}
#define RGB1(X) (RGBQUAD){(uint8_t)X, (uint8_t)X, (uint8_t)X}

/// Length of RGB vector
#define RGBLEN(RGBQ) sqrt( \
        + ((RGBQ)->rgbRed)*((RGBQ)->rgbRed) \
        + ((RGBQ)->rgbGreen)*((RGBQ)->rgbGreen) \
        + ((RGBQ)->rgbBlue)*((RGBQ)->rgbBlue))

/// Initialize RGB with colors components
#define RGBINIT(RGBQ, R, G, B) do { \
    (RGBQ)->rgbRed = (R); \
    (RGBQ)->rgbGreen = (G); \
    (RGBQ)->rgbBlue = (B); \
} while (0)

/// Initialize RGB with 255-colors components
#define RGBNEG256(RGBQ) do { \
    (RGBQ)->rgbRed = 255 - (RGBQ)->rgbRed; \
    (RGBQ)->rgbGreen = 255 - (RGBQ)->rgbGreen; \
    (RGBQ)->rgbBlue = 255 - (RGBQ)->rgbBlue; \
} while (0)

/// Selectors
#define RGB_R(RGBQ) ((RGBQ)->rgbRed)
#define RGB_G(RGBQ) ((RGBQ)->rgbGreen)
#define RGB_B(RGBQ) ((RGBQ)->rgbBlue)

/// Operations on RGB
#define __MUL(X, Y) ((X) * (Y))
#define __DIV(X, Y) ((X) / (Y))
#define __ADD(X, Y) ((X) + (Y))
#define __SUB(X, Y) ((X) - (Y))

#define RGBCOP(OP, RGBQ, X) \
    (RGBQUAD) { OP(RGB_R(RGBQ), X), \
        OP(RGB_G(RGBQ), X), \
        OP(RGB_B(RGBQ), X) }

#define RGBOP(OP, RGBQ1, RGBQ2) \
    (RGBQUAD) { OP(RGB_R(RGBQ1), RGB_R(RGBQ2)), \
        OP(RGB_G(RGBQ1), RGB_G(RGBQ2)), \
        OP(RGB_B(RGBQ1), RGB_B(RGBQ2)) }

const RGBQUAD RGB_WHITE = (const RGBQUAD){.rgbRed=255, .rgbGreen=255, .rgbBlue=255};
const RGBQUAD RGB_BLACK = (const RGBQUAD){.rgbRed=0, .rgbGreen=0, .rgbBlue=0};

static inline int brightness(RGBQUAD *rgb)
{
    /*int max = 0, min = 255, i, a[] = {RGB_R(rgb), RGB_G(rgb), RGB_G(rgb)};
    for (i=0; i<3; i++) {
        if (a[i] > max) max = a[i];
        if (a[i] < min) min = a[i];
    }
    return (max + min)/2;*/
    return ((RGB_R(rgb) + RGB_G(rgb) + RGB_B(rgb))/3);
}

/** Calculates coordinates of each node of regular polygon
*/
static void polygon_coords(unsigned int n, int cx, int cy, unsigned int r,
        int angle, pt2_t *coords)
{
    int i;
    for (i=0; i<n-1; i++) {
        coords[i].x = cx + r*cos(angle + 2*M_PI*i/n);
        coords[i].y = cy + r*sin(angle + 2*M_PI*i/n);
    }
}

/** Convert image to black-white
 * @param threshold the threshold of brightness in percents
 * @return 0 on success
 */
int algo_bw(IMG *img, int threshold)
{
    RGBQUAD rgb;
    unsigned int row, col, width, height;
    double rgbl, maxrgbl;

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height);

    maxrgbl = sqrt(255.*255. + 255.*255. + 255.*255.);

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            FreeImage_GetPixelColor(img, col, row, &rgb);
            rgbl = RGBLEN(&rgb);
            if (rgbl*100/maxrgbl > threshold) {
                // light
                FreeImage_SetPixelColor(img, col, row, (RGBQUAD*)&RGB_WHITE);
            } else {
                // dark
                FreeImage_SetPixelColor(img, col, row, (RGBQUAD*)&RGB_BLACK);
            }
        }
    }
    return (0);
}

/** grayscale image
 * @return 0 on success
 */
int algo_gr(IMG *img)
{
    RGBQUAD rgb;
    unsigned int row, col, width, height;
    double rgbl;
    uint8_t c;

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height);

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            FreeImage_GetPixelColor(img, col, row, &rgb);
            rgbl = RGBLEN(&rgb);
            c = rgbl/sqrt(3.);
            RGBINIT(&rgb, c, c, c);
            FreeImage_SetPixelColor(img, col, row, &rgb);
        }
    }
    return (0);
}

/** Negative
*/
int algo_neg(IMG *img)
{
    RGBQUAD rgb;
    unsigned int row, col, width, height;

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height);

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            FreeImage_GetPixelColor(img, col, row, &rgb);
            RGBNEG256(&rgb);
            FreeImage_SetPixelColor(img, col, row, &rgb);
        }
    }
    return (0);
}

/** Apply conv. matrix with size sideXside, use division, shifting, normalization.
*/
int algo_conv(IMG *img, int *matrix, int side, int div, int shift)
{
    RGBQUAD rgb;
    unsigned int row, col, i, j, width, height, cx, cy;
    uint8_t *res[3] = {NULL}; // Tmp working copies of bitmap for 3 chans
    int r1, g1, b1, margin, mi;

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height && div && matrix && side>2 && side%2);

    margin = side/2; // process central area, without margin
    // allocate tmp copies of channels
    for (i=0; i<3; i++) {
        res[i] = malloc(width*height*sizeof (uint8_t));
        if (!res[i]) {
            while (i >= 0) free(res[i--]);
            return (1);
        }
    }
    // processing
    for (row = margin; row < height-margin; row++) {
        for (col = margin; col < width-margin; col++) {
            r1 = g1 = b1 = 0;
            for (j = 0; j < side; j++) {
                cy = row - margin + j;
                for (i = 0; i < side; i++) {
                    cx = col - margin + i;
                    FreeImage_GetPixelColor(img, cx, cy, &rgb);
                    mi = j*side + i;
                    r1 += RGB_R(&rgb) * matrix[mi];
                    g1 += RGB_G(&rgb) * matrix[mi];
                    b1 += RGB_B(&rgb) * matrix[mi];
                }
            }
#define __NORM_RGB(C) (C)=shift+(C)/div; (C)=(C)<0? 0 : (C)>255? 255 : (C)
            __NORM_RGB(r1);
            __NORM_RGB(g1);
            __NORM_RGB(b1);
#undef __NORM_RGB
            mi = row*width + col;
            *(res[0] + mi) = (uint8_t)r1;
            *(res[1] + mi) = (uint8_t)g1;
            *(res[2] + mi) = (uint8_t)b1;
        }
    }
    // copy to original img data from tmp copies, margin is unchanged
    for (row = margin; row < height-margin; row++) {
        for (col = margin; col < width-margin; col++) {
            RGBINIT(&rgb,
                    *(res[0] + row*width + col),
                    *(res[1] + row*width + col),
                    *(res[2] + row*width + col));
            FreeImage_SetPixelColor(img, col, row, &rgb);
        }
    }
    // free tmp copies
    for (i=0; i<3; i++) free(res[i]);
    return (0);
}

/** Select only channels with bit-mask: chans is -----RGB byte,
 * so 7 select 3 chans
 */
int algo_ch(IMG *img, int chans)
{
    RGBQUAD rgb;
    unsigned int row, col, width, height;
    uint32_t mask;
    uint32_t rgb32;
    uint8_t r, g, b;

#define _R_ RGB_R(&rgb)
#define _G_ RGB_G(&rgb)
#define _B_ RGB_B(&rgb)

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height);

    // 00RRGGBB, so 1 is chans bits are '...RGB'
    mask = ((chans & 4)? 0xFF0000 : 0)
        | ((chans & 2)? 0xFF00 : 0)
        | ((chans & 1)? 0xFF : 0);
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            FreeImage_GetPixelColor(img, col, row, &rgb);
            rgb32 = mask & ((_R_<<16)|(_G_<<8)|_B_);
            _R_ = (rgb32 & 0xFF0000)>>16;
            _G_ = (rgb32 & 0xFF00)>>8;
            _B_ = rgb32 & 0xFF;
            FreeImage_SetPixelColor(img, col, row, &rgb);
        }
    }
    return (0);

#undef _R_
#undef _G_
#undef _B_
}

/** Convert image to gray levels
 * @param nlevels is number of levels (gradation)
 */
int algo_grl(IMG *img, int nlevels)
{
    RGBQUAD rgb;
    unsigned int row, col, width, height,
                 rgbl, maxrgbl, band;
    uint8_t c;

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height && nlevels>=2);

    maxrgbl = sqrt(255.*255. + 255.*255. + 255.*255.);
    band = maxrgbl/nlevels;

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            FreeImage_GetPixelColor(img, col, row, &rgb);
            rgbl = (int)RGBLEN(&rgb);
            rgbl = band*(rgbl/band);
            c = rgbl/sqrt(3.);
            RGBINIT(&rgb, c, c, c);
            FreeImage_SetPixelColor(img, col, row, &rgb);
        }
    }
    return (0);
}

/** Pixalization of image.
 * @param side is side of square-pixel
 * @param color is 1 or 0 to generate color or black-and-white "pixels"
 * @return 0 on success
 */
int algo_pix(IMG *img, int side, int color)
{
    RGBQUAD rgb;
    unsigned int row, col, irow, icol, width, height;
    unsigned long rs, gs, bs, pixarea, maxrgbl_2;

#define _R_ RGB_R(&rgb)
#define _G_ RGB_G(&rgb)
#define _B_ RGB_B(&rgb)

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height && side>0);

    pixarea = side*side;
    if (!color) maxrgbl_2 = sqrt(255.*255. + 255.*255. + 255.*255.)/2;
    for (row = 0; row < height; row += side) {
        for (col = 0; col < width; col += side) {
            rs = gs = bs = 0;
            for (irow = row; irow < row+side; irow++) {
                for (icol = col; icol < col+side; icol++) {
                    FreeImage_GetPixelColor(img, icol, irow, &rgb);
                    rs += _R_;
                    gs += _G_;
                    bs += _B_;
                }
            }
            _R_ = rs / pixarea;
            _G_ = gs / pixarea;
            _B_ = bs / pixarea;
            if (!color) {
                if (RGBLEN(&rgb) > maxrgbl_2) {
                    rgb = (RGBQUAD)RGB_WHITE;
                } else {
                    rgb = (RGBQUAD)RGB_BLACK;
                }

            }
            for (irow = row; irow < row+side; irow++) {
                for (icol = col; icol < col+side; icol++) {
                    FreeImage_SetPixelColor(img, icol, irow, &rgb);
                }
            }
        }
    }
    return (0);

#undef _R_
#undef _G_
#undef _B_
}

static int _median_cmpf(void const *x, void const *y)
{
    return (*(uint8_t*)x - *(uint8_t*)y);
}

/** Median filter with specified side
*/
int algo_med(IMG *img, int side)
{
    RGBQUAD rgb;
    unsigned int row, col, i, j, k, width, height, cx, cy;
    // Tmp working copies of bitmap for 3 chans and buffer for sorting (win)
    uint8_t *res[3] = {NULL}, *win[3] = {NULL};
    int margin, middle, result = 0, mi;

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height && side>2 && side%2);

    margin = side/2; // process central area, without margin
    middle = 1 + side*side/2; // middle element in sorted buffer (win[i])
    // allocate tmp copies of channels
    for (i=0; i<3; i++) {
        res[i] = malloc(width*height*sizeof (uint8_t));
        if (!res[i]) {
            result = 1;
            goto free_bufs;
        }
    }
    // allocate win buffers
    for (i=0; i<3; i++) {
        win[i] = malloc(side*side*sizeof (uint8_t));
        if (!win[i]) {
            result = 1;
            goto free_bufs;
        }
    }
    // processing
    for (row = margin; row < height-margin; row++) {
        for (col = margin; col < width-margin; col++) {
            k = 0;
            for (j = 0; j < side; j++) {
                cy = row - margin + j;
                for (i = 0; i < side; i++) {
                    cx = col - margin + i;
                    FreeImage_GetPixelColor(img, cx, cy, &rgb);
                    *(win[0] + k) = RGB_R(&rgb);
                    *(win[1] + k) = RGB_G(&rgb);
                    *(win[2] + k) = RGB_B(&rgb);
                    k++;
                }
            }
            qsort(win[0], k, sizeof win[0][0], _median_cmpf);
            qsort(win[1], k, sizeof win[1][0], _median_cmpf);
            qsort(win[2], k, sizeof win[2][0], _median_cmpf);
            mi = row*width + col;
            *(res[0] + mi) = win[0][middle];
            *(res[1] + mi) = win[1][middle];
            *(res[2] + mi) = win[2][middle];
        }
    }
    // copy to original img data from tmp copies, margin is unchanged
    for (row = margin; row < height-margin; row++) {
        for (col = margin; col < width-margin; col++) {
            mi = row*width + col;
            RGBINIT(&rgb, *(res[0] + mi), *(res[1] + mi), *(res[2] + mi));
            FreeImage_SetPixelColor(img, col, row, &rgb);
        }
    }
free_bufs:
    for (i=0; i<3; i++) {
        if (res[i]) {
            free(res[i]);
            res[i] = NULL;
        }
        if (win[i]) {
            free(win[i]);
            win[i] = NULL;
        }
    }
    return (result);
}

/** Binary dilate and erode generic implementation */
#define _ERODE_DILATE_GENERIC_ALGO() do { \
    RGBQUAD rgb; \
    unsigned int row, col, i, j, width, height, cx, cy; \
    uint8_t *res[3] = {NULL}; /* Tmp working copies of bitmap for 3 chans */ \
    int margin, mi; \
    bool detected; \
    width = FreeImage_GetWidth(img); \
    height = FreeImage_GetHeight(img); \
    \
    assert(img && width && height && matrix && side>2 && side%2); \
    \
    /* Process central area, without margin. Also margin is middle point, for side=5, middle is 2: 0,1,[2],3,4 */ \
    margin = side/2; \
    /* allocate tmp copies of channels */ \
    for (i=0; i<3; i++) { \
        res[i] = malloc(width*height*sizeof (uint8_t)); \
        if (!res[i]) { \
            while (i >= 0) free(res[i--]); \
            return (1); \
        } \
    } \
    /* processing */ \
    for (row = margin; row < height-margin; row++) { \
        for (col = margin; col < width-margin; col++) { \
            detected = false; \
            for (j = 0; j < side; j++) { \
                cy = row - margin + j; \
                for (i = 0; i < side; i++) { \
                    cx = col - margin + i; \
                    if (j == margin && i == margin) { \
                        /* if center point */ \
                        continue; \
                    } else { \
                        mi = j*side + i; \
                        FreeImage_GetPixelColor(img, cx, cy, &rgb); \
                        if (matrix[mi] \
                                && (__ISCOND(RGB_R(&rgb)) \
                                    || __ISCOND(RGB_G(&rgb)) \
                                    || __ISCOND(RGB_B(&rgb)))) { \
                            detected = true; \
                            goto _detected; \
                        } \
                    } \
                } \
            } \
_detected: \
            mi = row*width + col; \
            if (detected) { \
                *(res[0] + mi) = *(res[1] + mi) = *(res[2] + mi) = __RESYES; \
            } else { \
                *(res[0] + mi) = (-1==__RESNO)? RGB_R(&rgb) : __RESNO; \
                *(res[1] + mi) = (-1==__RESNO)? RGB_G(&rgb) : __RESNO; \
                *(res[2] + mi) = (-1==__RESNO)? RGB_B(&rgb) : __RESNO; \
            } \
        } \
    } \
    /* copy to original img data from tmp copies, margin is unchanged */ \
    for (row = margin; row < height-margin; row++) { \
        for (col = margin; col < width-margin; col++) { \
            RGBINIT(&rgb, \
                    *(res[0] + row*width + col), \
                    *(res[1] + row*width + col), \
                    *(res[2] + row*width + col)); \
            FreeImage_SetPixelColor(img, col, row, &rgb); \
        } \
    } \
    /* free tmp copies */ \
    for (i=0; i<3; i++) free(res[i]); \
} while (0)

/**
 * Binary dilate
 */
int algo_bindilate(IMG *img, int *matrix, int side)
{
#define __RESYES 255
#define __RESNO 0
#define __ISCOND(X) ((X)!=0)

    _ERODE_DILATE_GENERIC_ALGO();
    return (0);

#undef __RESYES
#undef __RESNO
#undef __ISCOND
}

/**
 * Binary erode
 */
int algo_binerode(IMG *img, int *matrix, int side)
{
#define __RESYES 0
#define __RESNO 255
#define __ISCOND(X) ((X)==0)

    _ERODE_DILATE_GENERIC_ALGO();
    return (0);

#undef __RESYES
#undef __RESNO
#undef __ISCOND
}

/** For binary operations on pixels as sets */

/* all channels should has the same values */
#define __GETBIT(Q) ((RGB_R(Q)!=0)?(-1):0)
#define __SETBIT(Q, B) do { \
    if (B) { \
        RGBINIT(Q, 255, 255, 255); \
    } else  { \
        RGBINIT(Q, 0, 0, 0); \
    } \
} while (0)

/**
 * Binary monadic not operation. img is in/out.
 */
int algo_binnot(IMG *img)
{
    RGBQUAD rgb;
    unsigned int row, col, width, height;

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height);

    // processing
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            FreeImage_GetPixelColor(img, col, row, &rgb);
            __SETBIT(&rgb, ~__GETBIT(&rgb));
            FreeImage_SetPixelColor(img, col, row, &rgb);
        }
    }
    return (0);
}

/**
 * Binary dyadic operations, result set to first.
 * op is:
 *      & - AND
 *      | - OR
 *      ^ - XOR
 *      / - SUB (X but not Y)
 */
int algo_binop2(char op, IMG *img1, IMG *img2)
{
    RGBQUAD rgb1, rgb2;
    unsigned int row, col, width1, height1, width2, height2,
                 wbound, hbound;

    width1 = FreeImage_GetWidth(img1);
    height1 = FreeImage_GetHeight(img1);

    width2 = FreeImage_GetWidth(img2);
    height2 = FreeImage_GetHeight(img2);

    assert(img1 && img2 && width1 && height1 && width2 && height2);

    wbound = MIN(width1, width2);
    hbound = MIN(height1, height2);

    // processing
    for (row = 0; row < hbound; row++) {
        for (col = 0; col < wbound; col++) {
            FreeImage_GetPixelColor(img1, col, row, &rgb1);
            FreeImage_GetPixelColor(img2, col, row, &rgb2);
            switch (op) {
                case '&':
                    __SETBIT(&rgb1, __GETBIT(&rgb1) & __GETBIT(&rgb2));
                    break;

                case '|':
                    __SETBIT(&rgb1, __GETBIT(&rgb1) | __GETBIT(&rgb2));
                    break;

                case '^':
                    __SETBIT(&rgb1, __GETBIT(&rgb1) ^ __GETBIT(&rgb2));
                    break;

                case '/':
                    __SETBIT(&rgb1, __GETBIT(&rgb1) & (__GETBIT(&rgb1) ^ __GETBIT(&rgb2)));
                    break;
            }
            FreeImage_SetPixelColor(img1, col, row, &rgb1);
        }
    }
    return (0);
}

#undef __GETBIT
#undef __SETBIT

#define __DITH_FNDCLOSEST(RGBQ) (brightness(RGBQ)<128? RGB_BLACK:RGB_WHITE)
#define __DITH_GET(X, Y) (FreeImage_GetPixelColor(img, X, Y, &oldrgb), &oldrgb)
#define __DITH_SET(X, Y, C) (newrgb = (C), FreeImage_SetPixelColor(img, X, Y, &newrgb))

/// Atkinson dithering
int algo_atkinson_dith(IMG *img)
{
    RGBQUAD newrgb, oldrgb;
    float qerr, c;
    unsigned int x, y, row, col, i, width, height;
    uint8_t *tmp = NULL;
    static const int
        s = 1, // step
        margin = 0; //process central area, without margin

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height);

    i = width*height*sizeof (uint8_t);
    tmp = (uint8_t*)malloc(i);
    if (!tmp) return (1);
    memset(tmp, 0, i);

    for (row = margin; row < height-margin; row+=s) {
        for (col = margin; col < width-margin; col+=s) {
            FreeImage_GetPixelColor(img, col, row, &oldrgb);
            newrgb = __DITH_FNDCLOSEST(&oldrgb);
            qerr = brightness(&oldrgb) - brightness(&newrgb);
            FreeImage_SetPixelColor(img, col, row, &newrgb);
            *(tmp + sizeof (uint8_t)*(row*width + col)) = RGB_R(&newrgb);

            struct { int x, y; }
                mat[] = {{col+s, row},   {col-s, row+s},   {col, row+s},
                         {col+s, row+s}, {col+2*s, row}, {col, row+2*s}};

            for (i = 0; i < sizeofarray(mat); i++) {
                x = mat[i].x; y = mat[i].y;
                __DITH_SET(x, y, RGB1(brightness(__DITH_GET(x, y)) + 1./8*qerr));
            }
        }
    }

    // copy to original img data from tmp copies, margin is unchanged
    for (row = margin; row < height-margin; row++) {
        for (col = margin; col < width-margin; col++) {
            newrgb = RGB1(*(tmp + sizeof (uint8_t)*(row*width + col)));
            FreeImage_SetPixelColor(img, col, row, &newrgb);
        }
    }

    free(tmp);
    return (0);
}

int algo_general_dith(IMG *img, dithpt_t *pts, int npts)
{
    RGBQUAD newrgb, oldrgb;
    float qerr, k;
    unsigned int x, y, row, col, i, width, height;
    uint8_t *tmp = NULL;
    static const int
        s = 1, // step
        margin = 0; //process central area, without margin

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height);

    i = width*height*sizeof (uint8_t);
    tmp = (uint8_t*)malloc(i);
    if (!tmp) return (1);
    memset(tmp, 0, i);

    for (row = margin; row < height-margin; row+=s) {
        for (col = margin; col < width-margin; col+=s) {
            FreeImage_GetPixelColor(img, col, row, &oldrgb);
            newrgb = __DITH_FNDCLOSEST(&oldrgb);
            qerr = brightness(&oldrgb) - brightness(&newrgb);
            FreeImage_SetPixelColor(img, col, row, &newrgb);
            *(tmp + sizeof (uint8_t)*(row*width + col)) = RGB_R(&newrgb);

            /*struct { int x, y; float k; }
                mat[] = {{col+s, row, 8./42}, {col+2*s, row, 4./42},
                         {col-2*s, row+s, 2./42}, {col-s, row+s, 4./42},
                         {col, row+s, 8./42}, {col+s, row+s, 4./42},
                         {col+2*s, row+s, 2./42}, {col-2*s, row+2*s, 1./42},
                         {col-s, row+2*s, 2./42}, {col, row+2*s, 4./42},
                         {col+s, row+2*s, 2./42}, {col+2*s, row+2*s, 1./42}};*/

            for (i = 0; i < npts; i++) {
                x = col + s*pts[i].x;
                y = row + s*pts[i].y;
                k = pts[i].k*qerr;
                __DITH_SET(x, y, RGB1(brightness(__DITH_GET(x, y)) + k));
            }
        }
    }

    // copy to original img data from tmp copies, margin is unchanged
    for (row = margin; row < height-margin; row++) {
        for (col = margin; col < width-margin; col++) {
            newrgb = RGB1(*(tmp + sizeof (uint8_t)*(row*width + col)));
            FreeImage_SetPixelColor(img, col, row, &newrgb);
        }
    }

    free(tmp);
    return (0);
}

/// Stucki dithering
int algo_stucki_dith(IMG *img)
{
    RGBQUAD newrgb, oldrgb;
    float qerr, k;
    unsigned int x, y, row, col, i, width, height;
    uint8_t *tmp = NULL;
    static const int
        s = 1, // step
        margin = 0; //process central area, without margin

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height);

    i = width*height*sizeof (uint8_t);
    tmp = (uint8_t*)malloc(i);
    if (!tmp) return (1);
    memset(tmp, 0, i);

    for (row = margin; row < height-margin; row+=s) {
        for (col = margin; col < width-margin; col+=s) {
            FreeImage_GetPixelColor(img, col, row, &oldrgb);
            newrgb = __DITH_FNDCLOSEST(&oldrgb);
            qerr = brightness(&oldrgb) - brightness(&newrgb);
            FreeImage_SetPixelColor(img, col, row, &newrgb);
            *(tmp + sizeof (uint8_t)*(row*width + col)) = RGB_R(&newrgb);

            struct { int x, y; float k; }
                mat[] = {{col+s, row, 8./42}, {col+2*s, row, 4./42},
                         {col-2*s, row+s, 2./42}, {col-s, row+s, 4./42},
                         {col, row+s, 8./42}, {col+s, row+s, 4./42},
                         {col+2*s, row+s, 2./42}, {col-2*s, row+2*s, 1./42},
                         {col-s, row+2*s, 2./42}, {col, row+2*s, 4./42},
                         {col+s, row+2*s, 2./42}, {col+2*s, row+2*s, 1./42}};

            for (i = 0; i < sizeofarray(mat); i++) {
                x = mat[i].x; y = mat[i].y; k = mat[i].k*qerr;
                __DITH_SET(x, y, RGB1(brightness(__DITH_GET(x, y)) + k));
            }
        }
    }

    // copy to original img data from tmp copies, margin is unchanged
    for (row = margin; row < height-margin; row++) {
        for (col = margin; col < width-margin; col++) {
            newrgb = RGB1(*(tmp + sizeof (uint8_t)*(row*width + col)));
            FreeImage_SetPixelColor(img, col, row, &newrgb);
        }
    }

    free(tmp);
    return (0);
}

/// Floyd-Steinberg dithering
int algo_floyd_steinberg_dith(IMG *img)
{
    RGBQUAD newrgb, oldrgb;
    float qerr, c, k;
    unsigned int x, y, row, col, i, width, height;
    uint8_t *tmp = NULL;
    static const int
        s = 1, // step
        margin = 0; //process central area, without margin

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height);

    i = width*height*sizeof (uint8_t);
    tmp = (uint8_t*)malloc(i);
    if (!tmp) return (1);
    memset(tmp, 255, i);

    for (row = margin; row < height-margin; row+=s) {
        for (col = margin; col < width-margin; col+=s) {
            FreeImage_GetPixelColor(img, col, row, &oldrgb);
            newrgb = __DITH_FNDCLOSEST(&oldrgb);
            qerr = brightness(&oldrgb) - brightness(&newrgb);
            FreeImage_SetPixelColor(img, col, row, &newrgb);
            *(tmp + sizeof (uint8_t)*(row*width + col)) = RGB_R(&newrgb);

            struct { int x, y; float k; }
                mat[] = {{col+s, row, 7./16}, {col-s, row+s, 3./16},
                         {col, row+s, 5./16}, {col+s, row+s, 1./16}};

            for (i = 0; i < sizeofarray(mat); i++) {
                x = mat[i].x; y = mat[i].y; k = mat[i].k*qerr;
                __DITH_SET(x, y, RGB1(brightness(__DITH_GET(x, y)) + k));
            }
        }
    }

    // copy to original img data from tmp copies, margin is unchanged
    for (row = margin; row < height-margin; row++) {
        for (col = margin; col < width-margin; col++) {
            newrgb = RGB1(*(tmp + sizeof (uint8_t)*(row*width + col)));
            FreeImage_SetPixelColor(img, col, row, &newrgb);
        }
    }

    free(tmp);
    return (0);
}

/// Ordered dithering
int algo_ordered_dith(IMG *img)
{
    RGBQUAD newrgb, oldrgb;
    static const float mratio = 1./17, mfactor = 255./5;
    int matrix[][4] = {{1, 9, 3, 11}, {13, 5, 15, 7},
                      {4, 12, 2, 10}, {16, 8, 14, 6}};
    unsigned int row, col, width, height;
    static const int
        s = 1, // step
        margin = 0; //process central area, without margin

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height);

    for (row = margin; row < height-margin; row+=s) {
        for (col = margin; col < width-margin; col+=s) {
            FreeImage_GetPixelColor(img, col, row, &oldrgb);
            newrgb = RGB1(brightness(&oldrgb) + mratio*matrix[col%4][row%4]*mfactor);
            newrgb = __DITH_FNDCLOSEST(&newrgb);
            FreeImage_SetPixelColor(img, col, row, &newrgb);
        }
    }

    return (0);
}

/// Random dithering
int algo_random_dith(IMG *img)
{
    RGBQUAD newrgb, oldrgb;
    float qerr, c;
    unsigned int x, y, row, col, width, height;
    static const int
        s = 1, // step
        margin = 0; //process central area, without margin

    width = FreeImage_GetWidth(img);
    height = FreeImage_GetHeight(img);

    assert(img && width && height);

    for (row = margin; row < height-margin; row+=s) {
        for (col = margin; col < width-margin; col+=s) {
            FreeImage_GetPixelColor(img, col, row, &oldrgb);
            newrgb = RGB1(brightness(&oldrgb) + random_range(-64, 64));
            newrgb = __DITH_FNDCLOSEST(&newrgb);
            FreeImage_SetPixelColor(img, col, row, &newrgb);
        }
    }

    return (0);
}

#undef __DITH_FNDCLOSEST
#undef __DITH_GET
#undef __DITH_SET
/** Thining algorithm of Zhang-Suen
*/
/*int algo_zhs(IMG *img)
  {
  RGBQUAD rgb;
  unsigned int row, col, i, j, k, width, height, cx, cy;
// Tmp working copies of bitmap for 3 chans and buffer for sorting (win)
uint8_t *preimg = NULL;
int margin, middle, result = 0, mi;
static const int
margin = 1, //process central area, without margin
middle = 1; // middle element in sorted buffer (win[i])

width = FreeImage_GetWidth(img);
height = FreeImage_GetHeight(img);

assert(img && width && height);

// allocate tmp copies of channels
preimg = malloc(width*height*sizeof (uint8_t));
if (!preimg) {
result = 1;
goto free_bufs;
}
// processing
for (row = margin; row < height-margin; row++) {
for (col = margin; col < width-margin; col++) {
k = 0;
for (j = 0; j < 3; j++) {
cy = row - margin + j;
for (i = 0; i < side; i++) {
cx = col - margin + i;
FreeImage_GetPixelColor(img, cx, cy, &rgb);
 *(win[0] + k) = RGB_R(&rgb);
 *(win[1] + k) = RGB_G(&rgb);
 *(win[2] + k) = RGB_B(&rgb);
 k++;
 }
 }
 mi = row*width + col;
 *(res[0] + mi) = win[0][middle];
 *(res[1] + mi) = win[1][middle];
 *(res[2] + mi) = win[2][middle];
 }
 }
// copy to original img data from tmp copies, margin is unchanged
for (row = margin; row < height-margin; row++) {
for (col = margin; col < width-margin; col++) {
mi = row*width + col;
RGBINIT(&rgb, *(res[0] + mi), *(res[1] + mi), *(res[2] + mi));
FreeImage_SetPixelColor(img, col, row, &rgb);
}
}
free_bufs:
free(preimg);
return (result);
}*/

// ---------------------------------------------------------------------------
// }}}

// Algorithms wrappers {{{
// ---------------------------------------------------------------------------

LISP img_bw(LISP img, LISP threshold)
{
    IMG *_img = NULL;
    long _threshold;

    _img = LISP2IMG(img);
    _threshold = get_c_long(threshold);
    if (!algo_bw(_img, (int)_threshold))
        return (a_true_value());
    else return (NIL);
}

LISP img_gr(LISP img)
{
    IMG *_img = NULL;

    _img = LISP2IMG(img);
    if (!algo_gr(_img))
        return (a_true_value());
    else return (NIL);
}

LISP img_neg(LISP img)
{
    IMG *_img = NULL;

    _img = LISP2IMG(img);
    if (!algo_neg(_img))
        return (a_true_value());
    else return (NIL);
}

LISP img_conv(LISP img, LISP matrix, LISP div, LISP shift)
{
    IMG *_img = NULL;
    LISP res, l;
    long _div, _shift;
    int *_matrix = NULL, matrix_len, side, i;

    matrix_len = nlength(matrix);
    side = sqrt(matrix_len);
    _matrix = malloc(matrix_len*sizeof(int));
    if (!_matrix) {
        res = err(__func__, llast_c_errmsg(-1));
        goto end;
    }
    for (l=matrix, i=0; i<matrix_len /*&& NNULLP(l)*/; l=cdr(l), i++) {
        _matrix[i] = get_c_long(car(l));
    }
    _img = LISP2IMG(img);
    _div = get_c_long(div);
    _shift = get_c_long(shift);
    if (!algo_conv(_img, _matrix, side, (int)_div, (int)_shift)) {
        res = a_true_value();
        goto free_matrix;
    } else {
        res = NIL;
        goto free_matrix;
    }
free_matrix:
    free(_matrix);
end:
    return (res);
}

LISP img_ch(LISP img, LISP channels)
{
    IMG *_img = NULL;
    long _channels;

    _img = LISP2IMG(img);
    _channels = get_c_long(channels);
    if (!algo_ch(_img, (int)_channels))
        return (a_true_value());
    else return (NIL);
}

LISP img_grl(LISP img, LISP nlevels)
{
    IMG *_img = NULL;
    long _nlevels;

    _img = LISP2IMG(img);
    _nlevels = get_c_long(nlevels);
    if (!algo_grl(_img, (int)_nlevels))
        return (a_true_value());
    else return (NIL);
}

LISP img_pix(LISP img, LISP side, LISP color)
{
    IMG *_img = NULL;
    long _side, _color;

    _img = LISP2IMG(img);
    _side = get_c_long(side);
    _color = get_c_long(color);
    if (!algo_pix(_img, (int)_side, (int)_color))
        return (a_true_value());
    else return (NIL);
}

LISP img_med(LISP img, LISP side)
{
    IMG *_img = NULL;
    long _side;

    _img = LISP2IMG(img);
    _side = get_c_long(side);
    if (!algo_med(_img, (int)_side))
        return (a_true_value());
    else return (NIL);
}

LISP img_bindilate(LISP img, LISP matrix)
{
    IMG *_img = NULL;
    LISP res, l;
    int *_matrix = NULL, matrix_len, side, i;

    matrix_len = nlength(matrix);
    _matrix = malloc(matrix_len*sizeof(int));
    if (!_matrix) {
        res = err(__func__, llast_c_errmsg(-1));
        goto end;
    }
    side = sqrt(matrix_len);
    for (l=matrix, i=0; i<matrix_len; l=cdr(l), i++) {
        _matrix[i] = get_c_long(car(l));
    }
    _img = LISP2IMG(img);
    if (!algo_bindilate(_img, _matrix, side)) {
        res = a_true_value();
        goto free_matrix;
    } else {
        res = NIL;
        goto free_matrix;
    }
free_matrix:
    free(_matrix);
end:
    return (res);
}

LISP img_binerode(LISP img, LISP matrix)
{
    IMG *_img = NULL;
    LISP res, l;
    int *_matrix = NULL, matrix_len, side, i;

    matrix_len = nlength(matrix);
    _matrix = malloc(matrix_len*sizeof(int));
    if (!_matrix) {
        res = err(__func__, llast_c_errmsg(-1));
        goto end;
    }
    side = sqrt(matrix_len);
    for (l=matrix, i=0; i<matrix_len; l=cdr(l), i++) {
        _matrix[i] = get_c_long(car(l));
    }
    _img = LISP2IMG(img);
    if (!algo_binerode(_img, _matrix, side)) {
        res = a_true_value();
        goto free_matrix;
    } else {
        res = NIL;
        goto free_matrix;
    }
free_matrix:
    free(_matrix);
end:
    return (res);
}

#define _GENERAL_BINOP() do { \
    IMG *_img1 = NULL, *_img2 = NULL; \
    \
    _img1 = LISP2IMG(img1); \
    _img2 = LISP2IMG(img2); \
    if (!algo_binop2(__OP, _img1, _img2)) \
    return (a_true_value()); \
    else \
    return (NIL); \
} while (0)

// and or xor sub
LISP img_binand(LISP img1, LISP img2)
{
#define __OP '&'
    _GENERAL_BINOP();
#undef __OP
}

LISP img_binor(LISP img1, LISP img2)
{
#define __OP '|'
    _GENERAL_BINOP();
#undef __OP
}

LISP img_binxor(LISP img1, LISP img2)
{
#define __OP '^'
    _GENERAL_BINOP();
#undef __OP
}

LISP img_binsub(LISP img1, LISP img2)
{
#define __OP '/'
    _GENERAL_BINOP();
#undef __OP
}

LISP img_binnot(LISP img)
{
    IMG *_img = NULL;

    _img = LISP2IMG(img);
    if (!algo_binnot(_img))
        return (a_true_value());
    else
        return (NIL);
}

LISP img_gendith(LISP img, LISP matrix)
{
    IMG *_img = NULL;
    dithpt_t *_matrix = NULL;
    LISP res, l;
    double d;
    int matrix_len, i;

    matrix_len = nlength(matrix)/3;
    _matrix = (dithpt_t*)malloc(matrix_len*sizeof(dithpt_t));
    if (!_matrix) {
        res = err(__func__, llast_c_errmsg(-1));
        goto end;
    }
    for (l=matrix, i=0; i<matrix_len; l=cdr(l), i++) {
        d = get_c_double(car(l));
        switch (i%3) {
            case 0: _matrix[i/3].x = (int)d; break;
            case 1: _matrix[i/3].y = (int)d; break;
            default: _matrix[i/3].k = d; break;
        }
    }
    _img = LISP2IMG(img);
    if (!algo_general_dith(_img, _matrix, matrix_len/3)) {
        res = a_true_value();
        goto free_matrix;
    } else {
        res = NIL;
        goto free_matrix;
    }
free_matrix:
    free(_matrix);
end:
    return (res);
}

LISP img_dith(LISP img, LISP alg)
{
    IMG *_img = NULL;
    long _alg;
    int (*pfn[])(IMG *img) = {
        algo_atkinson_dith, algo_floyd_steinberg_dith, algo_ordered_dith,
        algo_random_dith, algo_stucki_dith
    };

    _img = LISP2IMG(img);
    _alg = get_c_long(alg);
    if (_alg < 0 || _alg >= sizeofarray(pfn)) return (NIL);
    else {
        if (!pfn[_alg](_img)) return (a_true_value());
        else return (NIL);
    }
}


// ---------------------------------------------------------------------------
// }}}

// Common IMG wrappers {{{
// ---------------------------------------------------------------------------

/** Convert internal lisp object to stored there IMG pointer
*/
static IMG* LISP2IMG(LISP ptr)
{
    if (NTYPEP(ptr, tc_img)) err("not a IMG", ptr);
    if (!ptr->storage_as.string.data) err("IMG deallocated", ptr);
    else return ((IMG*)ptr->storage_as.string.data);
}

void _img_gc_free(LISP ptr)
{
    IMG *img;
    if ((img = LISP2IMG(ptr)))
        FreeImage_Unload(img);
    ptr->storage_as.string.data = NULL;
}

void _img_print(LISP ptr, struct gen_printio *f)
{
    char buf[16];
    IMG *img;
    img = LISP2IMG(ptr);
    sprintf(buf, "#<IMG %p>", img);
    gput_st(f, buf);
}

/** Load IMG from file
*/
LISP img_open(LISP fname)
{
    IMG *img = NULL;
    LISP res;
    long intflag;
    char *_fname;
    FREE_IMAGE_FORMAT fif;

    _fname = get_c_string(fname);

    intflag = no_interrupt(1);
    fif = FreeImage_GetFileType(_fname, 0);
    if (fif == FIF_UNKNOWN) {
        fif = FreeImage_GetFIFFromFilename(_fname);
        if (fif == FIF_UNKNOWN) {
            // fif can not be determined
            res = err(__func__, llast_c_errmsg(-1));
            goto end;
        }
    }
    img = FreeImage_Load(fif, _fname, 0);
    if (!img) {
        res = err(__func__, llast_c_errmsg(-1));
    } else {
        res = cons(NIL, NIL);
        res->type = tc_img;
        res->storage_as.string.data = (char*)img;
    }
end:
    no_interrupt(intflag);
    return (res);
}

/** Save IMG to file
*/
LISP img_save(LISP img, LISP fname)
{
    IMG *_img = NULL;
    LISP res;
    long intflag;
    char *_fname;
    FREE_IMAGE_FORMAT fif;

    _img = LISP2IMG(img);
    _fname = get_c_string(fname);

    intflag = no_interrupt(1);
    fif = FreeImage_GetFIFFromFilename(_fname);
    if (fif == FIF_UNKNOWN) {
        res = err(__func__, llast_c_errmsg(-1));
        goto end;
    }
    if (!FreeImage_Save(fif, _img, _fname, 0)) {
        res = err(__func__, llast_c_errmsg(-1));
        goto end;
    } else {
        res = a_true_value();
    }
end:
    no_interrupt(intflag);
    return (res);
}

/** Initialize IMG module
*/
void _img_init(void)
{
    long _kind;
    tc_img = allocate_user_tc();
    set_gc_hooks(tc_img, NULL, NULL, NULL, _img_gc_free, &_kind);
    set_print_hooks(tc_img, _img_print);
    init_storage();
    init_subrs();
    init_trace();
    init_imsh_version();
    init_subr_1("img_open", img_open);
    init_subr_2("img_save", img_save);
    init_subr_2("img_bw", img_bw);
    init_subr_1("img_gr", img_gr);
    init_subr_1("img_neg", img_neg);
    init_subr_4("img_conv", img_conv);
    init_subr_2("img_ch", img_ch);
    init_subr_2("img_grl", img_grl);
    init_subr_3("img_pix", img_pix);
    init_subr_2("img_med", img_med);
    init_subr_2("img_bindilate", img_bindilate);
    init_subr_2("img_binerode", img_binerode);
    init_subr_2("img_binand", img_binand);
    init_subr_2("img_binor", img_binor);
    init_subr_2("img_binxor", img_binxor);
    init_subr_2("img_binsub", img_binsub);
    init_subr_1("img_binnot", img_binnot);
    init_subr_2("img_dith", img_dith);
    init_subr_2("img_gendith", img_gendith);
}

// ---------------------------------------------------------------------------
// }}}

// Main entry {{{
// ---------------------------------------------------------------------------

int main(int argc, char **argv)
{
    const char _wrapper[] = "(*catch 'errobj (begin ";
    // size of _wrapper array without trailing '\0'
#define _WRAPPER_SIZE (sizeof _wrapper - 1)
    long retval = -1;
    int i;

    //FIBITMAP *bitmap = FreeImage_Load(FIF_IMG, "ngirl.img", IMG_DEFAULT);
    //FreeImage_Save(FIF_JPEG, bitmap, "mybitmap.jpg", 0);
    //FreeImage_Unload(bitmap);

    _img_init();
    if (1 == argc) {
        // REPL loop
        retval = repl_driver(1, 1, NULL);
    } else {
        FILE *fp = NULL;
        char *buf = NULL;
        long filesize;

        // XXX b - to prevent translating \r\n to \n!
        fp = fopen(argv[1], "rb");
        if (!fp) {
            fprintf(stderr, "Can not open '%s'!\n", argv[1]);
            retval = 255;
            goto closefile;
        }
        fseek(fp, 0, SEEK_END);
        buf = malloc((filesize=ftell(fp)) + _WRAPPER_SIZE + 10); // add for wrapper
        if (!buf) {
            fprintf(stderr, "Not enought memory!\n");
            retval = 255;
            goto freebuf;
        }
        // all is OK here
        strcpy(buf, _wrapper);
        fseek(fp, 0, SEEK_SET);
        fread(buf + _WRAPPER_SIZE, sizeof (char), filesize, fp);
        for (i = _WRAPPER_SIZE; i < _WRAPPER_SIZE + filesize; i++) {
            if (buf[i] == '\n' || buf[i] == '\r')
                buf[i] = ' ';
        }
        strcpy(buf + i, "))");
        retval = repl_c_string(buf, 1, 1, 1);
freebuf:
        free(buf);
        buf = NULL;
closefile:
        fclose(fp);
        fp = NULL;
    }
    printf("done with %d\n", retval);
    return (retval);
}

// }}}
