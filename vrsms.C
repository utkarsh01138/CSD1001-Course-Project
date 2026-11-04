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

/* ========================================
   4. INVENTORY MANAGEMENT FUNCTIONS
   ======================================== */

// Helper to convert MediaCategory enum to string for display
static const char* getCategoryName(MediaCategory category)
{
    switch (category)
    {
        case MEDIA_VIDEO:    return "Video";
        case MEDIA_MUSIC_CD: return "Music CD";
        default:             return "Unknown";
    }
}

// Helper to convert MediaFormat enum to string for display
static const char* getFormatName(MediaFormat format)
{
    switch (format)
    {
        case FORMAT_VHS:      return "VHS";
        case FORMAT_MP4:      return "MP4";
        case FORMAT_AUDIO_CD: return "Audio CD";
        default:              return "Unknown";
    }
}

// Helper to convert ItemStatus enum to string for display
static const char* getStatusName(ItemStatus status)
{
    switch (status)
    {
        case ITEM_AVAILABLE:               return "Available";
        case ITEM_LOANED:                  return "Loaned";
        case ITEM_REMOVED_LOST_DAMAGED:    return "Lost/Damaged";
        case ITEM_REMOVED_SOLD_LIQUIDATED: return "Sold (10%)";
        default:                           return "Unknown";
    }
}

// Procures a new item, sets purchase cost & daily rent, and logs procurement expense
void procureNewItem(StoreState *store, Date today)
{
    if (store -> inventoryCount >= MAX_INVENTORY)
    {
        printf("\n[ERROR] Inventory is full (%d items). Cannot procure more. \n", MAX_INVENTORY);
        return;
    }

    MediaItem *item = &store -> inventory[store -> inventoryCount];
    item -> itemID = store -> inventoryCount++; // Auto-increment ID

    printf("\n--- Procure New media Item (ID: %d) ---\n", item -> itemID);

    printf("Enter Title: ");
    fgets(item -> title, sizeof(item -> title), stdin);
    stripNewline(item -> title);

    // 1. Select Category
    int categoryChoice = 0;
    while (categoryChoice != 1 && categoryChoice != 2)
    {
        printf("Select Category (1. Video CD/DVD, 2. Music CD): ");
        if (scanf("%d", &categoryChoice) != 1 || (categoryChoice != 1 && categoryChoice != 2))
        {
            printf("[ERROR] Invalid input. Please enter 1 or 2.\n");
            clearInputBuffer();
            categoryChoice = 0; // Reset to loop again
            continue;
        }
        clearInputBuffer();
    }
    item -> category = (MediaCategory)categoryChoice;

    // 2. Select Format
    if (item -> category == MEDIA_VIDEO)
    {
        int formatChoice = 0;
        while (formatChoice != 1 && formatChoice != 2)
        {
            printf("Select Video Format (1=VHS, 2 = MP4): ");
            if (scanf("%d", &formatChoice) != 1 || (formatChoice != 1 && formatChoice != 2))
            {
                printf("Invalid selection. Please enter 1 or 2. \n");
                clearInputBuffer();
                formatChoice = 0; // Reset to loop again
                continue;
            }
            clearInputBuffer();
        }
        item -> format = (MediaFormat)formatChoice;
    } 
    else
    {
        // Music CDS are always standard Audio CD format
        item -> format = FORMAT_AUDIO_CD;
        printf("Format automatically set to Audio CD.\n");
    }

    // 3. Purchase Price
    printf("Enter purchase price paid by store (Rs.):");
    while (scanf("%lf", &item -> purchasePrice) != 1 || item -> purchasePrice <= 0.0)
    {
        printf("Invalid price. Please enter a positive amount: ");
        clearInputBuffer();
    }
    clearInputBuffer();

    // 4. Daily Rental Charge
    printf("Enter Daily Rental Charge (Rs.): ");
    while (scanf("%lf", &item -> baseDailyRate) != 1 || item -> baseDailyRate <= 0.0)
    {
        printf("Invalid rental rate. Enter a positive amount: ");
        clearInputBuffer();
    }
    clearInputBuffer();

    // 5. Initialise lifecycle & loan tracking
    item -> procurementDate = today;
    item -> status = ITEM_AVAILABLE;
    item -> everIssued = 0;
    item -> lastActivityDate = today;
    item -> loanedToMemberID = 0;
    item -> lockedDailyRent = 0.0;

    // Outflow: Update store's procurement expense ledger
    store -> totalProcurementCost += item -> purchasePrice;
    store -> inventoryCount++;

    printf("\n>>> Item procured sucessfully. \n");
    printf(">>> ID: %d | Title: '%s' | Category: %s | Format: %s\n", 
           item -> itemID, 
           item -> title, 
           getCategoryName(item -> category), 
           getFormatName(item -> format));
    printf(">> Procured On: %02d/%02d/%04d | Purchase Cost: Rs. %.2f | Daily Rent: Rs. %.2f\n",
           today.day, today.month, today.year, item -> purchasePrice, item -> baseDailyRate);
}

// Displays inventory including both original and effective (current) rate
void displayInventory(const StoreState *store, Date today)
{
    printf("\n========================================================================================================================\n");
    printf("                                                     STORE INVENTORY                                                    \n");
    printf("========================================================================================================================\n");

    printf("%-5s | %-24s | %-9s | %-8s | %-10s | %-9s | %-9s | %-10s | %-13s\n",
           "ID", "Title", "Category", "Format", "Purchased", "Cost(Rs)", "Base Rent", "Current Rent", "Status");
    printf("------------------------------------------------------------------------------------------------------------------------\n");
    
    if (store -> inventoryCount == 0)
        printf("                                          No items in inventory yet.                                                    \n");

    for (int i = 0; i < store -> inventoryCount; i++)
    {
        const MediaItem *item = &store -> inventory[i];
        double currentRate = getEffectiveDailyRate(item, today);

        printf("%-5d | %-24.24s | %-9s | %-8s | %02d/%02d/%04d | %-9.2f | %-9.2f | %-10.2f | %-13s\n",
               item->itemID,
               item->title,
               getCategoryName(item->category),
               getFormatName(item->format),
               item->procurementDate.day, item->procurementDate.month, item->procurementDate.year,
               item->purchasePrice,
               item->baseDailyRate,
               currentRate,
               getStatusName(item->status));
    }
    printf("========================================================================================================================\n");
}