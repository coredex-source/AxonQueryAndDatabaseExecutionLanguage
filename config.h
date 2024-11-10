#ifndef CONFIG_H
#define CONFIG_H

typedef struct ConfigNode {
    char key[50];
    char value[50];
    struct ConfigNode *next;
} ConfigNode;

void loadConfig(const char *filename, ConfigNode **configList);
const char* checkConfig(const ConfigNode *head, const char *key);
void freeConfigList(ConfigNode *head);
void ChangeConfigOption(const char * State);

#endif
