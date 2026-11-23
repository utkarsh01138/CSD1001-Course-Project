#include <stdio.h>
#include <string.h>
#include <time.h>
#include "vrsms.h"

// --- Helper Functions ---

// Flush leftover characters from input
void clearInputBuffer()
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
        ;
    if (ch == EOF)
        clearerr(stdin);
}

// Safe string reader to avoid buffer bleed
void readString(char *buffer, int maxLen)
{
    if (fgets(buffer, maxLen, stdin) != NULL)
    {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';
        else
            clearInputBuffer();
    }
}

// Validate calendar dates
int isValidDate(Date d)
{
    if (d.year < 1970 || d.year > 2100)
        return 0;
    if (d.month < 1 || d.month > 12)
        return 0;

    int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int isLeap = (d.year % 4 == 0 && (d.year % 100 != 0 || d.year % 400 == 0));

    if (isLeap && d.month == 2)
    {
        if (d.day < 1 || d.day > 29)
            return 0;
    }
    else
    {
        if (d.day < 1 || d.day > daysInMonth[d.month])
            return 0;
    }

    return 1;
}

// Days between dates with DST protection
int daysBetweenDates(Date d1, Date d2)
{
    struct tm t1 = {0};
    struct tm t2 = {0};

    t1.tm_mday = d1.day;
    t1.tm_mon = d1.month - 1;
    t1.tm_year = d1.year - 1900;
    t1.tm_isdst = -1;

    t2.tm_mday = d2.day;
    t2.tm_mon = d2.month - 1;
    t2.tm_year = d2.year - 1900;
    t2.tm_isdst = -1;

    time_t sec1 = mktime(&t1);
    time_t sec2 = mktime(&t2);

    if (sec1 == (time_t)(-1) || sec2 == (time_t)(-1))
        return 0;

    double diffSec = difftime(sec2, sec1);
    if (diffSec >= 0)
        return (int)((diffSec + 43200.0) / 86400.0);
    else
        return -(int)((-diffSec + 43200.0) / 86400.0);
}

// Half price after 1 year (365 days)
double getEffectiveDailyRate(const MediaItem *item, Date todayDate)
{
    int daysElapsed = daysBetweenDates(item -> procurementDate, todayDate);

    if (daysElapsed >= 365)
        return item -> baseDailyRate / 2.0;
    else
        return item -> baseDailyRate;
}


// --- File Persistence ---

// Save entire store state to binary file
int saveStoreState(const StoreState *store, const char *fileName)
{
    FILE *fp = fopen(fileName, "wb");
    if (fp == NULL)
        return 0;

    size_t written = fwrite(store, sizeof(StoreState), 1, fp);
    fclose(fp);

    return (written == 1) ? 1 : 0;
}

// Load store state from disk
int loadStoreState(StoreState *store, const char *fileName)
{
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL)
        return 0;

    size_t readCount = fread(store, sizeof(StoreState), 1, fp);
    fclose(fp);

    return (readCount == 1) ? 1 : 0;
}


// --- Member Operations ---

// Register a new customer
void registerMember(StoreState *store)
{
    if (store -> memberCount >= MAX_MEMBERS)
    {
        printf("\n[ERROR] Member capacity reached (%d). Cannot register more.\n", MAX_MEMBERS);
        return;
    }

    Member *m = &store -> members[store -> memberCount];
    m -> memberId = store -> memberCount + 1;

    printf("\n--- Register New Member (ID: %d) ---\n", m -> memberId);

    printf("Enter full name: ");
    readString(m -> name, sizeof(m -> name));
    while (strlen(m -> name) == 0)
    {
        printf("Name cannot be empty. Enter full name: ");
        readString(m -> name, sizeof(m -> name));
    }

    printf("Enter Address: ");
    readString(m -> address, sizeof(m -> address));

    printf("Enter Phone Number: ");
    readString(m -> phone, sizeof(m -> phone));

    m -> isActive = 1;
    m -> depositAmount = MEMBERSHIP_DEPOSIT;
    m -> outstandingDues = 0.0;
    m -> borrowedVideoId = 0;
    m -> borrowedMusicId = 0;

    store -> memberCount++;

    printf("\n>>> Member registered successfully!\n");
    printf(">>> Member ID: %d | Name: %s\n", m -> memberId, m -> name);
    printf(">>> Deposit of Rs. %.2f collected and recorded.\n", m -> depositAmount);
}

// Cancel membership and refund deposit
void cancelMembership(StoreState *store)
{
    int memberId;
    printf("\n--- Cancel Membership ---\n");
    printf("Enter Member ID to cancel: ");
    if (scanf("%d", &memberId) != 1)
    {
        printf("Invalid input.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (memberId < 1 || memberId > store -> memberCount)
    {
        printf("[ERROR] Member ID %d not found.\n", memberId);
        return;
    }

    Member *m = &store -> members[memberId - 1];

    if (!m -> isActive)
    {
        printf("[INFO] Member ID %d is already inactive.\n", memberId);
        return;
    }

    // Check for unreturned items
    if (m -> borrowedVideoId != 0 || m -> borrowedMusicId != 0)
    {
        printf("[ERROR] Cannot cancel membership. Member ID %d has unreturned items:\n", memberId);
        if (m -> borrowedVideoId != 0)
            printf(" - Must return Video Item ID %d\n", m -> borrowedVideoId);
        if (m -> borrowedMusicId != 0)
            printf(" - Must return Music CD Item ID %d\n", m -> borrowedMusicId);
        return;
    }

    // Check for outstanding dues
    if (m -> outstandingDues > 0.0)
    {
        printf("[ERROR] Cannot cancel membership. Member ID %d has outstanding dues of Rs. %.2f.\n", memberId, m -> outstandingDues);
        printf("Please settle outstanding dues before cancelling.\n");
        return;
    }

    m -> isActive = 0;
    double refundAmount = m -> depositAmount;
    m -> depositAmount = 0.0;

    printf("\n>>> Membership cancelled for %s (ID: %d).\n", m -> name, m -> memberId);
    printf(">>> Deposit refund of Rs. %.2f handed over to customer.\n", refundAmount);
}

// Settle member outstanding dues
void payMemberDues(StoreState *store)
{
    int memberId;
    printf("\n--- Pay Member Dues ---\n");
    printf("Enter Member ID: ");
    if (scanf("%d", &memberId) != 1)
    {
        printf("Invalid input.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (memberId < 1 || memberId > store -> memberCount)
    {
        printf("[ERROR] Member ID %d not found.\n", memberId);
        return;
    }

    Member *m = &store -> members[memberId - 1];

    if (!m -> isActive)
        printf("[INFO] Member ID %d is inactive.\n", memberId);

    if (m -> outstandingDues <= 0.0)
    {
        printf(">>> Member %s (ID %d) has no outstanding dues.\n", m -> name, m -> memberId);
        return;
    }

    printf("Member Name      : %s\n", m -> name);
    printf("Outstanding Dues : Rs. %.2f\n", m -> outstandingDues);

    double payAmount = 0.0;
    printf("Enter amount to pay (Rs.): ");
    while (scanf("%lf", &payAmount) != 1 || payAmount <= 0.0)
    {
        printf("Invalid amount. Enter a positive value: ");
        clearInputBuffer();
    }
    clearInputBuffer();

    if (payAmount > m -> outstandingDues)
        payAmount = m -> outstandingDues;

    m -> outstandingDues -= payAmount;
    store -> totalRentalIncome += payAmount;

    printf("\n>>> Payment of Rs. %.2f accepted.\n", payAmount);
    printf(">>> Remaining outstanding dues: Rs. %.2f\n", m -> outstandingDues);
}

// Show all registered members
void displayMembers(const StoreState *store)
{
    printf("\n========================================================================================\n");
    printf("                                  REGISTERED MEMBERS                                   \n");
    printf("========================================================================================\n");
    printf("%-5s | %-20s | %-14s | %-8s | %-7s | %-7s | %-10s\n",
           "ID", "Name", "Phone", "Status", "Video", "Music", "Dues (Rs)");
    printf("----------------------------------------------------------------------------------------\n");

    if (store -> memberCount == 0)
        printf("                       No members registered yet.                                       \n");

    for (int i = 0; i < store -> memberCount; i++)
    {
        const Member *m = &store -> members[i];
        printf("%-5d | %-20.20s | %-14.14s | %-8s | %-7d | %-7d | %-10.2f\n",
               m -> memberId,
               m -> name,
               m -> phone,
               m -> isActive ? "Active" : "Inactive",
               m -> borrowedVideoId,
               m -> borrowedMusicId,
               m -> outstandingDues);
    }
    printf("========================================================================================\n");
}


// --- Inventory Operations ---

static const char* getCategoryName(MediaCategory category)
{
    switch (category)
    {
        case MEDIA_VIDEO:    return "Video";
        case MEDIA_MUSIC_CD: return "Music CD";
        default:             return "Unknown";
    }
}

static const char* getFormatName(MediaFormat format)
{
    switch (format)
    {
        case FORMAT_VHS:      return "VHS";
        case FORMAT_MP4:      return "MP4";
        case FORMAT_DVD:      return "DVD";
        case FORMAT_VCD:      return "VCD";
        case FORMAT_AUDIO_CD: return "Audio CD";
        default:              return "Unknown";
    }
}

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

// Procure a new video or music cd
void procureNewItem(StoreState *store, Date todayDate)
{
    if (store -> inventoryCount >= MAX_INVENTORY)
    {
        printf("\n[ERROR] Inventory is full (%d items). Cannot procure more.\n", MAX_INVENTORY);
        return;
    }

    MediaItem *item = &store -> inventory[store -> inventoryCount];
    item -> itemId = store -> inventoryCount + 1;

    printf("\n--- Procure New Media Item (ID: %d) ---\n", item -> itemId);

    printf("Enter Title: ");
    readString(item -> title, sizeof(item -> title));
    while (strlen(item -> title) == 0)
    {
        printf("Title cannot be empty. Enter Title: ");
        readString(item -> title, sizeof(item -> title));
    }

    // Category selection
    int categoryChoice = 0;
    while (categoryChoice != 1 && categoryChoice != 2)
    {
        printf("Select Category (1. Video CD/DVD, 2. Music CD): ");
        if (scanf("%d", &categoryChoice) != 1 || (categoryChoice != 1 && categoryChoice != 2))
        {
            printf("[ERROR] Invalid input. Please enter 1 or 2.\n");
            clearInputBuffer();
            categoryChoice = 0;
            continue;
        }
        clearInputBuffer();
    }
    item -> category = (MediaCategory)categoryChoice;

    // Format selection
    if (item -> category == MEDIA_VIDEO)
    {
        int formatChoice = 0;
        while (formatChoice < 1 || formatChoice > 4)
        {
            printf("Select Video Format (1=VHS, 2=MP4, 3=DVD, 4=VCD): ");
            if (scanf("%d", &formatChoice) != 1 || formatChoice < 1 || formatChoice > 4)
            {
                printf("Invalid selection. Please enter 1, 2, 3, or 4.\n");
                clearInputBuffer();
                formatChoice = 0;
                continue;
            }
            clearInputBuffer();
        }
        item -> format = (MediaFormat)formatChoice;
    }
    else
    {
        item -> format = FORMAT_AUDIO_CD;
        printf("Format automatically set to Audio CD.\n");
    }

    // Date of procurement entry
    int dateChoice = 0;
    printf("Procurement Date:\n");
    printf("1. Use current date (%02d/%02d/%04d)\n", todayDate.day, todayDate.month, todayDate.year);
    printf("2. Enter specific procurement date\n");
    printf("Choice (1 or 2): ");
    while (scanf("%d", &dateChoice) != 1 || (dateChoice != 1 && dateChoice != 2))
    {
        printf("Please enter 1 or 2: ");
        clearInputBuffer();
    }
    clearInputBuffer();

    if (dateChoice == 1)
        item -> procurementDate = todayDate;
    else
    {
        Date customDate = {0};
        while (1)
        {
            printf("Enter procurement date (DD MM YYYY): ");
            if (scanf("%d %d %d", &customDate.day, &customDate.month, &customDate.year) == 3 && isValidDate(customDate))
            {
                clearInputBuffer();
                item -> procurementDate = customDate;
                break;
            }
            printf("Invalid calendar date. Please try again.\n");
            clearInputBuffer();
        }
    }

    // Purchase price
    printf("Enter purchase price paid by store (Rs.): ");
    while (scanf("%lf", &item -> purchasePrice) != 1 || item -> purchasePrice <= 0.0)
    {
        printf("Invalid price. Please enter a positive amount: ");
        clearInputBuffer();
    }
    clearInputBuffer();

    // Daily rental rate
    printf("Enter Daily Rental Charge (Rs.): ");
    while (scanf("%lf", &item -> baseDailyRate) != 1 || item -> baseDailyRate <= 0.0)
    {
        printf("Invalid rental rate. Enter a positive amount: ");
        clearInputBuffer();
    }
    clearInputBuffer();

    item -> status = ITEM_AVAILABLE;
    item -> everIssued = 0;
    item -> lastActivityDate = item -> procurementDate;
    item -> loanedToMemberId = 0;
    item -> loanIssueDate = (Date){0, 0, 0};
    item -> lockedDailyRent = 0.0;

    store -> totalProcurementCost += item -> purchasePrice;
    store -> inventoryCount++;

    printf("\n>>> Item procured successfully.\n");
    printf(">>> ID: %d | Title: '%s' | Category: %s | Format: %s\n",
           item -> itemId,
           item -> title,
           getCategoryName(item -> category),
           getFormatName(item -> format));
    printf(">>> Procured On: %02d/%02d/%04d | Purchase Cost: Rs. %.2f | Daily Rent: Rs. %.2f\n",
           item -> procurementDate.day, item -> procurementDate.month, item -> procurementDate.year,
           item -> purchasePrice, item -> baseDailyRate);
}

// Show current store inventory
void displayInventory(const StoreState *store, Date todayDate)
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
        double currentRate = getEffectiveDailyRate(item, todayDate);

        printf("%-5d | %-24.24s | %-9s | %-8s | %02d/%02d/%04d | %-9.2f | %-9.2f | %-10.2f | %-13s\n",
               item -> itemId,
               item -> title,
               getCategoryName(item -> category),
               getFormatName(item -> format),
               item -> procurementDate.day, item -> procurementDate.month, item -> procurementDate.year,
               item -> purchasePrice,
               item -> baseDailyRate,
               currentRate,
               getStatusName(item -> status));
    }
    printf("========================================================================================================================\n");
}


// --- Rental & Return Operations ---

// Borrow a media item (1 video, 1 music cd)
void issueItem(StoreState *store, Date todayDate)
{
    int memberId, itemId;

    printf("\n--- Issue Media Item (Borrow) ---\n");

    printf("Enter Member ID: ");
    if (scanf("%d", &memberId) != 1)
    {
        printf("Invalid input.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (memberId < 1 || memberId > store -> memberCount)
    {
        printf("[ERROR] Member ID %d not found.\n", memberId);
        return;
    }

    Member *m = &store -> members[memberId - 1];
    if (!m -> isActive)
    {
        printf("[ERROR] Member %s (ID %d) is inactive/cancelled. Cannot borrow.\n", m -> name, m -> memberId);
        return;
    }

    if (m -> outstandingDues > 0.0)
        printf("[WARNING] Member has outstanding dues of Rs. %.2f.\n", m -> outstandingDues);

    printf("Enter Media Item ID to borrow: ");
    if (scanf("%d", &itemId) != 1)
    {
        printf("Invalid input.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (itemId < 1 || itemId > store -> inventoryCount)
    {
        printf("[ERROR] Media Item ID %d does not exist.\n", itemId);
        return;
    }

    MediaItem *item = &store -> inventory[itemId - 1];
    if (item -> status != ITEM_AVAILABLE)
    {
        printf("[ERROR] Item '%s' (ID %d) is not available for borrowing. Current Status: %s\n",
               item -> title, item -> itemId, getStatusName(item -> status));
        return;
    }

    // Limit check: 1 video CD / DVD and 1 music CD
    if (item -> category == MEDIA_VIDEO)
    {
        if (m -> borrowedVideoId != 0)
        {
            printf("\n[BORROW LIMIT REACHED] Member %s (ID %d) already has a borrowed Video (ID: %d).\n",
                   m -> name, m -> memberId, m -> borrowedVideoId);
            printf("Members can hold at most 1 Video item at a time. Please return it first.\n");
            return;
        }
    }
    else if (item -> category == MEDIA_MUSIC_CD)
    {
        if (m -> borrowedMusicId != 0)
        {
            printf("\n[BORROW LIMIT REACHED] Member %s (ID %d) already has a borrowed Music CD (ID: %d).\n",
                   m -> name, m -> memberId, m -> borrowedMusicId);
            printf("Members can hold at most 1 Music CD at a time. Please return it first.\n");
            return;
        }
    }

    item -> lockedDailyRent = getEffectiveDailyRate(item, todayDate);
    item -> status = ITEM_LOANED;
    item -> loanedToMemberId = m -> memberId;
    item -> loanIssueDate = todayDate;
    item -> everIssued = 1;
    item -> lastActivityDate = todayDate;

    if (item -> category == MEDIA_VIDEO)
        m -> borrowedVideoId = item -> itemId;
    else
        m -> borrowedMusicId = item -> itemId;

    // Print receipt
    printf("\n==================================================\n");
    printf("                 RENTAL RECEIPT                   \n");
    printf("==================================================\n");
    printf("Receipt Date    : %02d/%02d/%04d\n", todayDate.day, todayDate.month, todayDate.year);
    printf("Member ID       : %d\n", m -> memberId);
    printf("Member Name     : %s\n", m -> name);
    printf("Item ID         : %d\n", item -> itemId);
    printf("Title           : %s\n", item -> title);
    printf("Category        : %s (%s)\n", getCategoryName(item -> category), getFormatName(item -> format));
    printf("Daily Rental    : Rs. %.2f / day\n", item -> lockedDailyRent);
    printf("Deposit Held    : Rs. %.2f\n", m -> depositAmount);
    printf("==================================================\n");

    // Save receipt text file
    char receiptFileName[64];
    sprintf(receiptFileName, "receipt_item%d_member%d.txt", item -> itemId, m -> memberId);
    FILE *rfp = fopen(receiptFileName, "w");
    if (rfp != NULL)
    {
        fprintf(rfp, "==================================================\n");
        fprintf(rfp, "                 RENTAL RECEIPT                   \n");
        fprintf(rfp, "==================================================\n");
        fprintf(rfp, "Receipt Date    : %02d/%02d/%04d\n", todayDate.day, todayDate.month, todayDate.year);
        fprintf(rfp, "Member ID       : %d\n", m -> memberId);
        fprintf(rfp, "Member Name     : %s\n", m -> name);
        fprintf(rfp, "Item ID         : %d\n", item -> itemId);
        fprintf(rfp, "Title           : %s\n", item -> title);
        fprintf(rfp, "Category        : %s (%s)\n", getCategoryName(item -> category), getFormatName(item -> format));
        fprintf(rfp, "Daily Rental    : Rs. %.2f / day\n", item -> lockedDailyRent);
        fprintf(rfp, "Deposit Held    : Rs. %.2f\n", m -> depositAmount);
        fprintf(rfp, "==================================================\n");
        fclose(rfp);
        printf(">>> Receipt saved to file '%s'.\n", receiptFileName);
    }
}

// Return or report lost / damaged item
void returnOrReportItem(StoreState *store, Date todayDate)
{
    int itemId;

    printf("\n--- Return / Report Lost or Damaged Item ---\n");
    printf("Enter Media Item ID: ");
    if (scanf("%d", &itemId) != 1)
    {
        printf("Invalid input.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (itemId < 1 || itemId > store -> inventoryCount)
    {
        printf("[ERROR] Media Item ID %d does not exist.\n", itemId);
        return;
    }

    MediaItem *item = &store -> inventory[itemId - 1];
    if (item -> status != ITEM_LOANED)
    {
        printf("[ERROR] Item '%s' (ID %d) is not currently on loan. Current Status: %s\n",
               item -> title, item -> itemId, getStatusName(item -> status));
        return;
    }

    Member *m = &store -> members[item -> loanedToMemberId - 1];

    int daysBorrowed = daysBetweenDates(item -> loanIssueDate, todayDate);
    if (daysBorrowed < 1)
        daysBorrowed = 1;

    double rentalDue = daysBorrowed * item -> lockedDailyRent;

    printf("\n>>> Loan Record Found:\n");
    printf(" - Borrower    : %s (Member ID: %d)\n", m -> name, m -> memberId);
    printf(" - Item        : '%s' (Item ID: %d)\n", item -> title, item -> itemId);
    printf(" - Issue Date  : %02d/%02d/%04d\n", item -> loanIssueDate.day, item -> loanIssueDate.month, item -> loanIssueDate.year);
    printf(" - Return Date : %02d/%02d/%04d (%d day%s)\n", todayDate.day, todayDate.month, todayDate.year, daysBorrowed, daysBorrowed == 1 ? "" : "s");
    printf(" - Locked Rate : Rs. %.2f / day\n", item -> lockedDailyRent);
    printf(" - Rental Due  : Rs. %.2f\n", rentalDue);

    int conditionChoice = 0;
    while (conditionChoice != 1 && conditionChoice != 2)
    {
        printf("\nSelect Return Condition:\n");
        printf("1. Item returned in good condition\n");
        printf("2. Item was LOST or DAMAGED\n");
        printf("Choice (1 or 2): ");
        if (scanf("%d", &conditionChoice) != 1 || (conditionChoice != 1 && conditionChoice != 2))
        {
            printf("Invalid selection. Please enter 1 or 2.\n");
            clearInputBuffer();
            conditionChoice = 0;
            continue;
        }
        clearInputBuffer();
    }

    if (conditionChoice == 1)
    {
        // Safe return
        printf("\nTotal Rental Amount Due: Rs. %.2f\n", rentalDue);
        printf("Enter amount paid now by member (Enter %.2f for full payment): ", rentalDue);

        double amountPaid = 0.0;
        while (scanf("%lf", &amountPaid) != 1 || amountPaid < 0.0)
        {
            printf("Invalid amount. Enter a non-negative amount: ");
            clearInputBuffer();
        }
        clearInputBuffer();

        if (amountPaid > rentalDue)
        {
            printf(">>> Change returned to customer: Rs. %.2f\n", amountPaid - rentalDue);
            amountPaid = rentalDue;
        }

        store -> totalRentalIncome += amountPaid;

        double unpaidAmount = rentalDue - amountPaid;
        if (unpaidAmount > 0.0)
        {
            m -> outstandingDues += unpaidAmount;
            printf(">>> Unpaid balance of Rs. %.2f recorded to member's dues.\n", unpaidAmount);
        }

        item -> status = ITEM_AVAILABLE;
        item -> loanedToMemberId = 0;
        item -> lastActivityDate = todayDate;
        item -> lockedDailyRent = 0.0;

        if (item -> category == MEDIA_VIDEO)
            m -> borrowedVideoId = 0;
        else
            m -> borrowedMusicId = 0;

        printf("\n==================================================\n");
        printf("               RETURN SETTLEMENT                  \n");
        printf("==================================================\n");
        printf("Member Name     : %s\n", m -> name);
        printf("Item Title      : %s\n", item -> title);
        printf("Days Borrowed   : %d\n", daysBorrowed);
        printf("Total Due       : Rs. %.2f\n", rentalDue);
        printf("Amount Paid     : Rs. %.2f\n", amountPaid);
        printf("Remaining Dues  : Rs. %.2f\n", m -> outstandingDues);
        printf("Status          : Item returned to shelf.\n");
        printf("==================================================\n");
    }
    else
    {
        // Lost or damaged policy: full price charged + rental fee
        double replacementCost = item -> purchasePrice;
        double totalDue = rentalDue + replacementCost;

        printf("\n==================================================\n");
        printf("           LOSS / DAMAGE PENALTY DUE              \n");
        printf("==================================================\n");
        printf("Rental Charge   : Rs. %.2f (%d day%s)\n", rentalDue, daysBorrowed, daysBorrowed == 1 ? "" : "s");
        printf("Replacement Fee : Rs. %.2f (100%% of purchase price)\n", replacementCost);
        printf("Total Charge    : Rs. %.2f\n", totalDue);
        printf("--------------------------------------------------\n");

        printf("Enter amount paid now by member (Enter %.2f for full payment): ", totalDue);
        double amountPaid = 0.0;
        while (scanf("%lf", &amountPaid) != 1 || amountPaid < 0.0)
        {
            printf("Invalid amount. Enter a non-negative amount: ");
            clearInputBuffer();
        }
        clearInputBuffer();

        if (amountPaid > totalDue)
        {
            printf(">>> Change returned to customer: Rs. %.2f\n", amountPaid - totalDue);
            amountPaid = totalDue;
        }

        if (amountPaid <= rentalDue)
            store -> totalRentalIncome += amountPaid;
        else
        {
            store -> totalRentalIncome += rentalDue;
            store -> totalPenaltyIncome += (amountPaid - rentalDue);
        }

        double unpaidAmount = totalDue - amountPaid;
        if (unpaidAmount > 0.0)
        {
            m -> outstandingDues += unpaidAmount;
            printf(">>> Unpaid balance of Rs. %.2f recorded to member's dues.\n", unpaidAmount);
        }

        item -> status = ITEM_REMOVED_LOST_DAMAGED;
        item -> loanedToMemberId = 0;
        item -> lastActivityDate = todayDate;
        item -> lockedDailyRent = 0.0;

        if (item -> category == MEDIA_VIDEO)
            m -> borrowedVideoId = 0;
        else
            m -> borrowedMusicId = 0;

        printf("\n==================================================\n");
        printf("           LOSS / DAMAGE INVOICE SUMMARY          \n");
        printf("==================================================\n");
        printf("Member Name     : %s\n", m -> name);
        printf("Item Title      : %s (Item ID: %d)\n", item -> title, item -> itemId);
        printf("Total Charge    : Rs. %.2f\n", totalDue);
        printf("Amount Paid     : Rs. %.2f\n", amountPaid);
        printf("Remaining Dues  : Rs. %.2f\n", m -> outstandingDues);
        printf("Status          : Item REMOVED from inventory.\n");
        printf("==================================================\n");
    }
}


// --- Clearance & Accounts ---

// 1-year unissued stock clearance sale at 10%
void runUnissuedClearance(StoreState *store, Date todayDate)
{
    printf("\n========================================================================================\n");
    printf("                       1-YEAR UNISSUED CLEARANCE SCAN (> 365 DAYS)                     \n");
    printf("========================================================================================\n");

    int eligibleCount = 0;
    int eligibleIds[MAX_INVENTORY];

    for (int i = 0; i < store -> inventoryCount; i++)
    {
        MediaItem *item = &store -> inventory[i];
        if (item -> status == ITEM_AVAILABLE)
        {
            int daysIdle = daysBetweenDates(item -> lastActivityDate, todayDate);
            if (daysIdle > 365)
            {
                eligibleIds[eligibleCount++] = item -> itemId;
                printf("[%d] ID %-4d | '%-22.22s' | Cost: Rs.%-7.2f | Idle: %d days | Clearance Price (10%%): Rs.%.2f\n",
                       eligibleCount,
                       item -> itemId,
                       item -> title,
                       item -> purchasePrice,
                       daysIdle,
                       item -> purchasePrice * 0.10);
            }
        }
    }

    if (eligibleCount == 0)
    {
        printf(">>> No items in inventory have been unissued for more than a year.\n");
        return;
    }

    printf("----------------------------------------------------------------------------------------\n");
    printf("Found %d eligible item(s) for clearance sale.\n", eligibleCount);

    int targetItemId;
    printf("Enter Media Item ID to sell at 10%% (or 0 to cancel): ");
    if (scanf("%d", &targetItemId) != 1)
    {
        clearInputBuffer();
        printf("Clearance operation cancelled.\n");
        return;
    }
    clearInputBuffer();

    if (targetItemId == 0)
    {
        printf("Clearance operation cancelled.\n");
        return;
    }

    int isEligible = 0;
    for (int i = 0; i < eligibleCount; i++)
        if (eligibleIds[i] == targetItemId)
        {
            isEligible = 1;
            break;
        }

    if (!isEligible)
    {
        printf("[ERROR] Item ID %d is not eligible for 10%% clearance sale.\n", targetItemId);
        return;
    }

    int buyerMemberId;
    printf("Enter purchasing Member ID: ");
    if (scanf("%d", &buyerMemberId) != 1)
    {
        printf("Invalid input.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (buyerMemberId < 1 || buyerMemberId > store -> memberCount)
    {
        printf("[ERROR] Member ID %d does not exist.\n", buyerMemberId);
        return;
    }

    Member *buyer = &store -> members[buyerMemberId - 1];
    if (!buyer -> isActive)
    {
        printf("[ERROR] Member %s (ID %d) is inactive. Only active members can buy clearance stock.\n", buyer -> name, buyer -> memberId);
        return;
    }

    MediaItem *targetItem = &store -> inventory[targetItemId - 1];
    double salePrice = targetItem -> purchasePrice * 0.10;

    store -> totalLiquidationSales += salePrice;
    targetItem -> status = ITEM_REMOVED_SOLD_LIQUIDATED;
    targetItem -> lastActivityDate = todayDate;

    printf("\n==================================================\n");
    printf("             CLEARANCE SALE RECEIPT               \n");
    printf("==================================================\n");
    printf("Sale Date       : %02d/%02d/%04d\n", todayDate.day, todayDate.month, todayDate.year);
    printf("Buyer Name      : %s (Member ID: %d)\n", buyer -> name, buyer -> memberId);
    printf("Item Sold       : '%s' (Item ID: %d)\n", targetItem -> title, targetItem -> itemId);
    printf("Original Cost   : Rs. %.2f\n", targetItem -> purchasePrice);
    printf("Discount        : 90%% OFF\n");
    printf("Final Price Paid: Rs. %.2f (10%% of purchase price)\n", salePrice);
    printf("Status          : Item SOLD and REMOVED from inventory.\n");
    printf("==================================================\n");
}

// Store profit and loss account
void displayProfitLoss(const StoreState *store)
{
    printf("\n========================================================\n");
    printf("           VIDEO RENTAL STORE - PROFIT & LOSS           \n");
    printf("========================================================\n");

    // Inflows
    printf(" REVENUE / CASH INFLOWS:\n");
    printf("  + Rental Income Collected          : Rs. %10.2f\n", store -> totalRentalIncome);
    printf("  + Lost / Damaged Recoveries (100%%) : Rs. %10.2f\n", store -> totalPenaltyIncome);
    printf("  + Liquidation Clearance Sales (10%%): Rs. %10.2f\n", store -> totalLiquidationSales);
    printf(" -------------------------------------------------------\n");

    double totalInflow = store -> totalRentalIncome +
                         store -> totalPenaltyIncome +
                         store -> totalLiquidationSales;
    printf("  TOTAL REVENUE EARNED               : Rs. %10.2f\n\n", totalInflow);

    // Outflows
    printf(" EXPENSES / CASH OUTFLOWS:\n");
    printf("  - Total Media Procurement Cost     : Rs. %10.2f\n", store -> totalProcurementCost);
    printf(" -------------------------------------------------------\n");

    double totalOutflow = store -> totalProcurementCost;
    printf("  TOTAL CAPITAL EXPENDITURE          : Rs. %10.2f\n\n", totalOutflow);

    // Net profit or loss
    printf("========================================================\n");
    double netBalance = totalInflow - totalOutflow;
    if (netBalance >= 0.0)
        printf("  NET POSITION : NET PROFIT of Rs. %.2f\n", netBalance);
    else
        printf("  NET POSITION : NET LOSS of Rs. %.2f\n", -netBalance);
    printf("========================================================\n");

    // Deposits are refundable so they are liabilities, not store profit
    int activeMembers = 0;
    double totalOutstandingDues = 0.0;
    for (int i = 0; i < store -> memberCount; i++)
    {
        if (store -> members[i].isActive)
            activeMembers++;
        totalOutstandingDues += store -> members[i].outstandingDues;
    }

    double totalDepositsHeld = activeMembers * MEMBERSHIP_DEPOSIT;
    printf("\n [LIABILITY NOTE]\n");
    printf("  Active Customer Deposits Held      : Rs. %.2f (%d active members)\n",
           totalDepositsHeld, activeMembers);
    printf("  (Refundable deposits are liabilities and excluded from profit/loss)\n");

    printf("\n [RECEIVABLES NOTE]\n");
    printf("  Total Outstanding Member Dues      : Rs. %.2f\n", totalOutstandingDues);
    printf("========================================================\n");
}
