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

/* ========================================
   5. RENTAL & RETURN OPERATIONS
   ======================================== */

// Issues a media item to a member, enforcing the 1-item-per-category rule 
void issueItem(StoreState *store, Date today)
{
    int memberID, itemID;

    printf("\n--- Issue media Item (Borrow) ---\n");

    // 1. Identify & Validate the Member
    printf("Enter member ID: ");
    if (scanf("%d", &memberID) != 1)
    {
        printf("Invalid input. \n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (memberID < 1 || memberID > store -> memberCount)
    {
        printf("[ERROR] Member ID %d not found. \n", memberID);
        return;
    }

    Member *m = &store -> members[memberID - 1];
    if (!m -> isActive)
    {
        printf("[ERROR] Member %s (ID %d) is inactive/cancelled. Cannot borrow. \n", m -> name, m ->memberID);
        return;
    }

    // 2. Identify & Validate the Item
    printf("Enter Media Item ID to borrow: ");
    if (scanf("%d", &itemID) != 1)
    {
        printf("Invalid input. \n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (itemID < 1 || itemID > store -> inventoryCount)
    {
        printf("[ERROR] Media Item ID %d does not exist. \n", itemID);
        return;
    }
    
    MediaItem *item = &store -> inventory[itemID - 1];
    if (item -> status != ITEM_AVAILABLE)
    {
        printf("[ERROR] Item '%s' (ID %d) is not available for borrowing. Current Status: %s\n", 
               item -> title, item -> itemID, getStatusName(item -> status));
        return;
    }

    // 3. Enforce the strict 1-item-per-category rule
    if (item -> category == MEDIA_VIDEO)
    {
        if (m -> borrowedVideoID != 0)
        {
            printf("\n[BORROW LIMIT REACHED] Member %s (ID %d) already has a borrowed Video Item (ID: %d). \n", 
                   m -> name, m -> memberID, m -> borrowedVideoID);
            printf("Members can hold at most 1 Video at a time. Please return it first.\n");
            return;
        }
    }
    else if (item -> category == MEDIA_MUSIC_CD)
    {
        if (m -> borrowedMusicID != 0)
        {
            printf("\n[BORROW LIMIT REACHED] Member %s (ID %d) already has a borrowed Music CD (ID: %d). \n", 
                   m -> name, m -> memberID, m -> borrowedMusicID);
            printf("Members can hold at most 1 Music CD at a time. Please return it first.\n");
            return;
        }
    }

    // 4. Lock in current effective daily rent (halved if > 1 year old)
    item -> lockedDailyRent = getEffectiveDailyRate(item, today);
    item -> status = ITEM_LOANED;
    item -> loanedToMemberID = m -> memberID;
    item -> loanIssueDate = today;
    item -> everIssued = 1;
    item -> lastActivityDate = today;

    // Link item to member
    if (item -> category == MEDIA_VIDEO)
        m -> borrowedVideoID = item -> itemID;
    else
        m -> borrowedMusicID = item -> itemID;

    // 5. Print Rental Receipt to console
    printf("\n==================================================\n");
    printf("                 RENTAL RECEIPT                   \n");
    printf("==================================================\n");
    printf("Receipt Date    : %02d/%02d/%04d\n", today.day, today.month, today.year);
    printf("Member ID       : %d\n", m->memberID);
    printf("Member Name     : %s\n", m->name);
    printf("Item ID         : %d\n", item->itemID);
    printf("Title           : %s\n", item->title);
    printf("Category        : %s (%s)\n", getCategoryName(item->category), getFormatName(item->format));
    printf("Daily Rental    : Rs. %.2f / day\n", item->lockedDailyRent);
    printf("Deposit Held    : Rs. %.2f\n", m->depositAmount);
    printf("==================================================\n");

    // Also saving receipt to a text file
    char receiptFileName[64];
    sprintf(receiptFileName, "receipt_item%d_member%d.txt", item -> itemID, m -> memberID);
    FILE *rfp = fopen(receiptFileName, "w");
    if (rfp != NULL)
    {
        fprintf(rfp, "==================================================\n");
        fprintf(rfp, "                 RENTAL RECEIPT                   \n");
        fprintf(rfp, "==================================================\n");
        fprintf(rfp, "Receipt Date    : %02d/%02d/%04d\n", today.day, today.month, today.year);
        fprintf(rfp, "Member ID       : %d\n", m->memberID);
        fprintf(rfp, "Member Name     : %s\n", m->name);
        fprintf(rfp, "Item ID         : %d\n", item->itemID);
        fprintf(rfp, "Title           : %s\n", item->title);
        fprintf(rfp, "Category        : %s (%s)\n", getCategoryName(item->category), getFormatName(item->format));
        fprintf(rfp, "Daily Rental    : Rs. %.2f / day\n", item->lockedDailyRent);
        fprintf(rfp, "Deposit Held    : Rs. %.2f\n", m->depositAmount);
        fprintf(rfp, "==================================================\n");
        fclose(rfp);
        printf(">>> Receipt saved to file '%s'.\n", receiptFileName);
    }
}

// Handles returning an item or reporting it lost/damaged
void returnOrReportItem(StoreState *store, Date today)
{
    int itemID;

    printf("\n--- Return / Report Lost or Damaged Itme ---\n");
    printf("Enter Media Item ID: ");
    if (scanf("%d", &itemID) != 1)
    {
        printf("Invalid input. \n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (itemID < 1 || itemID > store -> inventoryCount)
    {
        printf("[ERROR] Media Item ID %d does not exist. \n", itemID);
        return;
    }

    MediaItem *item = &store -> inventory[itemID - 1];
    if (item -> status != ITEM_LOANED)
    {
        printf("[ERROR] Item '%s' (ID %d) is not currently on loan. Current Status: %s\n", 
               item -> title, item -> itemID, getStatusName(item -> status));
               return;
    }

    Member *m = &store -> members[item -> loanedToMemberID - 1];

    // 1. Calculate rental duration in days 
    int days = daysBetweenDates(item -> loanIssueDate, today);
    if (days < 1)
        days = 1; // Minimum 1 day charge
    
    double rentalDue = days * item -> lockedDailyRent;

    printf("\n>>> Loan Record Found: \n");
    printf(" - Borrower    : %s (Member ID: %d)\n", m->name, m->memberID);
    printf(" - Item        : '%s' (Item ID: %d)\n", item->title, item->itemID);
    printf(" - Issue Date  : %02d/%02d/%04d\n", item->loanIssueDate.day, item->loanIssueDate.month, item->loanIssueDate.year);
    printf(" - Return Date : %02d/%02d/%04d (%d day%s)\n", today.day, today.month, today.year, days, days == 1 ? "" : "s");
    printf(" - Locked Rate : Rs. %.2f / day\n", item->lockedDailyRent);
    printf(" - Rental Due  : Rs. %.2f\n", rentalDue);

    // 2. Determine condition of the item
    int conditionChoice = 0;
    while (conditionChoice != 1 && conditionChoice != 2)
    {
        printf("\nSelect Return Condition: \n");
        printf("1. Item returned in good condition\n");
        printf("2. Item was LOST or DAMAGED\n");
        printf("Choice (1 or 2): ");
        if (scanf("%d", &conditionChoice) != 1 || (conditionChoice != 1 && conditionChoice != 2))
        {
            printf("Invalid selection. Please enter 1 or 2. \n");
            clearInputBuffer();
            conditionChoice = 0;
            continue; 
        }
        clearInputBuffer();
    }

    if (conditionChoice == 1)
    {
        // --- SAFE RETURN ---
        store -> totalRentalIncome += rentalDue;

        // Reset item to available
        item -> status = ITEM_AVAILABLE;
        item -> loanedToMemberID = 0;
        item -> lastActivityDate = today;
        item -> lockedDailyRent = 0.0;

        // Unlink item from member's active borrow slot. 
        if (item->category == MEDIA_VIDEO) {
            m->borrowedVideoID = 0;
        } else {
            m->borrowedMusicID = 0;
        }
        printf("\n==================================================\n");
        printf("               RETURN SETTLEMENT                  \n");
        printf("==================================================\n");
        printf("Member Name     : %s\n", m->name);
        printf("Item Title      : %s\n", item->title);
        printf("Days Borrowed   : %d\n", days);
        printf("Amount Paid     : Rs. %.2f\n", rentalDue);
        printf("Status          : Item returned to shelf.\n");
        printf("==================================================\n");
    } 
    else 
    {
        // --- LOST OR DAMAGED ---
        // Rule: Full price of the item is charged to them, and item is removed
        double replacementCost = item->purchasePrice;
        double totalDue = rentalDue + replacementCost;
        store->totalRentalIncome += rentalDue;
        store->totalPenaltyIncome += replacementCost;
        // Mark item as removed from inventory
        item->status = ITEM_REMOVED_LOST_DAMAGED;
        item->loanedToMemberID = 0;
        item->lastActivityDate = today;
        // Unlink item from member
        if (item->category == MEDIA_VIDEO) {
            m->borrowedVideoID = 0;
        } else {
            m->borrowedMusicID = 0;
        }
        printf("\n==================================================\n");
        printf("           LOSS / DAMAGE PENALTY INVOICE          \n");
        printf("==================================================\n");
        printf("Member Name     : %s\n", m->name);
        printf("Item Title      : %s (Item ID: %d)\n", item->title, item->itemID);
        printf("Rental Charge   : Rs. %.2f (%d day%s)\n", rentalDue, days, days == 1 ? "" : "s");
        printf("Replacement Fee : Rs. %.2f (100%% of purchase price)\n", replacementCost);
        printf("--------------------------------------------------\n");
        printf("TOTAL COLLECTED : Rs. %.2f\n", totalDue);
        printf("Status          : Item REMOVED from inventory.\n");
        printf("==================================================\n");
    }
}

/* ========================================
   6. CLEARANCE & PROFIT/LOSS ACCOUNTING
   ======================================== */

// Scans for items unissued for >365 days and sells them to active members at 10% of cost
void runUnissuedClearance(StoreState *store, Date today)
{
    printf("\n========================================================================================\n");
    printf("                       1-YEAR UNISSUED CLEARANCE SCAN (> 365 DAYS)                     \n");
    printf("========================================================================================\n");

    int eligibleCount = 0;
    int eligibleIDs[MAX_INVENTORY];

    // Find all items that have sat unissued for  > 365 days
    for (int i = 0; i < store -> inventoryCount; i++)
    {
        MediaItem *item = &store -> inventory[i];
        if (item -> status == ITEM_AVAILABLE)
        {
            int daysIdle = daysBetweenDates(item -> lastActivityDate, today);
            if (daysIdle > 365)
            {
                eligibleIDs[eligibleCount++] = item -> itemID;
                printf("[%d] ID %-4d | '%-22.22s' | Cost: Rs.%-7.2f | Idle: %d days | Clearance Price (10%%): Rs.%.2f\n",
                       eligibleCount,
                       item->itemID,
                       item->title,
                       item->purchasePrice,
                       daysIdle,
                       item->purchasePrice * 0.10);
            }
        }
    }

    if (eligibleCount == 0)
    {
        printf(">>> No items in inventory have been unissued for more than a year. \n");
        return;
    }

    printf("----------------------------------------------------------------------------------------\n");
    printf("Found %d eligible item(s) for clearance sale.\n", eligibleCount);

    int targetItemID;
    printf("Enter Media Item ID to sell at 10%% (or 0  to cancel): ");
    if (scanf("%d", &targetItemID) != 1)
    {
        clearInputBuffer();
        printf("Clearance operation cancelled. \n");
        return;
    }
    clearInputBuffer();

    // Verify selection is indeed eligible
    int isEligible = 0;
    for (int i = 0; i < eligibleCount; i++)
    {
        if (eligibleIDs[i] == targetItemID)
        {
            isEligible = 1;
            break;
        }
    }
    if (!isEligible)
    {
        printf("[ERROR] Item ID %d is not eligible for 10%% clearance sale. \n", targetItemID);
        return;
    }

    // Buyer must be an active registered member
    int buyerMemberID;
    printf("Enter purchasing Member ID: ");
    if (scanf("%d", &buyerMemberID) != 1)
    {
        printf("Invalid input. \n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (buyerMemberID < 1 || buyerMemberID > store -> memberCount)
    {
        printf("[ERROR] Member ID %d does not exist. \n", buyerMemberID);
        return;
    }

    Member *buyer = &store -> members[buyerMemberID - 1];
    if (!buyer -> isActive)
    {
        printf("[ERROR] Member %s (ID %d) is inactive. Only active members can buy clearance stock. \n", buyer -> name, buyer -> memberID);
        return;
    }

    MediaItem *targetItem = &store -> inventory[targetItemID - 1];
    double salePrice = targetItem -> purchasePrice * 0.10;

    // Record liquidation revenue and remove item from inventory
    store -> totalLiquidationSales += salePrice;
    targetItem -> status = ITEM_REMOVED_SOLD_LIQUIDATED;
    targetItem -> lastActivityDate = today;

    printf("\n==================================================\n");
    printf("             CLEARANCE SALE RECEIPT               \n");
    printf("==================================================\n");
    printf("Sale Date       : %02d/%02d/%04d\n", today.day, today.month, today.year);
    printf("Buyer Name      : %s (Member ID: %d)\n", buyer->name, buyer->memberID);
    printf("Item Sold       : '%s' (Item ID: %d)\n", targetItem->title, targetItem->itemID);
    printf("Original Cost   : Rs. %.2f\n", targetItem->purchasePrice);
    printf("Discount        : 90%% OFF\n");
    printf("Final Price Paid: Rs. %.2f (10%% of purchase price)\n", salePrice);
    printf("Status          : Item SOLD and REMOVED from inventory.\n");
    printf("==================================================\n");
}

// Computes and prints the full Profit & Loss Account of the store
void displayProfitLoss(const StoreState *store) {
    printf("\n========================================================\n");
    printf("           VIDEO RENTAL STORE - PROFIT & LOSS           \n");
    printf("========================================================\n");
    
    // 1. Inflows (Revenue)
    printf(" REVENUE / CASH INFLOWS:\n");
    printf("  + Rental Income Collected          : Rs. %10.2f\n", store->totalRentalIncome);
    printf("  + Lost / Damaged Recoveries (100%%) : Rs. %10.2f\n", store->totalPenaltyIncome);
    printf("  + Liquidation Clearance Sales (10%%): Rs. %10.2f\n", store->totalLiquidationSales);
    printf(" -------------------------------------------------------\n");
    double totalInflow = store->totalRentalIncome +
                         store->totalPenaltyIncome +
                         store->totalLiquidationSales;
    printf("  TOTAL REVENUE EARNED               : Rs. %10.2f\n\n", totalInflow);
    
    // 2. Outflows (Expenses)
    printf(" EXPENSES / CASH OUTFLOWS:\n");
    printf("  - Total Media Procurement Cost     : Rs. %10.2f\n", store->totalProcurementCost);
    printf(" -------------------------------------------------------\n");
    double totalOutflow = store->totalProcurementCost;
    printf("  TOTAL CAPITAL EXPENDITURE          : Rs. %10.2f\n\n", totalOutflow);
    
    // 3. Net Financial Position
    printf("========================================================\n");
    double netBalance = totalInflow - totalOutflow;
    if (netBalance >= 0.0) 
    {
        printf("  NET POSITION : NET PROFIT of Rs. %.2f\n", netBalance);
    } 
    else 
    {
        printf("  NET POSITION : NET LOSS of Rs. %.2f\n", -netBalance);
    }
    printf("========================================================\n");
    // 4. Balance Sheet Transparency Note: Customer Deposits
    int activeMembers = 0;
    for (int i = 0; i < store->memberCount; i++) {
        if (store->members[i].isActive) {
            activeMembers++;
        }
    }
    double totalDepositsHeld = activeMembers * MEMBERSHIP_DEPOSIT;
    printf("\n [LIABILITY NOTE]\n");
    printf("  Active Customer Deposits Held      : Rs. %.2f (%d active members)\n",
           totalDepositsHeld, activeMembers);
    printf("  (Refundable deposits are held as liabilities and\n");
    printf("   are strictly excluded from store profit/loss calculations).\n");
    printf("========================================================\n");
} 