#include <stdint.h>

static uint8_t is_leap(uint16_t y)
{
    return ((y % 4U) == 0U && ((y % 100U) != 0U || (y % 400U) == 0U)) ? 1U : 0U;
}

static uint8_t days_in_month(uint16_t y, uint8_t m)
{
    static const uint8_t dm[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2U) return (uint8_t)(dm[1] + is_leap(y));
    if (m >= 1U && m <= 12U) return dm[m - 1U];
    return 31U;
}

static uint8_t dow_mon1(uint16_t y, uint8_t m, uint8_t d)
{
    static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint16_t yy = y;
    if (m < 3U) yy -= 1U;
    uint32_t w = (uint32_t)(yy + yy/4U - yy/100U + yy/400U + t[m-1U] + d) % 7U;
    if (w == 0U) return 7U;
    return (uint8_t)w;
}

static uint8_t last_sunday_of_month(uint16_t y, uint8_t m)
{
    uint8_t last_day = days_in_month(y, m);
    uint8_t w_last   = dow_mon1(y, m, last_day);
    uint8_t back = (uint8_t)(w_last % 7U);
    return (uint8_t)(last_day - back);
}

static int cmp_date_time(uint16_t y, uint8_t mo, uint8_t d, uint8_t h,
                         uint16_t y2, uint8_t mo2, uint8_t d2, uint8_t h2)
{
    if (y != y2) return (y < y2) ? -1 : 1;
    if (mo != mo2) return (mo < mo2) ? -1 : 1;
    if (d != d2) return (d < d2) ? -1 : 1;
    if (h != h2) return (h < h2) ? -1 : 1;
    return 0;
}

static uint8_t warsaw_is_dst_utc(uint16_t y, uint8_t mo, uint8_t d, uint8_t h)
{
    if (mo < 3U || mo > 10U) return 0U;
    if (mo > 3U && mo < 10U) return 1U;

    if (mo == 3U) {
        uint8_t ls = last_sunday_of_month(y, 3U);

        return (cmp_date_time(y, mo, d, h, y, 3U, ls, 1U) >= 0) ? 1U : 0U;
    }

    {
        uint8_t ls = last_sunday_of_month(y, 10U);
        return (cmp_date_time(y, mo, d, h, y, 10U, ls, 1U) < 0) ? 1U : 0U;
    }
}

static void add_hours(uint16_t *y, uint8_t *mo, uint8_t *d, uint8_t *wd, uint8_t *h, uint8_t add)
{
    uint16_t Y = *y; uint8_t M = *mo; uint8_t D = *d; uint8_t WD = *wd; uint8_t H = *h;

    H = (uint8_t)(H + add);
    while (H >= 24U) {
        H = (uint8_t)(H - 24U);
        uint8_t dim = days_in_month(Y, M);
        D++;
        if (D > dim) {
            D = 1U;
            M++;
            if (M > 12U) {
                M = 1U;
                Y++;
            }
        }
        WD++;
        if (WD > 7U) WD = 1U;
    }

    *y = Y; *mo = M; *d = D; *wd = WD; *h = H;
}

void rtc_utc_to_warsaw(uint8_t *yy, uint8_t *mo, uint8_t *dd, uint8_t *wd,
                       uint8_t *hh, uint8_t *mi, uint8_t *ss)
{
    (void)mi; (void)ss;

    uint16_t Y = (uint16_t)(2000U + (uint16_t)(*yy));
    uint8_t  M = *mo, D = *dd, WD = *wd, H = *hh;

    uint8_t dst = warsaw_is_dst_utc(Y, M, D, H);
    uint8_t add = (uint8_t)(1U + dst);

    add_hours(&Y, &M, &D, &WD, &H, add);

    *yy = (uint8_t)(Y - 2000U);
    *mo = M;
    *dd = D;
    *wd = WD;
    *hh = H;
}