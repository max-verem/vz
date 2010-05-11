#include "vzTTFont.h"
#include "vzTTFontFilter.h"
#include "stdint.h"
#include "vzLogger.h"

struct arabic_presentation_form_desc
{
    uint16_t origin;
    uint8_t shape;
    uint16_t isolated;
    uint16_t end;
    uint16_t middle;
    uint16_t beginning;
};

const static struct arabic_presentation_form_desc APFB[] =
{
    { 0x0621, 'U', 0xFE80 },
    { 0x0622, 'R', 0xFE81, 0xFE82 },
    { 0x0623, 'R', 0xFE83, 0xFE84 },
    { 0x0624, 'R', 0xFE85, 0xFE86 },
    { 0x0625, 'R', 0xFE87, 0xFE88 },
    { 0x0626, 'D', 0xFE89, 0xFE8A, 0xFE8B, 0xFE8C },
    { 0x0627, 'R', 0xFE8D, 0xFE8E },
    { 0x0628, 'D', 0xFE8F, 0xFE90, 0xFE91, 0xFE92 },
    { 0x0629, 'R', 0xFE93, 0xFE94 },
    { 0x062A, 'D', 0xFE95, 0xFE96, 0xFE97, 0xFE98 },
    { 0x062B, 'D', 0xFE99, 0xFE9A, 0xFE9B, 0xFE9C },
    { 0x062C, 'D', 0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0 },
    { 0x062D, 'D', 0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4 },
    { 0x062E, 'D', 0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8 },
    { 0x062F, 'R', 0xFEA9, 0xFEAA },
    { 0x0630, 'R', 0xFEAB, 0xFEAC },
    { 0x0631, 'R', 0xFEAD, 0xFEAE },
    { 0x0632, 'R', 0xFEAF, 0xFEB0 },
    { 0x0633, 'D', 0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4 },
    { 0x0634, 'D', 0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8 },
    { 0x0635, 'D', 0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC },
    { 0x0636, 'D', 0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0 },
    { 0x0637, 'D', 0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4 },
    { 0x0638, 'D', 0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8 },
    { 0x0639, 'D', 0xFEC9, 0xFECA, 0xFECB, 0xFECC },
    { 0x063A, 'D', 0xFECD, 0xFECE, 0xFECF, 0xFED0 },
    { 0x0640, 'D', 0x0640, 0x0640, 0x0640, 0x0640 },
    { 0x0641, 'D', 0xFED1, 0xFED2, 0xFED3, 0xFED4 },
    { 0x0642, 'D', 0xFED5, 0xFED6, 0xFED7, 0xFED8 },
    { 0x0643, 'D', 0xFED9, 0xFEDA, 0xFEDB, 0xFEDC },
    { 0x0644, 'D', 0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0 },
    { 0x0645, 'D', 0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4 },
    { 0x0646, 'D', 0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8 },
    { 0x0647, 'D', 0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC },
    { 0x0648, 'R', 0xFEED, 0xFEEE },
    { 0x0649, 'D', 0xFEEF, 0xFEF0, 0xFBE8, 0xFBE9 },
    { 0x064A, 'D', 0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4 },
    { 0x0671, 'R', 0xFB50, 0xFB51 },
    { 0x0677, 'R', 0xFBDD /* ???? */ },
    { 0x0679, 'D', 0xFB66, 0xFB67, 0xFB68, 0xFB69 },
    { 0x067A, 'D', 0xFB5E, 0xFB5F, 0xFB60, 0xFB61 },
    { 0x067B, 'D', 0xFB52, 0xFB53, 0xFB54, 0xFB55 },
    { 0x067E, 'D', 0xFB56, 0xFB57, 0xFB58, 0xFB59 },
    { 0x067F, 'D', 0xFB62, 0xFB63, 0xFB64, 0xFB65 },
    { 0x0680, 'D', 0xFB5A, 0xFB5B, 0xFB5C, 0xFB5D },
    { 0x0683, 'D', 0xFB76, 0xFB77, 0xFB78, 0xFB79 },
    { 0x0684, 'D', 0xFB72, 0xFB73, 0xFB74, 0xFB75 },
    { 0x0686, 'D', 0xFB7A, 0xFB7B, 0xFB7C, 0xFB7D },
    { 0x0687, 'D', 0xFB7E, 0xFB7F, 0xFB80, 0xFB81 },
    { 0x0688, 'R', 0xFB88, 0xFB89 },
    { 0x068C, 'R', 0xFB84, 0xFB85 },
    { 0x068D, 'R', 0xFB82, 0xFB83 },
    { 0x068E, 'R', 0xFB86, 0xFB87 },
    { 0x0691, 'R', 0xFB8C, 0xFB8D },
    { 0x0698, 'R', 0xFB8A, 0xFB8B },
    { 0x06A4, 'D', 0xFB6A, 0xFB6B, 0xFB6C, 0xFB6D },
    { 0x06A6, 'D', 0xFB6E, 0xFB6F, 0xFB70, 0xFB71 },
    { 0x06A9, 'D', 0xFB8E, 0xFB8F, 0xFB90, 0xFB91 },
    { 0x06AD, 'D', 0xFBD3, 0xFBD4, 0xFBD5, 0xFBD6 },
    { 0x06AF, 'D', 0xFB92, 0xFB93, 0xFB94, 0xFB95 },
    { 0x06B1, 'D', 0xFB9A, 0xFB9B, 0xFB9C, 0xFB9D },
    { 0x06B3, 'D', 0xFB96, 0xFB97, 0xFB98, 0xFB99 },
    { 0x06BA, 'D', 0xFB9E, 0xFB9F /* ???? ???? */},
    { 0x06BB, 'D', 0xFBA0, 0xFBA1, 0xFBA2, 0xFBA3 },
    { 0x06BE, 'D', 0xFBAA, 0xFBAB, 0xFBAC, 0xFBAD },
    { 0x06C0, 'R', 0xFBA4, 0xFBA5 },
    { 0x06C1, 'D', 0xFBA6, 0xFBA7, 0xFBA8, 0xFBA9 },
    { 0x06C5, 'R', 0xFBE0, 0xFBE1 },
    { 0x06C6, 'R', 0xFBD9, 0xFBDA },
    { 0x06C7, 'R', 0xFBD7, 0xFBD8 },
    { 0x06C8, 'R', 0xFBDB, 0xFBDC },
    { 0x06C9, 'R', 0xFBE2, 0xFBE3 },
    { 0x06CB, 'R', 0xFBDE, 0xFBDF },
    { 0x06CC, 'D', 0xFBFC, 0xFBFD, 0xFBFE, 0xFBFF },
    { 0x06D0, 'D', 0xFBE4, 0xFBE5, 0xFBE6, 0xFBE7 },
    { 0x06D2, 'R', 0xFBAE, 0xFBAF },
    { 0x06D3, 'R', 0xFBB0, 0xFBB1 },
    {      0,  0,       0,      0,      0,      0 }
};

static int is_APFB_present(uint16_t c)
{
    if (!c) return 0;

    for(int i = 0; APFB[i].origin; i++)
    {
        if(APFB[i].origin       == c)     return ('o' << 8) | i;
        if(APFB[i].isolated     == c)     return ('i' << 8) | i;
        if(APFB[i].end          == c)     return ('e' << 8) | i;
        if(APFB[i].middle       == c)     return ('m' << 8) | i;
        if(APFB[i].beginning    == c)     return ('b' << 8) | i;
    };

    return 0;
};

static void revert_string(uint16_t* string_uni, int p1, int p2)
{
    for(; p1 < p2; p1++, p2--)
    {
        uint16_t r = string_uni[p1];
        string_uni[p1] = string_uni[p2];
        string_uni[p2] = r;
    };
};

static int vzTTFontFilterArabic(uint16_t* string_uni, struct vzTTFontLayoutConf* layout_conf)
{
    int i;
    int is_arabic = 0;

    /* check if any arabic symbols here */
    for(i = 0; !is_arabic && string_uni[i]; i++)
        is_arabic = is_APFB_present(string_uni[i]);

    /* should the filter be applied */
    if(!is_arabic) return 0;

    logger_printf(0, "vzTTFontFilterArabic: will be used");

    /* join symbols */
    for(i = 0; string_uni[i]; i++)
    {
        int f = 0, j, c;
        uint16_t r;

        /* request info about char */
        is_arabic = is_APFB_present(string_uni[i]);
        j = is_arabic & 0xFF;
        c = is_arabic >> 8;

        /* check if it is origin */
        if('o' != c) continue;

        /* setup flags */
        if(i && is_APFB_present(string_uni[i - 1]))                 f |= 2;
        if(string_uni[i + 1] && is_APFB_present(string_uni[i + 1])) f |= 1;

        /*  check flags */
        switch(f)
        {
            case 0: r = APFB[j].isolated;   break;
            case 3: r = APFB[j].middle;     break;
            case 2: r = APFB[j].end;        break;
            case 1: r = APFB[j].beginning;  break;
        };
logger_printf(1, "vzTTFontFilterArabic: S [%2d] 0x%.4X => 0x%.4X F=%d",
    i, string_uni[i], r, f);

        if(r) string_uni[i] = r;
    };

    /* revert string */
    revert_string(string_uni, 0, i - 1);

//for(i = 0; string_uni[i]; i++)
//logger_printf(1, "vzTTFontFilterArabic: R [%2d]=0x%.4X", i, string_uni[i]);

    return 0;
};

void vzTTFontFilter(uint16_t* string_uni, struct vzTTFontLayoutConf* layout_conf)
{
    /* arabic layout filter */
    vzTTFontFilterArabic(string_uni, layout_conf);

    return;
};

