#include <stdio.h>
#include <string.h>
#include <time.h>
#include "vrsms.h"

/* ========================================
    1. Utility & Helper Functions
======================================== */

// Clears leftover characters & newlines from stdin
void clearInputBuffer() 
{
    int c;
    while((c = getchar()) != '\n' && c != EOF);
}

// Calculates exact days between two dates using <time.h>
int daysBetweenDates(Date d1, Date d2)
{
    struct tm t1 = {0};
    struct tm t2 = {0};

    t1.tm_mday = d1.day;
    t1.tm_mon = d1.month - 1; // tm_mon is 0-based
    t1.tm_year = d1.year - 1900; // tm_year is years since 1900

    t2.tm_mday = d2.day;
    t2.tm_mon = d2.month - 1;
    t2.tm_year = d2.year - 1900;

    time_t sec1 = mktime(&t1);
    time_t sec2 = mktime(&t2);

    return (int)difftime(sec2, sec1) / 86400.0; // 86400 seconds in a day
}

// Dynamically calculates current daily rate (halved after 365 days)
double getEffectiveDailyRate(const MediaItem *item, Date today)
{
    int daysElapsed = daysBetweenDates(item->procurementDate, today);

    if (daysElapsed >= 365)
    {
        return item->baseDailyRate / 2.0;
    }
    else
    {
        return item->baseDailyRate;
    }
}