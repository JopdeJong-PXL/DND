#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "cJSON.h"

// Linked list struct
struct item_list {
    struct Item *item_dnd;
    struct item_list *next_item;
};

struct Money {
    int copper;
    int silver;
    int electrum;
    int gold;
    int platinum ;
};

// dnd object struct
struct Item {
    char *name;
    double weight;  // Changed to double
    struct item_cost {
        int cost;
        char *cost_unit;
    } cost;
    char *description;
};

void path_maker(char *path, const char *file_name);
void push_to_list(struct item_list **head, struct Item *object_to_push);
void fill_item_from_json(struct Item *item, const char *filename);
void extract_item(const cJSON *json, struct Item *item);
void traverse_list(struct item_list **head, double max_weight, double *total_weight, struct Money *money);
void pop_and_write_to_file(struct item_list **head, struct item_list *item_to_pop, double *total_weight);
void clear_camp_file();
void parse_money(const char *money_str, struct Money *money);

int check_extension(const char *file_name);
int check_file_exist(const char *file_path);
char *read_file(const char *filename);

cJSON *parse_json(const char *json_data);

int main(int argc, char* argv[]) { //Het geheel maakt gebruik van het ingeladen bestand en speelbaar deel te maken die objecten bijhoudt
    clear_camp_file();

    struct item_list *head = NULL;
    int counter_list = 0;
    double max_weight = 0; // standaard waarde max_weight
    double total_weight = 0; // initialize total weight being carried

    struct Money money = {0, 0, 0}; // Initialize money values to 0

    char path_str[256] = "C:\\Users\\jopde\\PXL\\Programmeren\\DND\\json_files\\";
    char *path_ptr = path_str;

    for (int i = 1; i < argc; i++) { //Er wordt correct gebruikgemaakt van de arguments to main (input en output files via flags)
        if (strcmp(argv[i], "-w") == 0) {
            if (i + 1 < argc) {
                max_weight = atof(argv[i + 1]);
                i++;
            } else {
                printf("Error: No value specified for -w flag.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-m") == 0) {
            // Read the next three arguments as coin values
            if (i + 3 < argc) {
                for (int j = 0; j < 3; j++) {
                    if (strstr(argv[i + 1 + j], "cp")) {
                        money.copper = atoi(argv[i + 1 + j]);

                    } else if (strstr(argv[i + 1 + j], "sp")) {
                        money.silver = atoi(argv[i + 1 + j]);

                    } else if (strstr(argv[i + 1 + j], "ep")) {
                        money.electrum  = atoi(argv[i + 1 + j]);

                    } else if (strstr(argv[i + 1 + j], "gp")) {
                        money.gold   = atoi(argv[i + 1 + j]);

                    } else if (strstr(argv[i + 1 + j], "pp")) {
                        money.platinum   = atoi(argv[i + 1 + j]);

                    } else {
                        
                        printf("Invalid coin type: %s\n", argv[i + 1 + j]);
                    }
                }
                i += 3; // ga naar volgende flag
            } else {
                printf("Error: Insufficient values specified for -m flag.\n");
                return 1;
            }
        } else if (check_extension(argv[i]) == 0) {
            path_maker(path_ptr, argv[i]);

            if (check_file_exist(path_ptr) == 0) {
                struct Item *new_item = (struct Item *)malloc(sizeof(struct Item)); //Geheugen wordt juist en efficiënt gealloceerd

                fill_item_from_json(new_item, path_ptr);

                if (counter_list == 0) {
                    head = (struct item_list *)malloc(sizeof(struct item_list));
                    head->item_dnd = new_item;
                    head->next_item = head;
                    counter_list++;
                } else {
                    push_to_list(&head, new_item);
                }
                total_weight += new_item->weight;
            }
            strcpy(path_ptr, "C:\\Users\\jopde\\PXL\\Programmeren\\DND\\json_files\\");
        }
    }

    traverse_list(&head, max_weight, &total_weight, &money);

    if (head != NULL) {
        struct item_list *current = head;
        do {
            struct item_list *temp = current;
            current = current->next_item;
            free(temp->item_dnd->name);
            free(temp->item_dnd->cost.cost_unit);
            free(temp->item_dnd->description);
            free(temp->item_dnd);
            free(temp);
        } while (current != head);
    }

    return 0;
}



// Function to concatenate file name to path
void path_maker(char *path, const char *file_name) {
    strcat(path, file_name);
}

// Function to check if file has .json extension
int check_extension(const char *file_name) {
    const char *extension = strrchr(file_name, '.');
    return (extension != NULL && strcmp(extension, ".json") == 0) ? 0 : 1;
}

// Function to check if file exists
int check_file_exist(const char *file_path) {
    struct stat buffer;
    return (stat(file_path, &buffer) == 0) ? 0 : 1;
}

// Function to push new item to linked list
void push_to_list(struct item_list **head, struct Item *object_to_push) { //Functies zijn voorzien om push, pop and cycle te doen op de circulaire linked list
    struct item_list *new_item = (struct item_list *)malloc(sizeof(struct item_list));
    new_item->item_dnd = object_to_push;
    new_item->next_item = *head;

    if (*head == NULL) {
        *head = new_item;
        new_item->next_item = new_item;  // Circular link
    } else {
        struct item_list *current = *head;
        while (current->next_item != *head) {
            current = current->next_item;
        }
        current->next_item = new_item;
    }
}

// Function to read the entire JSON file into a string
char *read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = (char *)malloc(length + 1);
    if (!data) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);
    return data;
}

// Function to parse JSON data from a string
cJSON *parse_json(const char *json_data) {
    cJSON *json = cJSON_Parse(json_data);
    if (!json) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
    }
    return json;
}

// Function to extract values from the JSON object and fill the Item struct
void extract_item(const cJSON *json, struct Item *item) { //Parse de json data van het bestand naar de voorziene gegevensstructuur


    const cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
    const cJSON *weight = cJSON_GetObjectItemCaseSensitive(json, "weight");
    const cJSON *cost = cJSON_GetObjectItemCaseSensitive(json, "cost");
    const cJSON *cost_unit = cJSON_GetObjectItemCaseSensitive(cost, "unit");
    const cJSON *cost_quantity = cJSON_GetObjectItemCaseSensitive(cost, "quantity");
    const cJSON *description = cJSON_GetObjectItemCaseSensitive(json, "description");
    const cJSON *desc_array = cJSON_GetObjectItemCaseSensitive(json, "desc");

    // Check and assign name
    if (cJSON_IsString(name) && (name->valuestring != NULL)) {
        item->name = strdup(name->valuestring);
    } else {
        item->name = strdup("N/A");
    }

    // Check and assign weight
    if (cJSON_IsNumber(weight)) {
        item->weight = cJSON_GetObjectItemCaseSensitive(json, "weight")->valuedouble;  // Changed to valuedouble
    } else {
        item->weight = 0.0;  // Default value
    }

    // Check and assign cost
    if (cJSON_IsNumber(cost_quantity)) {
        item->cost.cost = cost_quantity->valueint;
    } else {
        item->cost.cost = 0;  // Default value
    }

    // Check and assign cost unit
    if (cJSON_IsString(cost_unit) && (cost_unit->valuestring != NULL)) {
        item->cost.cost_unit = strdup(cost_unit->valuestring);
    } else {
        item->cost.cost_unit = strdup("N/A");
    }

    item->description = strdup("");
    // Check and assign description
    if (cJSON_IsArray(desc_array)) {
        // Handle desc array
        size_t total_length = 0;
        const cJSON *desc_item;
        cJSON_ArrayForEach(desc_item, desc_array) {
            if (cJSON_IsString(desc_item) && desc_item->valuestring != NULL) {
                total_length += strlen(desc_item->valuestring) + 1;  // +1 for the newline character
            }
        }
        item->description = (char *)realloc(item->description, strlen(item->description) + total_length + 1);
        cJSON_ArrayForEach(desc_item, desc_array) {
            if (cJSON_IsString(desc_item) && desc_item->valuestring != NULL) {
                strcat(item->description, "\n");
                strcat(item->description, desc_item->valuestring);
            }
        }
    } else {
        item->description = strdup("N/A");
    }
}

// Function to fill the Item struct with data from a JSON file
void fill_item_from_json(struct Item *item, const char *filename) { // Parse de json data van het bestand naar de voorziene gegevensstructuur
    char *json_data = read_file(filename);
    if (!json_data) {
        return;
    }

    cJSON *json = parse_json(json_data);
    if (!json) {
        free(json_data);
        return;
    }

    extract_item(json, item);

    cJSON_Delete(json);
    free(json_data);
}

void traverse_list(struct item_list **head, double max_weight, double *total_weight, struct Money *money) { //cycle
    if (*head == NULL) {
        printf("The list is empty.\n");
        return;
    }

    struct item_list *current = *head;
    char input[10];
    int item_count = 0;

    struct item_list *temp = *head;
    do {
        item_count++;
        temp = temp->next_item;
    } while (temp != *head);

    do {
        printf("\n\nItem Name: %s\n\n", current->item_dnd->name);
        printf("Maximum Weight: %.2f\n", max_weight);
        printf("Total Weight Being Carried: %.2f\n", *total_weight);
        printf("Items in Inventory: %d\n", item_count);
        printf("Money: %d gc, %d sc, %d bc\n", money->copper, money->silver, money->electrum, money->gold, money->platinum);

        // Check if weight exceeds maximum allowed weight
        if (max_weight != -1 && *total_weight > max_weight) {
            printf("To many items, move item to the camp\n\n");
        }

        printf("\n- Press 'n' to see the next item\n");
        printf("- Press 'i' to show more information\n");
        printf("- Press 'c' to add item to the camp\n");
        printf("- Press any other key to exit\n");

        scanf("%s", input);
        getchar();
        printf("\n");

        if (input[0] == 'i') {
            printf("Object Informatie: \n");
            printf("Name: %s\n", current->item_dnd->name);
            printf("Cost: %d %s\n", current->item_dnd->cost.cost, current->item_dnd->cost.cost_unit);
            printf("Weight: %.2f\n", current->item_dnd->weight);
            printf("Description: %s\n\n", current->item_dnd->description);
            printf("Total Weight Being Carried: %.2f\n", *total_weight);
            printf("Maximum Weight: %.2f\n", max_weight);
            printf("Items in Inventory: %d\n", item_count);
            printf("Money: %d gc, %d sc, %d bc\n", money->copper, money->silver, money->electrum, money->gold, money->platinum);
            printf("------------------------------------------\n");

            printf("\n- Press 'n' to see the next item\n");
            printf("- Press 'i' to show more information\n");
            printf("- Press 'c' to add item to the camp\n");
            printf("- Press any other key to exit\n");
            
            scanf("%s", input);
            getchar();
            printf("\n");
        }

        if (input[0] == 'n') {
            current = current->next_item;
        } else if (input[0] == 'c') {
            struct item_list *next_item = current->next_item;
            pop_and_write_to_file(head, current, total_weight);
            item_count--;
            if (*head == NULL) {
                printf("The list is empty.\n");
                return;
            }
            current = (current == *head) ? *head : next_item;
        } else {
            break;
        }
    } while (current != *head || input[0] == 'n' || input[0] == 'c');
}

void pop_and_write_to_file(struct item_list **head, struct item_list *item_to_pop, double *total_weight) { //pop
    if (*head == NULL || item_to_pop == NULL) {
        printf("Invalid operation.\n");
        return;
    }

    struct item_list *current = *head;
    struct item_list *previous = NULL;

    // Find the item_to_pop in the list
    while (current != item_to_pop && current->next_item != *head) {
        previous = current;
        current = current->next_item;
    }

    if (current != item_to_pop) {
        printf("Item not found.\n");
        return;
    }

    // Remove item_to_pop from the list
    if (previous == NULL) {
        // If item_to_pop is the head of the list
        if (current->next_item == current) {
            // If there is only one item in the list
            *head = NULL;
        } else {
            *head = current->next_item; // Update head to the next item
            struct item_list *temp = current->next_item;
            while (temp->next_item != current) {
                temp = temp->next_item;
            }
            temp->next_item = *head; // Make the last item point to the new head
        }
    } else {
        // If item_to_pop is not the head of the list
        previous->next_item = current->next_item; // Update the previous node's next pointer
    }

    // Update total weight being carried
    *total_weight -= item_to_pop->item_dnd->weight;

    // Write the item's contents to camp.txt
    FILE *file = fopen("camp.txt", "a"); //Bewaren van gemaakte keuzes in bestand m.b.v. file handling
    if (file == NULL) {
        printf("Error opening file.\n"); 
        free(item_to_pop->item_dnd->name); //// Geheugen leegmaken tegen memory leaks
        free(item_to_pop->item_dnd->cost.cost_unit);
        free(item_to_pop->item_dnd->description);
        free(item_to_pop->item_dnd);
        free(item_to_pop);
        return;
    }

    fprintf(file, "Name: %s\n", item_to_pop->item_dnd->name);
    fprintf(file, "Weight: %.2f\n", item_to_pop->item_dnd->weight);
    fprintf(file, "Cost: %d %s\n", item_to_pop->item_dnd->cost.cost, item_to_pop->item_dnd->cost.cost_unit);
    fprintf(file, "Description: %s\n\n", item_to_pop->item_dnd->description);

    fclose(file);

    // Free the memory used by the popped item
    free(item_to_pop->item_dnd->name);
    free(item_to_pop->item_dnd->cost.cost_unit);
    free(item_to_pop->item_dnd->description);
    free(item_to_pop->item_dnd);
    free(item_to_pop);
}

void clear_camp_file() {
    FILE *file = fopen("camp.txt", "w");
    if (file == NULL) {
        printf("Error opening file.\n");
        return;
    }
    fclose(file);
}

void parse_money(const char *money_str, struct Money *money) {
    money->copper = 0;
    money->silver = 0;
    money->electrum = 0;
    money->gold = 0;
    money->platinum = 0;



    const char delimiters[] = " ,";
    char *token;

    printf("Input string: %s\n", money_str); // Print the input string for debugging

    // Tokenize the input string
    char *copy = strdup(money_str); // Make a copy since strtok modifies the original string
    token = strtok(copy, delimiters);

    // Parse each token
    while (token != NULL) {
        printf("Token: %s\n", token); // Print each token for debugging
        int value;
        char type[3];

        // Parse value and type directly from token
        if (sscanf(token, "%d%2s", &value, type) >= 1) { ////call-by-reference
            printf("Value: %d, Type: %s\n", value, type); // Print parsed value and type for debugging
            // Update corresponding coin type
            if (strcmp(type, "cp") == 0) {
                money->copper += value;
            } else if (strcmp(type, "sp") == 0) {
                money->silver += value;
            } else if (strcmp(type, "ep") == 0) {
                money->electrum += value;
            } else if (strcmp(type, "gp") == 0) {
                money->gold += value;
            } else if (strcmp(type, "pp") == 0) {
                money->platinum += value;
            }
        } else {
            printf("Invalid token format: %s\n", token);
        }

        // Move to the next token
        token = strtok(NULL, delimiters);
    }

    free(copy); 
}
