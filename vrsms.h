#ifndef VRSMS_H
#define VRSMS_H

#define MAX_MEMBERS 200
#define MAX_INVENTORY 500
#define MEMBERSHIP_DEPOSIT 1000

typedef struct {
    int day;
    int month;
    int year;
} Date;

typedef enum {
    MEDIA_VIDEO = 1,
    MEDIA_MUSIC_CD = 2
} MediaCategory;

typedef enum {
    FORMAT_VHS = 1,
    FORMAT_MP4 = 2,
    FORMAT_AUDIO_CD = 3
} MediaFormat;

typedef enum {
    ITEM_AVAILABLE = 1,
    ITEM_LOANED = 2,
    ITEM_REMOVED_LOST_DAMAGED = 3,
    ITEM_REMOVED_SOLD_LIQUIDATED = 4
} ItemStatus;

typedef struct {
    int itemID;
    char title[100];
    MediaCategory category;
    MediaFormat format;
    Date procurementDate;
    double purchasePrice;
    double baseDailyRate;

    ItemStatus status;
    int everIssued;
    Date lastActivityDate;

    int loanedToMemberID;
    Date loanIssueDate;
    double lockedDailyRent;
} MediaItem;

typedef struct {
    int memberID;
    char name[60];
    char address[120];
    char phone[20];
    int isActive;
    double depositAmount;
    double outstandingDues;

    int borrowedVideoID;
    int borrowedMusicID;
} Member;

typedef struct {
    Member members[MAX_MEMBERS];
    int memberCount;

    MediaItem inventory[MAX_INVENTORY];
    int inventoryCount;

    double totalProcurementCost;
    double totalRentalIncome;
    double totalPenaltyIncome;
    double totalLiquidationSales;
} StoreState;

/* =================== 
Function Prototypes
=================== */

// 1. Utility & Helper Functions
void clearInputBuffer();
int daysBetweenDates(Date d1, Date d2);
double getEffectiveDailyRate(const MediaItem *item, Date today);

// 2. Binary File persistence (Save & Load)
int saveStoreState(const StoreState *store, const char *fileName);
int loadStoreState(StoreState *store, const char *fileName);

// 3. Member Functions
void registerMember(StoreState *store);
void cancelMembership(StoreState *store);
void displayMembers(const StoreState *store);

// 4. Inventory Operations
void procureNewItem(StoreState *store, Date today);
void displayInventory(const StoreState *store, Date today);

// 5. Rental & Return Operations
void issueItem(StoreState *store, Date today);
void returnOrReportItem(StoreState *store, Date today);

// 6. Liquidation & Accounting
void runUnissuedClearance(StoreState *store, Date today);
void displayProfitLoss(const StoreState *store);

#endif /* VRSMS_H */