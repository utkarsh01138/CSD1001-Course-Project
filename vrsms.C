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
    
}