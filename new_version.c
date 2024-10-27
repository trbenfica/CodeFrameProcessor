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

// Definir o enum para identificar os campos
typedef enum {
    CO_NAMES,
    CO_VARNAMES,
    CO_FREEVARS,
    CO_CELLVARS,
    CO_CONSTS,
    CO_CODE
} CodeObjectField;

// Estrutura para armazenar um item de co_consts
typedef struct ConstItem {
    ConstType type;
    union {
        struct ConstNode *node; // Para objetos recursivos
        char *string_val;       // Para strings
        double number_val;      // Para números
    };
} ConstItem;

// Estrutura recursiva para armazenar o JSON (Frame Context)
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

// Definir a estrutura ConstNode (simplificada)
// typedef struct ConstNode {
//     char *co_code;
//     char **co_names;
//     size_t co_names_len;
//     char **co_varnames;
//     size_t co_varnames_len;
//     char **co_freevars;
//     size_t co_freevars_len;
//     char **co_cellvars;
//     size_t co_cellvars_len;
//     struct ConstNode **co_consts; // Pode conter outros ConstNode ou outros tipos
//     size_t co_consts_len;
// } ConstNode;

// Estrutura para armazenar frames (pilha de contextos)
typedef struct Frame {
    ConstNode *current_context;  // Contexto atual (CONST_NODE)
    struct Frame *previous_frame; // Referência ao frame anterior (pilha)
} Frame;

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

// Função para mudar de contexto para um novo frame baseado em um ConstNode
Frame* change_frame(Frame *current_frame, ConstNode *new_context) {
    Frame *new_frame = malloc(sizeof(Frame));
    new_frame->current_context = new_context;
    new_frame->previous_frame = current_frame;
    return new_frame;
}

// Função para restaurar o frame anterior (pop)
Frame* restore_previous_frame(Frame *current_frame) {
    if (current_frame == NULL || current_frame->previous_frame == NULL) {
        printf("Nenhum frame anterior para restaurar!\n");
        return current_frame;
    }

    Frame *previous_frame = current_frame->previous_frame;
    free(current_frame); // Liberar o frame atual
    return previous_frame;
}

void *get_value(ConstNode *context, CodeObjectField field, size_t index) {
    if (!context) {
        return NULL; // Verificar se o contexto é válido
    }

    switch (field) {
        case CO_NAMES:
            if (index < context->co_names_len) {
                return context->co_names[index];
            }
            break;
        case CO_VARNAMES:
            if (index < context->co_varnames_len) {
                return context->co_varnames[index];
            }
            break;
        case CO_FREEVARS:
            if (index < context->co_freevars_len) {
                return context->co_freevars[index];
            }
            break;
        case CO_CELLVARS:
            if (index < context->co_cellvars_len) {
                return context->co_cellvars[index];
            }
            break;
        case CO_CONSTS:
            if (index < context->co_consts_len) {
                if (context->co_consts[index]) {
                    return context->co_consts[index]; // Pode ser ConstNode ou outro tipo
                }
            }
            break;
        case CO_CODE:
            return context->co_code;
        default:
            return NULL; // Se o campo não for reconhecido
    }

    return NULL; // Retorna NULL se o índice for inválido ou o campo não existir
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
    const char *filename = "output.json"; // Nome do arquivo JSON
    cJSON *json = load_json_from_file(filename);
    if (!json) {
        return EXIT_FAILURE;
    }

    // Fazer o parsing do JSON
    ConstNode *root = parse_json(json);

    // Iniciar o frame atual com o contexto raiz
    Frame *current_frame = malloc(sizeof(Frame));
    current_frame->current_context = root;
    current_frame->previous_frame = NULL;

    // Variável auxiliar para o contexto atual
    ConstNode *current_context = current_frame->current_context;

    // Exemplo de acesso ao primeiro co_name usando a variável auxiliar
    // if (current_context->co_names_len > 0) {
    //     printf("co_code: %s\n", current_context->co_code);

    //     printf("co_names: %s\n", current_context->co_names[0]);
    //     printf("co_names: %s\n", current_context->co_names[1]);
    //     printf("co_names: %s\n", current_context->co_names[2]);
    // }

    // Exemplo de mudar para um novo frame (constante dentro de co_consts)
    // if (current_context->co_consts_len > 0) {
    //     for (size_t i = 0; i < current_context->co_consts_len; i++) {
    //         if (current_context->co_consts[i]->type == TYPE_CONST_NODE) {
    //             printf("Mudando para o frame de ConstNode no índice %zu\n", i);
    //             current_frame = change_frame(current_frame, current_context->co_consts[i]->node);
    //             current_context = current_frame->current_context; // Atualizar o contexto atual
    //             break; // Mudança de contexto para o primeiro ConstNode encontrado
    //         }
    //     }
    // }

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

    char* bytecode = current_context->co_code;

    for (int i = 0; i < strlen(bytecode); i += 4) {
        int instruction = convert_two_chars_to_hex(bytecode[i], bytecode[i + 1]);
        int arg = convert_two_chars_to_hex(bytecode[i + 2], bytecode[i + 3]);
        void *result = NULL;

        switch(instruction) {
            case LOAD_CONST:
                char *result = (char *)get_value(current_context, CO_NAMES, 0);
                printf("\nLOAD CONST: %s", result);
                break;
            case LOAD_FAST:
                result = get_value(current_context, CO_NAMES, arg);
                break;
            case STORE_FAST:
                result = get_value(current_context, CO_NAMES, arg);
                break;
            case LOAD_NAME:
                result = (char *)get_value(&root, CO_NAMES, 0);

                break;
            case STORE_NAME:
                // result = set_value(current_context, CO_NAMES, arg, "new_value");
                break;
            case LOAD_ATTR:
                result = get_value(current_context, CO_NAMES, arg);
                break;
            case STORE_ATTR:
                // result = set_value(current_context, CO_NAMES, arg, "new_value");
                break;
            case LOAD_GLOBAL:
                result = get_value(current_context, CO_NAMES, arg);
                break;
            case STORE_GLOBAL:
                // result = set_value(current_context, CO_NAMES, 1, "new_value");
                break;
            case CALL_FUNCTION:
                current_frame = restore_previous_frame(current_frame);
                current_context = current_frame->current_context;
                result = (char *)get_value(&root, CO_NAMES, 0);
                // result = get_code(current_context);
                break;
            case MAKE_FUNCTION:
                printf("not implemented");
                break;
        }

        printf("Instrução: %d", instruction);
    }

    
    

    // Liberar memória e finalizar
    cJSON_Delete(json);
    free(root);
    return EXIT_SUCCESS;
}

