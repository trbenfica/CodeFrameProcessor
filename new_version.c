#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

// Tipo para armazenar diferentes tipos de valores em co_consts
typedef enum {
    TYPE_CONST_NODE,
    TYPE_STRING,
    TYPE_NUMBER,
    TYPE_NULL
} ConstType;

// Estrutura para armazenar um item de co_consts
typedef struct ConstItem {
    ConstType type;
    union {
        struct ConstNode *node; // Para objetos recursivos
        char *string_val;       // Para strings
        double number_val;      // Para números
    };
} ConstItem;

// Estrutura recursiva para armazenar o JSON
typedef struct ConstNode {
    char *co_code;
    char **co_names;
    size_t co_names_len;
    char **co_varnames;
    size_t co_varnames_len;
    char **co_freevars;
    size_t co_freevars_len;
    char **co_cellvars;
    size_t co_cellvars_len;
    ConstItem **co_consts; // Recursão, strings, números, ou null
    size_t co_consts_len;
} ConstNode;

// Função auxiliar para extrair arrays de strings
char **parse_string_array(cJSON *json_array, size_t *length) {
    if (!cJSON_IsArray(json_array)) {
        *length = 0;
        return NULL;
    }

    size_t count = cJSON_GetArraySize(json_array);
    char **result = malloc(sizeof(char *) * count);

    for (size_t i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(json_array, i);
        if (cJSON_IsString(item)) {
            result[i] = strdup(item->valuestring); // Duplicando a string
        } else {
            result[i] = NULL;
        }
    }

    *length = count;
    return result;
}

// Função para fazer o parsing recursivo
ConstNode* parse_json(cJSON *json_item);

// Função para processar o array co_consts
ConstItem **parse_co_consts(cJSON *co_consts, size_t *length) {
    if (!cJSON_IsArray(co_consts)) {
        *length = 0;
        return NULL;
    }

    size_t count = cJSON_GetArraySize(co_consts);
    ConstItem **result = malloc(sizeof(ConstItem *) * count);

    for (size_t i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(co_consts, i);
        result[i] = malloc(sizeof(ConstItem));

        if (cJSON_IsObject(item)) {
            // Se for um objeto, processamos recursivamente
            result[i]->type = TYPE_CONST_NODE;
            result[i]->node = parse_json(item);
        } else if (cJSON_IsString(item)) {
            // Se for uma string
            result[i]->type = TYPE_STRING;
            result[i]->string_val = strdup(item->valuestring);
        } else if (cJSON_IsNumber(item)) {
            // Se for um número
            result[i]->type = TYPE_NUMBER;
            result[i]->number_val = item->valuedouble;
        } else if (cJSON_IsNull(item)) {
            // Se for null
            result[i]->type = TYPE_NULL;
        } else {
            // Caso não reconhecido, trate como null
            result[i]->type = TYPE_NULL;
        }
    }

    *length = count;
    return result;
}

// Função para fazer o parsing recursivo do JSON
ConstNode* parse_json(cJSON *json_item) {
    ConstNode *node = malloc(sizeof(ConstNode));
    memset(node, 0, sizeof(ConstNode));

    // Parsing do campo "co_code"
    cJSON *co_code = cJSON_GetObjectItem(json_item, "co_code");
    if (co_code && cJSON_IsString(co_code)) {
        node->co_code = strdup(co_code->valuestring); // Copiando a string
    }

    // Parsing de arrays de strings
    node->co_names = parse_string_array(cJSON_GetObjectItem(json_item, "co_names"), &node->co_names_len);
    node->co_varnames = parse_string_array(cJSON_GetObjectItem(json_item, "co_varnames"), &node->co_varnames_len);
    node->co_freevars = parse_string_array(cJSON_GetObjectItem(json_item, "co_freevars"), &node->co_freevars_len);
    node->co_cellvars = parse_string_array(cJSON_GetObjectItem(json_item, "co_cellvars"), &node->co_cellvars_len);

    // Parsing do array `co_consts`, que pode conter valores recursivos, strings, números ou null
    cJSON *co_consts = cJSON_GetObjectItem(json_item, "co_consts");
    if (co_consts && cJSON_IsArray(co_consts)) {
        node->co_consts = parse_co_consts(co_consts, &node->co_consts_len);
    }

    return node;
}

// Função para carregar o JSON de um arquivo
cJSON* load_json_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = malloc(file_size + 1);
    fread(file_content, 1, file_size, file);
    fclose(file);

    cJSON *json = cJSON_Parse(file_content);
    free(file_content);

    if (!json) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return NULL;
    }

    return json;
}

// Função principal para testar o parsing
int main() {
    const char *filename = "data.json"; // Nome do arquivo JSON
    cJSON *json = load_json_from_file(filename);
    if (!json) {
        return EXIT_FAILURE;
    }

    // Fazer o parsing do JSON
    ConstNode *root = parse_json(json);

    // Acessar e exibir dados (exemplo de acesso ao primeiro nome)
    if (root->co_names_len > 0) {
        printf("Primeiro co_name: %s\n", root->co_names[0]);
    }

    // Exemplo de acesso ao co_consts
    if (root->co_consts_len > 0) {
        for (size_t i = 0; i < root->co_consts_len; i++) {
            if (root->co_consts[i]->type == TYPE_STRING) {
                printf("Const %zu é uma string: %s\n", i, root->co_consts[i]->string_val);
            } else if (root->co_consts[i]->type == TYPE_NUMBER) {
                printf("Const %zu é um número: %f\n", i, root->co_consts[i]->number_val);
            } else if (root->co_consts[i]->type == TYPE_NULL) {
                printf("Const %zu é null\n", i);
            } else if (root->co_consts[i]->type == TYPE_CONST_NODE) {
                printf("Const %zu é um objeto recursivo\n", i);
            }
        }
    }

    // Processar e navegar pelos dados aqui...

    // Liberar memória alocada
    cJSON_Delete(json);

    return EXIT_SUCCESS;
}
