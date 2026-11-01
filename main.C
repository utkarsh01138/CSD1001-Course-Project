#include <stdio.h>
#include "vrsms.h"

int main() {
    StoreState store = {0};
    const char *dataFile = "vrsms_data.dat";

    // Attempting to load previous state from disk (if any exists)
    if (loadStoreState(&store, dataFile)) 
    {
        printf(">>> Loaded existing store data from '%s'.\n", dataFile);
    }
    else 
    {
        printf(">>> No existing data found. Starting with a fresh store.\n");
    }

    // Default simulation date
    Date today = {01, 01, 2026};

    int choice = -1;
    int running = 1;


    printf("\n===============================\n");
    printf("\tVideo Rental Store Management System\n");
    printf("===============================\n");
    
    while (running)
    {
        printf("\n--- Current Date: %02d/%02d/%04d ---\n", today.day, today.month, today.year);
        printf("1. Register New Member\n");
        printf("2. Cancel Membership\n");
        printf("3. Procure New Media Item\n");
        printf("4. Issue Media Item (Borrow)\n");
        printf("5. Return/Report Lost or Damaged Item\n");
        printf("6. Run 1-year Unissued Clearance\n");
        printf("7. Display Inventory\n");
        printf("8. Display Members\n");
        printf("9. View Profit & Loss Account\n");
        printf("10. Fast-Forward / Set Today's Date\n");
        printf("0. Save & Exit\n");

        printf("--------------------------------\n");
        printf("Enter your choice (0-10): ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number between 0 and 10.\n");
            clearInputBuffer();
            continue;
        }
        clearInputBuffer(); 

        switch (choice)
        {
            case 1:
                registerMember(&store);
                break;
            case 2:
                cancelMembership(&store);
                break;
            case 3: 
                procureNewItem(&store, today);
                break;
            case 4: 
                issueItem(&store, today);
                break;
            case 5:
                returnOrReportItem(&store, today);
                break;
            case 6:
                runUnissuedClearance(&store, today);
                break;
            case 7:
                displayInventory(&store, today);
                break;
            case 8:
                displayMembers(&store);
                break;
            case 9:
                displayProfitLoss(&store);
                break;
            case 10:
                printf("Enter new date (DD MM YYYY): ");
                scanf("%d %d %d", &today.day, &today.month, &today.year);
                clearInputBuffer();
                printf("Date updated to %02d/%02d/%04d.\n", today.day, today.month, today.year);
                break;
            case 0:
                printf("\nSaving store data...\n");
                if (saveStoreState(&store, dataFile))
                {
                    printf("Store state successfully saved to '%s'. \n", dataFile);
                }
                else
                {
                    printf("Error: Could not save store state!\n");
                }
                printf("Exiting VRSMS. Goodbye!\n");
                running = 0;
                break;
            default:
                printf("Invalid choice (%d). Please choose an option between 0 and 10.\n", choice);
                break;
        }

    }
    return 0;
}