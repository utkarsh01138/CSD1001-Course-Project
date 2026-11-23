#include <stdio.h>
#include "vrsms.h"

int main()
{
    StoreState store = {0};
    const char *dataFile = "vrsms_data.dat";

    // Attempting to load previous state from disk (if any exists)
    if (loadStoreState(&store, dataFile))
        printf(">>> Loaded existing store data from '%s'.\n", dataFile);
    else
        printf(">>> No existing data found. Starting with a fresh store.\n");

    // Default simulation date
    Date todayDate = {1, 1, 2026};

    int userChoice = -1;
    int isRunning = 1;

    printf("\n===============================\n");
    printf("\tVideo Rental Store Management System\n");
    printf("===============================\n");

    while (isRunning)
    {
        printf("\n--- Current Date: %02d/%02d/%04d ---\n", todayDate.day, todayDate.month, todayDate.year);
        printf("1. Register New Member\n");
        printf("2. Cancel Membership\n");
        printf("3. Pay Member Outstanding Dues\n");
        printf("4. Procure New Media Item\n");
        printf("5. Issue Media Item (Borrow)\n");
        printf("6. Return / Report Lost or Damaged Item\n");
        printf("7. Run 1-Year Unissued Clearance\n");
        printf("8. Display Inventory\n");
        printf("9. Display Members\n");
        printf("10. View Profit & Loss Account\n");
        printf("11. Fast-Forward / Set Today's Date\n");
        printf("0. Save & Exit\n");

        printf("--------------------------------\n");
        printf("Enter your choice (0-11): ");

        if (scanf("%d", &userChoice) != 1)
        {
            printf("Invalid input. Please enter a number between 0 and 11.\n");
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        switch (userChoice)
        {
            case 1:
                registerMember(&store);
                break;
            case 2:
                cancelMembership(&store);
                break;
            case 3:
                payMemberDues(&store);
                break;
            case 4:
                procureNewItem(&store, todayDate);
                break;
            case 5:
                issueItem(&store, todayDate);
                break;
            case 6:
                returnOrReportItem(&store, todayDate);
                break;
            case 7:
                runUnissuedClearance(&store, todayDate);
                break;
            case 8:
                displayInventory(&store, todayDate);
                break;
            case 9:
                displayMembers(&store);
                break;
            case 10:
                displayProfitLoss(&store);
                break;
            case 11:
            {
                Date newDate = {0};
                printf("Enter new date (DD MM YYYY): ");
                if (scanf("%d %d %d", &newDate.day, &newDate.month, &newDate.year) != 3 || !isValidDate(newDate))
                {
                    printf("[ERROR] Invalid calendar date entered.\n");
                    clearInputBuffer();
                    break;
                }
                clearInputBuffer();
                todayDate = newDate;
                printf("Date updated to %02d/%02d/%04d.\n", todayDate.day, todayDate.month, todayDate.year);
                break;
            }
            case 0:
                printf("\nSaving store data...\n");
                if (saveStoreState(&store, dataFile))
                    printf("Store state successfully saved to '%s'.\n", dataFile);
                else
                    printf("[ERROR] Could not save store state!\n");
                printf("Exiting VRSMS. Goodbye!\n");
                isRunning = 0;
                break;
            default:
                printf("Invalid choice (%d). Please choose an option between 0 and 11.\n", userChoice);
                break;
        }
    }

    return 0;
}
