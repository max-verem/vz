/*
    Here some notes about bytes order for pix fmts

    BGR
      0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    | B | G | R | B | G | R | B | G | R | B | G | R | B | G | R | B | .....
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     |=========| |=========| |=========| |=========| |=========| |=== .....

    BGRA
      0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    | B | G | R | A | B | G | R | A | B | G | R | A | B | G | R | A | .....
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     |=============| |=============| |=============| |=============|


    RGB
      0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    | R | G | B | R | G | B | R | G | B | R | G | B | R | G | R | R | .....
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     |=========| |=========| |=========| |=========| |=========| |=== .....

     RGBA
      0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    | R | G | B | A | R | G | B | A | R | G | B | A | R | G | B | A | .....
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     |=============| |=============| |=============| |=============|

      GRAY (luminance)
      0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | .....
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     |=| |=| |=| |=| |=| |=| |=| |=| |=| |=| |=| |=| |=| |=| |=| |=|

 */
#define PIXFMTCONV_BEGIN(NAME)                                      \
static void NAME(unsigned char* src, unsigned char* dst, int count) \
{                                                                   \
    unsigned char t;                                                \
    for(int i = 0; i < count; i++)                                  \
    {

#define PIXFMTCONV_END                                              \
    };                                                              \
};

#pragma warning(push)
#pragma warning(disable:4101)

PIXFMTCONV_BEGIN(line_conv_BGR_to_BGRA)
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = 0xFF;
    dst += 4; src += 3;
PIXFMTCONV_END

PIXFMTCONV_BEGIN(line_conv_BGR_to_RGB)
    t = src[0];
    src[0] = src[2];
    src[2] = t;
    src += 3;
PIXFMTCONV_END

PIXFMTCONV_BEGIN(line_conv_BGR_to_RGBA)
    dst[0] = src[2];
    dst[1] = src[1];
    dst[2] = src[0];
    dst[3] = 0xFF;
    dst += 4; src += 3;
PIXFMTCONV_END

PIXFMTCONV_BEGIN(line_conv_BGRA_to_BGR)
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst += 3; src += 4;
PIXFMTCONV_END

PIXFMTCONV_BEGIN(line_conv_BGRA_to_RGB)
    dst[0] = src[2];
    dst[1] = src[1];
    dst[2] = src[0];
    dst += 3; src += 4;
PIXFMTCONV_END

PIXFMTCONV_BEGIN(line_conv_BGRA_to_RGBA)
    t = src[0];
    src[0] = src[2];
    src[2] = t;
    src += 4;
PIXFMTCONV_END

PIXFMTCONV_BEGIN(line_conv_GRAY_to_RGB)
    dst[2] = dst[1] = dst[0] = src[0];
    dst += 3; src += 1;
PIXFMTCONV_END

PIXFMTCONV_BEGIN(line_conv_GRAY_to_RGBA)
    dst[2] = dst[1] = dst[0] = src[0];
    dst[3] = 0xFF;
    dst += 4; src += 1;
PIXFMTCONV_END

#pragma warning(pop)
