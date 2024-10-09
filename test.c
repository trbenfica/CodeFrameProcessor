#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <stdint.h>

typedef enum {
    CO_NAMES,
    CO_VARNAMES,
    CO_FREEVARS,
    CO_CELLVARS,
    CO_CONSTS,
    CO_CODE
} CodeObjectField;

// Definindo a estrutura para um par chave-valor
typedef struct KeyValuePair {
    char *key;
    char *value;
    struct KeyValuePair *next;
} KeyValuePair;

// Definindo a estrutura para o objeto de código
typedef struct CodeObjectStruct {
    char *co_code;
    KeyValuePair *co_names;
    KeyValuePair *co_varnames;
    KeyValuePair *co_freevars;
    KeyValuePair *co_cellvars;
    struct ConstItem *co_consts;
} CodeObjectStruct;

// Definindo a estrutura para itens da constante
typedef struct ConstItem {
    union {
        int integer;
        CodeObjectStruct *codeObject;
    };
    int isObject; // Flag para indicar se é um objeto
    struct ConstItem *next;
} ConstItem;

// Função para criar um novo par chave-valor
KeyValuePair *createKeyValuePair(const char *key, const char *value) {
    KeyValuePair *newPair = malloc(sizeof(KeyValuePair));
    if (newPair) {
        newPair->key = strdup(key);
        newPair->value = value ? strdup(value) : NULL;
        newPair->next = NULL;
    }
    return newPair;
}

// Função para adicionar um par chave-valor à lista
void addKeyValuePair(KeyValuePair **head, KeyValuePair *newPair) {
    if (*head == NULL) {
        *head = newPair;
    } else {
        KeyValuePair *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newPair;
    }
}

// Função para criar um novo item de constante
ConstItem *createConstItem(int isObject) {
    ConstItem *newItem = malloc(sizeof(ConstItem));
    if (newItem) {
        newItem->isObject = isObject;
        newItem->next = NULL;
        if (isObject) {
            newItem->codeObject = malloc(sizeof(CodeObjectStruct));
            if (newItem->codeObject) {
                newItem->codeObject->co_code = NULL;
                newItem->codeObject->co_names = NULL;
                newItem->codeObject->co_varnames = NULL;
                newItem->codeObject->co_freevars = NULL;
                newItem->codeObject->co_cellvars = NULL;
                newItem->codeObject->co_consts = NULL;
            }
        } else {
            newItem->integer = 0;
        }
    }
    return newItem;
}

// Função para adicionar um item de constante à lista
void addConstItem(ConstItem **head, ConstItem *newItem) {
    if (*head == NULL) {
        *head = newItem;
    } else {
        ConstItem *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newItem;
    }
}

void processCodeObject(cJSON *json, CodeObjectStruct *codeObject) {
    cJSON *co_code = cJSON_GetObjectItemCaseSensitive(json, "co_code");
    if (cJSON_IsString(co_code)) {
        codeObject->co_code = strdup(co_code->valuestring);
    }

    cJSON *co_names = cJSON_GetObjectItemCaseSensitive(json, "co_names");
    if (cJSON_IsArray(co_names)) {
        cJSON *name;
        cJSON_ArrayForEach(name, co_names) {
            KeyValuePair *newPair = createKeyValuePair(name->valuestring, NULL);
            if (newPair) {
                addKeyValuePair(&(codeObject->co_names), newPair);
            }
        }
    }

    cJSON *co_varnames = cJSON_GetObjectItemCaseSensitive(json, "co_varnames");
    if (cJSON_IsArray(co_varnames)) {
        cJSON *varname;
        cJSON_ArrayForEach(varname, co_varnames) {
            KeyValuePair *newPair = createKeyValuePair(varname->valuestring, NULL);
            if (newPair) {
                addKeyValuePair(&(codeObject->co_varnames), newPair);
            }
        }
    }

    cJSON *co_freevars = cJSON_GetObjectItemCaseSensitive(json, "co_freevars");
    if (cJSON_IsArray(co_freevars)) {
        cJSON *freevar;
        cJSON_ArrayForEach(freevar, co_freevars) {
            KeyValuePair *newPair = createKeyValuePair(freevar->valuestring, NULL);
            if (newPair) {
                addKeyValuePair(&(codeObject->co_freevars), newPair);
            }
        }
    }

    cJSON *co_cellvars = cJSON_GetObjectItemCaseSensitive(json, "co_cellvars");
    if (cJSON_IsArray(co_cellvars)) {
        cJSON *cellvar;
        cJSON_ArrayForEach(cellvar, co_cellvars) {
            KeyValuePair *newPair = createKeyValuePair(cellvar->valuestring, NULL);
            if (newPair) {
                addKeyValuePair(&(codeObject->co_cellvars), newPair);
            }
        }
    }

    cJSON *co_consts = cJSON_GetObjectItemCaseSensitive(json, "co_consts");
    if (cJSON_IsArray(co_consts)) {
        cJSON *co_const;
        cJSON_ArrayForEach(co_const, co_consts) {
            if (cJSON_IsObject(co_const)) {
                ConstItem *newItem = createConstItem(1);
                if (newItem) {
                    processCodeObject(co_const, newItem->codeObject);
                    addConstItem(&(codeObject->co_consts), newItem);
                }
            } else if (cJSON_IsNumber(co_const)) {
                ConstItem *newItem = createConstItem(0);
                if (newItem) {
                    newItem->integer = co_const->valueint;
                    addConstItem(&(codeObject->co_consts), newItem);
                }
            }
        }
    }
}


const char *getKeyValuePairValue(KeyValuePair *head, int index) {
    int currentIndex = 0;
    KeyValuePair *current = head;
    while (current != NULL) {
        if (currentIndex == index) {
            return current->key;  // Retornando o campo key como valor.
        }
        current = current->next;
        currentIndex++;
    }
    return NULL; // Retorna NULL se o índice estiver fora do alcance.
}


int getConstItemValue(ConstItem *head, int index, int *isObject) {
    int currentIndex = 0;
    ConstItem *current = head;
    while (current != NULL) {
        if (currentIndex == index) {
            *isObject = current->isObject;
            if (current->isObject) {
                return (int)(uintptr_t)current->codeObject; // Retornando o ponteiro como inteiro
            } else {
                return current->integer;
            }
        }
        current = current->next;
        currentIndex++;
    }
    *isObject = -1; // Indica que o índice está fora do alcance.
    return 0; // Valor padrão se o índice estiver fora do alcance.
}


const char *get_value(CodeObjectStruct *codeObject, CodeObjectField field, int index) {
    KeyValuePair *currentPair;
    ConstItem *currentItem;
    int currentIndex = 0;

    switch (field) {
        case CO_NAMES:
            currentPair = codeObject->co_names;
            while (currentPair != NULL) {
                if (currentIndex == index) {
                    return currentPair->key;
                }
                currentPair = currentPair->next;
                currentIndex++;
            }
            break;
        case CO_VARNAMES:
            currentPair = codeObject->co_varnames;
            while (currentPair != NULL) {
                if (currentIndex == index) {
                    return currentPair->key;
                }
                currentPair = currentPair->next;
                currentIndex++;
            }
            break;
        case CO_FREEVARS:
            currentPair = codeObject->co_freevars;
            while (currentPair != NULL) {
                if (currentIndex == index) {
                    return currentPair->key;
                }
                currentPair = currentPair->next;
                currentIndex++;
            }
            break;
        case CO_CELLVARS:
            currentPair = codeObject->co_cellvars;
            while (currentPair != NULL) {
                if (currentIndex == index) {
                    return currentPair->key;
                }
                currentPair = currentPair->next;
                currentIndex++;
            }
            break;
        case CO_CONSTS:
            currentItem = codeObject->co_consts;
            while (currentItem != NULL) {
                if (currentIndex == index) {
                    if (currentItem->isObject) {
                        return "CodeObject"; // Retornar um placeholder para o objeto de código
                    } else {
                        static char buffer[12];
                        snprintf(buffer, 12, "%d", currentItem->integer);
                        return buffer;
                    }
                }
                currentItem = currentItem->next;
                currentIndex++;
            }
            break;
        default:
            break;
    }

    printf('Erro! Índice fora do alcance ou campo inválido.\n');
    return NULL;
}

char* get_code(CodeObjectStruct *codeObject) {
    return codeObject->co_code;
}

int setKeyValuePairValue(KeyValuePair *head, int index, const char *newValue) {
    int currentIndex = 0;
    KeyValuePair *current = head;
    while (current != NULL) {
        if (currentIndex == index) {
            free(current->key); // Liberar a memória do valor antigo
            current->key = strdup(newValue); // Atualizar com o novo valor
            return 1; // Sucesso
        }
        current = current->next;
        currentIndex++;
    }
    return 0; // Índice fora do alcance
}

int set_value(CodeObjectStruct *codeObject, CodeObjectField field, int index, const char *newValue) {
    KeyValuePair *currentPair;
    ConstItem *currentItem;
    int currentIndex;

    switch (field) {
        case CO_NAMES:
            currentPair = codeObject->co_names;
            currentIndex = 0;
            while (currentPair != NULL) {
                if (currentIndex == index) {
                    free(currentPair->key);
                    currentPair->key = strdup(newValue);
                    return 1; // Sucesso
                }
                currentPair = currentPair->next;
                currentIndex++;
            }
            break;
        case CO_VARNAMES:
            currentPair = codeObject->co_varnames;
            currentIndex = 0;
            while (currentPair != NULL) {
                if (currentIndex == index) {
                    free(currentPair->key);
                    currentPair->key = strdup(newValue);
                    return 1; // Sucesso
                }
                currentPair = currentPair->next;
                currentIndex++;
            }
            break;
        case CO_FREEVARS:
            currentPair = codeObject->co_freevars;
            currentIndex = 0;
            while (currentPair != NULL) {
                if (currentIndex == index) {
                    free(currentPair->key);
                    currentPair->key = strdup(newValue);
                    return 1; // Sucesso
                }
                currentPair = currentPair->next;
                currentIndex++;
            }
            break;
        case CO_CELLVARS:
            currentPair = codeObject->co_cellvars;
            currentIndex = 0;
            while (currentPair != NULL) {
                if (currentIndex == index) {
                    free(currentPair->key);
                    currentPair->key = strdup(newValue);
                    return 1; // Sucesso
                }
                currentPair = currentPair->next;
                currentIndex++;
            }
            break;
        case CO_CONSTS:
            currentItem = codeObject->co_consts;
            currentIndex = 0;
            while (currentItem != NULL) {
                if (currentIndex == index) {
                    if (currentItem->isObject) {
                        // Para simplificar, vamos assumir que `newValue` aponta para um objeto CodeObjectStruct existente
                        currentItem->codeObject = (CodeObjectStruct *)(uintptr_t)atoi(newValue);
                    } else {
                        currentItem->integer = atoi(newValue);
                    }
                    return 1; // Sucesso
                }
                currentItem = currentItem->next;
                currentIndex++;
            }
            break;
        case CO_CODE:
            if (index == 0) {
                free(codeObject->co_code);
                codeObject->co_code = strdup(newValue);
                return 1; // Sucesso
            }
            break;
        default:
            break;
    }

    printf('Erro! Falha ao atualizar o valor: índice fora do alcance ou campo inválido.\n');
    return 0;
}


int setConstItemValue(ConstItem *head, int index, int isObject, const char *newValue) {
    int currentIndex = 0;
    ConstItem *current = head;
    while (current != NULL) {
        if (currentIndex == index) {
            if (isObject) {
                // Para simplificar, vamos assumir que `newValue` aponta para um objeto CodeObjectStruct existente
                current->codeObject = (CodeObjectStruct *)(uintptr_t)atoi(newValue);
            } else {
                current->integer = atoi(newValue);
            }
            return 1; // Sucesso
        }
        current = current->next;
        currentIndex++;
    }
    return 0; // Índice fora do alcance
}

CodeObjectStruct* call_frame(CodeObjectStruct* codeObject, int n) {
    if (!codeObject) {
        return NULL;
    }

    ConstItem *current = codeObject->co_consts;
    int currentIndex = 0;

    while (current != NULL) {
        if (currentIndex == n) {
            if (current->isObject) {
                return current->codeObject;
            } else {
                return NULL; // O item no índice n não é um objeto-code
            }
        }
        current = current->next;
        currentIndex++;
    }

    printf('Erro! Falha ao chamar o frame: índice fora do alcance.\n');
    return NULL;
}


char* initialize_file() {
    // Abrir o arquivo JSON para leitura
    FILE *file = fopen("output.json", "r");
    if (file == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo JSON.\n");
        return 1;
    }
    
    // Determinar o tamanho do arquivo
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Alocar memória para armazenar o conteúdo do arquivo
    char *jsonData = (char *)malloc(fileSize + 1);
    if (jsonData == NULL) {
        fprintf(stderr, "Erro ao alocar memória.\n");
        fclose(file);
        return 1;
    }
    
    // Ler o conteúdo do arquivo para a string jsonData
    fread(jsonData, 1, fileSize, file);
    jsonData[fileSize] = '\0'; // Adicionar terminador de string
    
    // Fechar o arquivo após a leitura
    fclose(file);

    return jsonData;

}

void initialize_frameObject(CodeObjectStruct* frame) {
    frame->co_code = NULL;
    frame->co_names = NULL;
    frame->co_varnames = NULL;
    frame->co_freevars = NULL;
    frame->co_cellvars = NULL;
    frame->co_consts = NULL;
}

CodeObjectStruct* process_JSON() {
    char* jsonData = initialize_file();

    cJSON *json = cJSON_Parse(jsonData);
    if (json == NULL) {
        printf("Erro ao analisar o JSON.\n");
        return 1;
    }

    // Criar a estrutura para armazenar os dados do objeto de código
    CodeObjectStruct *codeObject = malloc(sizeof(CodeObjectStruct));
    if (codeObject == NULL) {
        printf("Erro ao alocar memória.\n");
        cJSON_Delete(json);
        return 1;
    }

    // Inicializar a estrutura
    initialize_frameObject(codeObject);

    // Processar o objeto de código
    processCodeObject(json, codeObject);

    // Liberar a memória alocada para o JSON
    cJSON_Delete(json);
    return codeObject;
}

int convert_char_to_hex(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    else if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    return -1; // Retorna -1 em caso de erro
}

int convert_two_chars_to_hex(char high, char low) {
    int high_nibble = convert_char_to_hex(high);
    int low_nibble = convert_char_to_hex(low);

    if (high_nibble == -1 || low_nibble == -1) {
        printf("Erro: caracteres inválidos!\n");
        return -1; // Erro
    }

    return (high_nibble << 4) | low_nibble;
}


int main() {
    /*
        Este enum lista as instruções e OPCODES *padronizados* pelo interpretador CPython. Estas 
        são as instruções abrangidas pelo gerenciador.

        Lista completa de OPCODES disponível em: https://unpyc.sourceforge.net/Opcodes.html
    */
    typedef enum {
        LOAD_CONST = 0x64,
        LOAD_FAST = 0x7C,
        STORE_FAST = 0x7D,
        LOAD_NAME = 0x65,
        STORE_NAME = 0x5A,
        LOAD_ATTR = 0x69,
        STORE_ATTR = 0x5F,
        LOAD_GLOBAL = 0x74,
        STORE_GLOBAL = 0x61,
        CALL_FUNCTION = 0x83,
        MAKE_FUNCTION = 0x84
    } Opcode; 


    CodeObjectStruct *currentFrame = process_JSON();
    char* bytecode = get_code(currentFrame);

    for (int i = 0; i < strlen(bytecode); i += 4) {
        int instruction = convert_two_chars_to_hex(bytecode[i], bytecode[i + 1]);
        int arg = convert_two_chars_to_hex(bytecode[i + 2], bytecode[i + 3]);
        char *result = NULL;

        switch(instruction) {
            case LOAD_CONST:
                result = get_value(currentFrame, CO_CONSTS, arg);
                break;
            case LOAD_FAST:
                result = get_value(currentFrame, CO_NAMES, arg);
                break;
            case STORE_FAST:
                result = get_value(currentFrame, CO_NAMES, arg);
                break;
            case LOAD_NAME:
                result = get_value(currentFrame, CO_NAMES, arg);
                break;
            case STORE_NAME:
                result = set_value(currentFrame, CO_NAMES, arg, "new_value");
                break;
            case LOAD_ATTR:
                result = get_value(currentFrame, CO_NAMES, arg);
                break;
            case STORE_ATTR:
                result = set_value(currentFrame, CO_NAMES, arg, "new_value");
                break;
            case LOAD_GLOBAL:
                result = get_value(currentFrame, CO_NAMES, arg);
                break;
            case STORE_GLOBAL:
                result = set_value(currentFrame, CO_NAMES, 1, "new_value");
                break;
            case CALL_FUNCTION:
                currentFrame = call_frame(currentFrame, 0);
                result = get_code(currentFrame);
                break;
            case MAKE_FUNCTION:
                printf("not implemented");
                break;
        }

        printf("Instrução: %d", instruction);
    }
    

    // chamar frame
    // CodeObjectStruct *nestedCodeObject = call_frame(codeObject, 0);
    

    // chamar frame
    // CodeObjectStruct *nestedCodeObject = call_frame(codeObject, 0);

    return 0;
}
