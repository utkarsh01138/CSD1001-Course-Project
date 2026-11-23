#ifndef VRSMS_H
#define VRSMS_H

#define MAX_MEMBERS 200
#define MAX_INVENTORY 500
#define MEMBERSHIP_DEPOSIT 1000.0

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
    FORMAT_DVD = 3,
    FORMAT_VCD = 4,
    FORMAT_AUDIO_CD = 5
} MediaFormat;

typedef enum {
    ITEM_AVAILABLE = 1,
    ITEM_LOANED = 2,
    ITEM_REMOVED_LOST_DAMAGED = 3,
    ITEM_REMOVED_SOLD_LIQUIDATED = 4
} ItemStatus;

typedef struct {
    int itemId;
    char title[100];
    MediaCategory category;
    MediaFormat format;
    Date procurementDate;
    double purchasePrice;
    double baseDailyRate;

    ItemStatus status;
    int everIssued;
    Date lastActivityDate;

    int loanedToMemberId;
    Date loanIssueDate;
    double lockedDailyRent;
} MediaItem;

typedef struct {
    int memberId;
    char name[60];
    char address[120];
    char phone[20];
    int isActive;
    double depositAmount;
    double outstandingDues;

    int borrowedVideoId;
    int borrowedMusicId;
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

// --- Helper Functions ---
void clearInputBuffer();
void readString(char *buffer, int maxLen);
int isValidDate(Date d);
int daysBetweenDates(Date d1, Date d2);
double getEffectiveDailyRate(const MediaItem *item, Date todayDate);

// --- File Persistence ---
int saveStoreState(const StoreState *store, const char *fileName);
int loadStoreState(StoreState *store, const char *fileName);

// --- Member Operations ---
void registerMember(StoreState *store);
void cancelMembership(StoreState *store);
void payMemberDues(StoreState *store);
void displayMembers(const StoreState *store);

// --- Inventory Operations ---
void procureNewItem(StoreState *store, Date todayDate);
void displayInventory(const StoreState *store, Date todayDate);

// --- Rental & Return Operations ---
void issueItem(StoreState *store, Date todayDate);
void returnOrReportItem(StoreState *store, Date todayDate);

// --- Clearance & Accounts ---
void runUnissuedClearance(StoreState *store, Date todayDate);
void displayProfitLoss(const StoreState *store);

#endif /* VRSMS_H */
