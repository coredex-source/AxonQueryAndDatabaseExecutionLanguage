#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

static ConfigNode* createNode(const char* key, const char* value) {
    ConfigNode *newNode = (ConfigNode *)malloc(sizeof(ConfigNode));
    if (newNode == NULL) {
        perror("Error allocating memory");
        exit(1);
    }
    strcpy(newNode->key, key);
    strcpy(newNode->value, value);
    newNode->next = NULL;
    return newNode;
}

static void addConfig(ConfigNode **head, const char* key, const char* value) {
    ConfigNode *newNode = createNode(key, value);
    if (*head == NULL) {
        *head = newNode;
    } else {
        ConfigNode *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}

void loadConfig(const char *filename, ConfigNode **configList) {
    FILE *file = fopen(filename, "r");
    char key[50], value[50];
    char line[100];

    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        sscanf(line, "%s = %s", key, value);
        addConfig(configList, key, value);
    }

    fclose(file);
}

const char* checkConfig(const ConfigNode *head, const char *key) {
    const ConfigNode *current = head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return (strcmp(current->value, "True") == 0) ? "True" : "False";
        }
        current = current->next;
    }
    return "Key not found";
}

void freeConfigList(ConfigNode *head) {
    ConfigNode *current = head;
    while (current != NULL) {
        ConfigNode *temp = current;
        current = current->next;
        free(temp);
    }
}

void ChangeConfigOption(const char * State) {
    FILE *file;
    const char *filename = "config.txt";

    file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    if(strcmp(State, "True") == 0){
        fprintf(file, "AutomaticallyLoadDatabases = True\n");
    } else {
        fprintf(file, "AutomaticallyLoadDatabases = False\n");
    }
    fclose(file);

}