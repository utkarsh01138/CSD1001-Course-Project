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
/* ========================================
    2. Binary File Persistence Functions (Save & Load)
======================================== */

int saveStoreState(const StoreState *store, const char *fileName)
{
    FILE *fp = fopen(fileName, "wb");
    if (fp == NULL)
    {
        return 0; // Failed to create or open the file
    }

    size_t written = fwrite(store, sizeof(StoreState), 1, fp);
    fclose(fp);

    return (written == 1) ? 1 : 0; // Return 1 if successful, 0 if not
}

int loadStoreState(StoreState *store, const char *fileName)
{
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
        return 0; // File does not exist yet (first run)
    }

    size_t read = fread(store, sizeof(StoreState), 1, fp);
    fclose(fp);

    return (read == 1) ? 1 : 0; // Return 1 if successful, 0 if not
}


/* ========================================
    3. Member Management Functions
======================================== */

// Helper to remove trailing '\n' added by fgets()
static void stripNewline(char *str)
{
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
    {
        str[len - 1] = '\0';
    }
}

// Registers a new customer with a mandatory Rs. 1000 deposit
void registerMember(StoreState *store)
{
    if (store -> memberCount >= MAX_MEMBERS)
    {
        printf("\n[ERROR] Member capacity reached (%d). Cannot regiseter more. \n", MAX_MEMBERS);
        return;
    }

    Member *m = &store -> members[store -> memberCount];
    m -> memberID = store -> memberCount + 1; // Auto-increment ID

    printf("\n--- Register New Member (ID: %d) ---\n", m -> memberID);

    printf("Enter full name: ");
    fgets(m -> name, sizeof(m -> name), stdin);
    stripNewline(m -> name);

    printf("Enter Address: ");
    fgets(m->address, sizeof(m->address), stdin);
    stripNewline(m->address);

    printf("Enter Phone Number: ");
    fgets(m->phone, sizeof(m->phone), stdin);
    stripNewline(m->phone);

    m->isActive = 1;
    m->depositAmount = MEMBERSHIP_DEPOSIT; // Rs. 1000.0
    m->outstandingDues = 0.0;
    m->borrowedVideoID = 0;
    m->borrowedMusicID = 0;

    store->memberCount++;
    printf("\n>>> Member registered successfully!\n");
    printf(">>> Member ID: %d | Name: %s\n", m->memberID, m->name);
    printf(">>> Deposit of Rs. %.2f collected and recorded.\n", m->depositAmount);
}

// Cancels membership & refunds deposit iff dues == 0 and no loans active
void cancelMembership(StoreState *store)
{
    int ID;
    printf("\n-- Cancel Membership ---\n");
    printf("Enter Member ID to cancel: ");
    if (scanf("%d", &ID) != 1)
    {
        printf("Invalid input. \n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (ID < 1 || ID > store -> memberCount)
    {
        printf("[ERROR] Member ID %d not found. \n", ID);
        return;
    }

    Member *m = &store -> members[ID - 1];

    if (!m -> isActive)
    {
        printf("[INFO] Member ID %d is already cancelled/inactive. \n", ID);
        return;
    }

    // Edge Case 1: Unreturned Items
    if (m -> borrowedVideoID != 0 || m -> borrowedMusicID != 0)
    {
        printf("[ERROR] Cannot cancel membership. Member ID %d has unreturned items.\n", ID);
        if (m -> borrowedVideoID != 0)
            printf(" - Must return Video Item ID %d\n", m -> borrowedVideoID);
        if (m -> borrowedMusicID != 0)
            printf(" - Must return Music Item ID %d\n", m -> borrowedMusicID);
        return;
    }

    // Edge Case 2: Outstanding Financial Dues
    if (m -> outstandingDues > 0.0)
    {
        printf("[ERROR] Cannot cancel membership. Member ID %d has outstanding dues of Rs. %.2f\n", ID, m -> outstandingDues);
        return;
    }

    // Both checks passed: Process the cancellation and refund the deposit
    m -> isActive = 0; // Mark as inactive
    printf("\n>>> Membership cancelled for %s (ID: %d). \n", m -> name, m -> memberID);
    printf(">>> Deposit refund of Rs. %.2f handed over to customer. \n", m -> depositAmount);
}

// Displays a formatted table of all registered members
void displayMembers(const StoreState *store)
{
    printf("\n========================================================================================\n");
    printf("                                  REGISTERED MEMBERS                                   \n");
    printf("========================================================================================\n");
    printf("%-5s | %-20s | %-12s | %-8s | %-7s | %-7s | %-10s\n",
           "ID", "Name", "Phone", "Status", "Video", "Music", "Dues (Rs)");
    printf("----------------------------------------------------------------------------------------\n");

    if (store->memberCount == 0) 
        printf("                       No members registered yet.                                      \n");
    
    for (int i = 0; i < store -> memberCount; i++)
    {
        const Member *m = &store -> members[i];
        printf("%-5d | %-20s | %-12s | %-8s | %-7d | %-7d | %-10.2f\n",
               m -> memberID,
               m -> name,
               m -> phone,
               m -> isActive ? "Active" : "Inactive",
               m -> borrowedVideoID,
               m -> borrowedMusicID,
               m -> outstandingDues);
    }
    printf("========================================================================================\n");


}