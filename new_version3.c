#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

// Estrutura para armazenar um "code" e seus vínculos
typedef struct Code {
    char* co_code;
    char** co_names;
    size_t co_names_count;
    char** co_varnames;
    size_t co_varnames_count;
    struct Code** co_consts; // Vetor de ponteiros para sub-codes
    size_t co_consts_count;
    struct Code* parent;  // Ponteiro para o code pai
    struct Code* next;    // Ponteiro para o próximo code (apenas se necessário)
} Code;

char* read_json_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* content = (char*)malloc(file_size + 1);
    if (content == NULL) {
        perror("Erro ao alocar memória");
        fclose(file);
        return NULL;
    }

    fread(content, 1, file_size, file);
    content[file_size] = '\0';

    fclose(file);
    return content;
}

// Função para criar um novo objeto "Code"
Code* create_code() {
    Code* new_code = (Code*)malloc(sizeof(Code));
    new_code->co_code = NULL;
    new_code->co_names = NULL;
    new_code->co_names_count = 0;
    new_code->co_varnames = NULL;
    new_code->co_varnames_count = 0;
    new_code->co_consts = NULL;
    new_code->co_consts_count = 0;
    new_code->parent = NULL;
    new_code->next = NULL;
    return new_code;
}

// Função para liberar a memória de um objeto "Code"
void free_code(Code* code) {
    if (code == NULL) return;
    
    free(code->co_code);
    
    for (size_t i = 0; i < code->co_names_count; ++i) {
        free(code->co_names[i]);
    }
    free(code->co_names);
    
    for (size_t i = 0; i < code->co_varnames_count; ++i) {
        free(code->co_varnames[i]);
    }
    free(code->co_varnames);
    
    for (size_t i = 0; i < code->co_consts_count; ++i) {
        free_code(code->co_consts[i]);
    }
    free(code->co_consts);
    
    free(code);
}

// Função para analisar o JSON e criar a estrutura de "Code"
Code* parse_code(cJSON* json, Code* parent) {
    if (json == NULL) return NULL;

    Code* code = create_code();
    code->parent = parent;

    // Lê o "co_code"
    cJSON* co_code = cJSON_GetObjectItemCaseSensitive(json, "co_code");
    if (cJSON_IsString(co_code) && (co_code->valuestring != NULL)) {
        code->co_code = strdup(co_code->valuestring);
    }

    // Lê "co_names"
    cJSON* co_names = cJSON_GetObjectItemCaseSensitive(json, "co_names");
    if (cJSON_IsArray(co_names)) {
        code->co_names_count = cJSON_GetArraySize(co_names);
        code->co_names = (char**)malloc(code->co_names_count * sizeof(char*));
        for (size_t i = 0; i < code->co_names_count; ++i) {
            cJSON* name = cJSON_GetArrayItem(co_names, i);
            if (cJSON_IsString(name)) {
                code->co_names[i] = strdup(name->valuestring);
            }
        }
    }

    // Lê "co_varnames"
    cJSON* co_varnames = cJSON_GetObjectItemCaseSensitive(json, "co_varnames");
    if (cJSON_IsArray(co_varnames)) {
        code->co_varnames_count = cJSON_GetArraySize(co_varnames);
        code->co_varnames = (char**)malloc(code->co_varnames_count * sizeof(char*));
        for (size_t i = 0; i < code->co_varnames_count; ++i) {
            cJSON* varname = cJSON_GetArrayItem(co_varnames, i);
            if (cJSON_IsString(varname)) {
                code->co_varnames[i] = strdup(varname->valuestring);
            }
        }
    }

    // Lê "co_consts"
    cJSON* co_consts = cJSON_GetObjectItemCaseSensitive(json, "co_consts");
    if (cJSON_IsArray(co_consts)) {
        code->co_consts_count = cJSON_GetArraySize(co_consts);
        code->co_consts = (Code**)malloc(code->co_consts_count * sizeof(Code*));
        for (size_t i = 0; i < code->co_consts_count; ++i) {
            cJSON* const_item = cJSON_GetArrayItem(co_consts, i);
            if (cJSON_IsObject(const_item)) {
                // Recursivamente parseia o sub-code
                code->co_consts[i] = parse_code(const_item, code);
            } else {
                code->co_consts[i] = NULL; // Pode ser NULL se não for um objeto "code"
            }
        }
    }

    return code;
}

// Função para carregar e analisar o JSON do arquivo
Code* load_code_from_file(const char* filename) {
    char* json_string = read_json_file(filename);
    if (json_string == NULL) {
        return NULL;
    }

    cJSON* json = cJSON_Parse(json_string);
    free(json_string);

    if (json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Erro ao analisar o JSON: %s\n", error_ptr);
        }
        return NULL;
    }

    // Parseia o JSON em um objeto "Code"
    Code* code = parse_code(json, NULL);

    // Libera a memória do JSON cJSON
    cJSON_Delete(json);

    return code;
}

int main() {
    const char* filename = "output.json";
    Code* root_code = load_code_from_file(filename);

    if (root_code == NULL) {
        printf("Falha ao carregar o JSON.\n");
        return 1;
    }

    // A partir daqui, você pode acessar e percorrer o "code" e seus sub-objetos.
    // Por exemplo: printf("Código principal: %s\n", root_code->co_code);

    // Libera a memória dos objetos "Code" após o uso
    free_code(root_code);

    return 0;
}
