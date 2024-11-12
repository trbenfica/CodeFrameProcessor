#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <variant>
#include <stack>
#include <sstream>
#include <typeinfo>
#include "include/json.hpp"
#include <bitset>
#include <iomanip>
		    
// Estruturas e tipos previamente definidos
class Code;  // Declaração antecipada
using VarType = std::variant<Code, int, bool, std::string, std::nullptr_t>;

// Função para converter um inteiro de 32 bits para binário e escrevê-lo no fluxo
void writeBinaryInt32(std::ostream& os, uint32_t value) {
    os.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void hexToBinaryStream(const std::string& hex, std::ostream& outStream) {
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteStr = hex.substr(i, 2);  // Pega dois caracteres (um byte)
        unsigned int byte;
        std::stringstream(byteStr) >> std::hex >> byte;  // Converte o par hex para inteiro

        // Escreve o byte como binário no fluxo
        outStream.put(static_cast<char>(byte));  // Coloca o byte no fluxo de saída
    }
}

std::vector<VarType>* globals = nullptr;

class PayloadType {
public:
    static const std::unordered_map<std::string, std::string> typeMap;
    static const std::unordered_map<std::string, std::string> codeMap;
};

// Mapeamento entre tipo e código binário
const std::unordered_map<std::string, std::string> PayloadType::typeMap = {
    {"Code", "100"},
    {"nullptr_t", "000"},
    {"int", "001"},
    {"float", "010"},
    {"string", "011"},
    {"bool", "101"},
    {"custom", "110"},
    {"unknown", "111"}
};

// Mapeamento invertido entre código binário e tipo
const std::unordered_map<std::string, std::string> PayloadType::codeMap = {
    {"100", "Code"},
    {"000", "nullptr_t"},
    {"001", "int"},
    {"010", "float"},
    {"011", "string"},
    {"101", "bool"},
    {"110", "custom"},
    {"111", "unknown"}
};

class Code {
public:
    std::string co_code;
    std::vector<VarType> co_names;
    std::vector<VarType> co_varnames;
    std::vector<VarType> co_freevars;
    std::vector<VarType> co_cellvars;
    std::vector<VarType> co_consts;

    // Construtor
    Code(const std::string& code = "") : co_code(code) {}

    // Métodos para definir os valores de cada campo
    void setCoCode(const std::string& code) { co_code = code; }
    void setCoNames(const std::vector<VarType>& names) { co_names = names; }
    void setCoVarnames(const std::vector<VarType>& varnames) { co_varnames = varnames; }
    void setCoFreevars(const std::vector<VarType>& freevars) { co_freevars = freevars; }
    void setCoCellvars(const std::vector<VarType>& cellvars) { co_cellvars = cellvars; }
    void setCoConsts(const std::vector<VarType>& consts) { co_consts = consts; }

    // Método para imprimir todos os campos
    void print() const {
        std::cout << "co_code: " << co_code << std::endl;

        auto printVector = [](const std::vector<VarType>& vec) {
            for (const auto& item : vec) {
                std::visit([](const auto& val) {
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (std::is_same_v<T, Code>) {
                        std::cout << "<code> ";
                    } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                        std::cout << "null ";
                    } else if constexpr (std::is_integral_v<T>) {
                        std::cout << val << " ";
                    } else {
                        std::cout << "\"" << val << "\"" << " ";
                    }
                }, item);
            }
            std::cout << std::endl;
        };

        std::cout << "globals: ";
        printVector(*globals);

        std::cout << "co_names: ";
        printVector(co_names);

        std::cout << "co_varnames: ";
        printVector(co_varnames);

        std::cout << "co_freevars: ";
        printVector(co_freevars);

        std::cout << "co_cellvars: ";
        printVector(co_cellvars);

        std::cout << "co_consts: ";
        printVector(co_consts);
    }

    // Método para acessar um objeto Code aninhado
    const Code& getCodeFromConsts(size_t index) const {
        if (index >= co_consts.size()) {
            throw std::out_of_range("Index out of range for co_consts");
        }

        // Verifica se o elemento no índice é do tipo Code
        if (std::holds_alternative<Code>(co_consts[index])) {
            return std::get<Code>(co_consts[index]); // Retorna a referência ao objeto Code
        } else {
            throw std::bad_variant_access(); // Lança uma exceção se não for do tipo Code
        }
    }
        
    std::string generatePayload() const {
        const char GS = 29;  // ASCII Group Separator
        const char US = 31;  // ASCII Unit Separator
        const char NULL_CHAR = 0;  // ASCII NULL

        // Função para converter uma string de 3 bits em um byte
        auto bitsToByte = [](const std::string& bits) -> char {
            return static_cast<char>(std::bitset<3>(bits).to_ulong());
        };

        std::ostringstream result;
        hexToBinaryStream(co_code, result);
        result << GS;

        // Função lambda para formatar um vetor com códigos de tipo
        auto formatVector = [&](const std::vector<VarType>& vec) -> std::string {
            std::ostringstream oss;
            writeBinaryInt32(oss, vec.size());
            for (const auto& item : vec) {
                oss << US;  // Inicia o item com um US
                
                std::visit([&oss, &bitsToByte, this](const auto& val) {
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (std::is_same_v<T, Code>) {
                        oss << bitsToByte(PayloadType::typeMap.at("Code"));  // Código para Code como byte binário
                        oss << NULL_CHAR;
                    } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                        oss << bitsToByte(PayloadType::typeMap.at("nullptr_t"));  // Código para nullptr como byte binário
                        oss << NULL_CHAR;
                    } else if constexpr (std::is_integral_v<T>) {
                        oss << bitsToByte(PayloadType::typeMap.at("int"));  // Código para int como byte binário
                        writeBinaryInt32(oss, val);
                    } else if constexpr (std::is_floating_point_v<T>) {
                        oss << bitsToByte(PayloadType::typeMap.at("float"));  // Código para float como byte binário
                        oss << val;
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        oss << bitsToByte(PayloadType::typeMap.at("string"));  // Código para string como byte binário
                        oss << val;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        oss << bitsToByte(PayloadType::typeMap.at("bool"));  // Código para bool como byte binário
                        oss << (val ? "1" : "0");
                    } else {
                        oss << bitsToByte(PayloadType::typeMap.at("unknown"));  // Código reservado como byte binário
                        oss << "undefined";  // Placeholder para tipos não especificados
                    }
                }, item);
            }
            return oss.str();
        };

        // Concatena cada campo formatado com GS entre eles
        result << formatVector(*globals) << GS;
        result << formatVector(co_names) << GS;
        result << formatVector(co_varnames) << GS;
        result << formatVector(co_freevars) << GS;
        result << formatVector(co_cellvars) << GS;

        // Formata o campo CONSTS, sem incluir o tamanho
        for (size_t i = 0; i < co_consts.size(); ++i) {
            if (i > 0) result << US;
            std::visit([&result, &bitsToByte, this](const auto& val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, Code>) {
                    result << bitsToByte(PayloadType::typeMap.at("Code"));  // NULL_CHAR para Code ou nullptr como byte binário
                    result << NULL_CHAR;
                } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                    result << bitsToByte(PayloadType::typeMap.at("nullptr_t"));  // NULL_CHAR para Code ou nullptr como byte binário
                    result << NULL_CHAR;
                } else if constexpr (std::is_integral_v<T>) {
                    result << bitsToByte(PayloadType::typeMap.at("int"));  // Código para int como byte binário
                    writeBinaryInt32(result, val);
                } else if constexpr (std::is_floating_point_v<T>) {
                    result << bitsToByte(PayloadType::typeMap.at("float"));  // Código para float como byte binário
                    result << val;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    result << bitsToByte(PayloadType::typeMap.at("string"));  // Código para string como byte binário
                    result << val;
                } else if constexpr (std::is_same_v<T, bool>) {
                    result << bitsToByte(PayloadType::typeMap.at("bool"));  // Código para bool como byte binário
                    result << (val ? "1" : "0");
                } else {
                    result << bitsToByte(PayloadType::typeMap.at("unkown"));  // Código reservado como byte binário
                    result << "undefined";  // Placeholder para tipos não especificados
                }
            }, co_consts[i]);
        }

        // Salva o payload em um arquivo binário
        std::string resultString = result.str();
        std::ofstream outFile("output.bin", std::ios::binary);
        if (outFile) {
            outFile.write(resultString.data(), resultString.size());  // Escreve os dados da string no arquivo binário
        } else {
            std::cerr << "Erro ao abrir o arquivo para escrita!" << std::endl;
        }
        
        return result.str();
    }

    std::string generateTestPayload() const {
        const char GS = 29;  // ASCII Group Separator
        const char US = 31;  // ASCII Unit Separator
        const char NULL_CHAR = 0;  // ASCII NULL

        // Função para converter uma string de 3 bits em um byte
        auto bitsToByte = [](const std::string& bits) -> char {
            return static_cast<char>(std::bitset<3>(bits).to_ulong());
        };

        std::ostringstream result;

        // Função lambda para formatar um vetor com códigos de tipo
        auto formatVector = [&](const std::vector<VarType>& vec) -> std::string {
            std::ostringstream oss;
            for (const auto& item : vec) {
                oss << US;  // Inicia o item com um US
                
                std::visit([&oss, &bitsToByte, this](const auto& val) {
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (std::is_same_v<T, Code>) {
                        oss << bitsToByte(PayloadType::typeMap.at("Code"));  // Código para Code como byte binário
                        oss << NULL_CHAR;
                    } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                        oss << bitsToByte(PayloadType::typeMap.at("nullptr_t"));  // Código para nullptr como byte binário
                        oss << NULL_CHAR;
                    } else if constexpr (std::is_integral_v<T>) {
                        oss << bitsToByte(PayloadType::typeMap.at("int"));  // Código para int como byte binário
                        writeBinaryInt32(oss, val);
                    } else if constexpr (std::is_floating_point_v<T>) {
                        oss << bitsToByte(PayloadType::typeMap.at("float"));  // Código para float como byte binário
                        oss << val;
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        oss << bitsToByte(PayloadType::typeMap.at("string"));  // Código para string como byte binário
                        oss << val;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        oss << bitsToByte(PayloadType::typeMap.at("bool"));  // Código para bool como byte binário
                        oss << (val ? "1" : "0");
                    } else {
                        oss << bitsToByte(PayloadType::typeMap.at("unknown"));  // Código reservado como byte binário
                        oss << "undefined";  // Placeholder para tipos não especificados
                    }
                }, item);
            }
            return oss.str();
        };

        // Concatena cada campo formatado com GS entre eles
        result << formatVector(*globals) << GS;
        result << formatVector(co_names) << GS;
        result << formatVector(co_varnames) << GS;
        result << formatVector(co_freevars) << GS;
        result << formatVector(co_cellvars) << GS;

        // Salva o payload em um arquivo binário
        std::string resultString = result.str();
        std::ofstream outFile("input.bin", std::ios::binary);
        if (outFile) {
            outFile.write(resultString.data(), resultString.size());
        } else {
            std::cerr << "Erro ao abrir o arquivo para escrita!" << std::endl;
        }
        
        return result.str();
    }

    void updateFromPayload(const std::string& payload) {
        const char GS = 29;
        const char US = 31;

        auto parseVector = [&](const std::string& segment) -> std::vector<VarType> {
            std::vector<VarType> result;
            std::istringstream stream(segment);
            std::string item;
            
            while (std::getline(stream, item, US)) {
                if (item.empty()) continue;

                std::string typeCode = item.substr(0, 1);  // Extrai apenas o primeiro byte como tipo
                std::string value = item.substr(1);         // Extrai o restante como dados
                
                // Converte typeCode para uma string de bits e busca no mapa
                std::string typeBits = std::bitset<3>(typeCode[0]).to_string(); // Converte o byte para uma string de 3 bits
                std::string typeName = PayloadType::codeMap.at(typeBits);
                
                if (typeName == "Code") {
                    result.emplace_back(Code()); // Insere um objeto Code vazio (ou implemente um código de deserialização)
                } else if (typeName == "nullptr_t") {
                    result.emplace_back(nullptr); // Insere um nullptr
                } else if (typeName == "int") {
                    result.emplace_back(static_cast<int>(static_cast<unsigned char>(value[0])));
                } else if (typeName == "string") {
                    result.emplace_back(value); // Adiciona diretamente como string
                } else if (typeName == "bool") {
                    result.emplace_back(value == "1"); // Converte "1" para true, "0" para false
                } else {
                    throw std::runtime_error("Tipo desconhecido no payload");
                }
            }
            
            return result;
        };

        // Divide o payload em segmentos usando GS
        std::vector<std::string> segments;
        std::istringstream stream(payload);
        std::string segment;
        
        while (std::getline(stream, segment, GS)) {
            segments.push_back(segment);
        }

        // Atualiza os campos com os segmentos decodificados
        if (segments.size() > 0) *globals = parseVector(segments[0]);
        if (segments.size() > 1) co_names = parseVector(segments[1]);
        if (segments.size() > 2) co_varnames = parseVector(segments[2]);
        if (segments.size() > 3) co_freevars = parseVector(segments[3]);
        if (segments.size() > 4) co_cellvars = parseVector(segments[4]);
    }
};

// Gera caso de teste: todos os valores estão abrangidos aqui
Code generateInputPayloadCases() {
    std::unordered_map<std::string, Code> inputPayloadCases;
    int stdSize = 4;

    Code codeObj;

    Code otherCode;
    // todos arrays > 0, com valores arbitrários 
    codeObj.setCoNames(std::vector<VarType>{10, 20, "one", otherCode, true});
    codeObj.setCoVarnames(std::vector<VarType>{30, 40, "two", otherCode, true});
    codeObj.setCoFreevars(std::vector<VarType>{50, 60, "three", otherCode, false});
    codeObj.setCoCellvars(std::vector<VarType>{70, 80, "four", otherCode, true});

    codeObj.generatePayload();

    return codeObj;
}

// Função para ler o JSON e criar o objeto Code
Code readCodeFromJson(const nlohmann::json& jsonData) {
    Code codeObj;

    codeObj.setCoCode(jsonData["co_code"].get<std::string>());

    size_t namesSize = jsonData["co_names"].size();
    size_t varnamesSize = jsonData["co_varnames"].size();
    size_t freevarsSize = jsonData["co_freevars"].size();
    size_t cellvarsSize = jsonData["co_cellvars"].size();

    codeObj.setCoNames(std::vector<VarType>(namesSize, nullptr));
    codeObj.setCoVarnames(std::vector<VarType>(varnamesSize, nullptr));
    codeObj.setCoFreevars(std::vector<VarType>(freevarsSize, nullptr));
    codeObj.setCoCellvars(std::vector<VarType>(cellvarsSize, nullptr));

    std::vector<VarType> consts;
    for (const auto& item : jsonData["co_consts"]) {
        if (item.is_string()) {
            consts.push_back(item.get<std::string>());
        } else if (item.is_number_integer()) {
            consts.push_back(item.get<int>());
        } else if (item.is_boolean()) {
            consts.push_back(item.get<bool>());
        } else if (item.is_null()) {
            consts.push_back(nullptr);
        } else if (item.is_object()) {
            consts.push_back(readCodeFromJson(item)); // Chamada recursiva
        }
    }
    codeObj.setCoConsts(consts);

    return codeObj;
}

Code readCodeFromJsonFile(const std::string& filename) {
    std::ifstream inputFile(filename);
    nlohmann::json jsonData;

    if (!inputFile.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    inputFile >> jsonData;  // Lê o JSON do arquivo
    return readCodeFromJson(jsonData); // Lê o primeiro Frame e retorna
}

// Pilha para gerenciar a navegação entre os objetos Code
class CodeNavigator {
private:
    std::stack<const Code*> navigationStack;

public:
    // Adiciona um novo objeto Code à pilha
    void push(const Code* code) {
        navigationStack.push(code);
    }

    // Retorna o objeto Code no topo da pilha
    const Code* pop() {
        if (navigationStack.empty()) {
            throw std::runtime_error("Navigation stack is empty.");
        }
        const Code* top = navigationStack.top();
        navigationStack.pop();
        return top;
    }

    // Retorna o objeto Code no topo da pilha sem removê-lo
    const Code* peek() const {
        if (navigationStack.empty()) {
            throw std::runtime_error("Navigation stack is empty.");
        }
        return navigationStack.top();
    }
};

std::string readPayloadFromFile(const std::string& filename) {
    std::ifstream inputFile(filename, std::ios::binary);
    if (!inputFile) {
        throw std::runtime_error("Erro ao abrir o arquivo para leitura.");
    }

    // Move o ponteiro de leitura para o final do arquivo para obter o tamanho
    inputFile.seekg(0, std::ios::end);
    std::streamsize fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    // Lê o conteúdo do arquivo para uma string
    std::string payload(fileSize, '\0');
    if (!inputFile.read(&payload[0], fileSize)) {
        throw std::runtime_error("Erro ao ler o conteúdo do arquivo.");
    }

    return payload;
}


/* Backlog
 *
 * [OK] globals
 * [OK] generatePayload
 * [OK] updateFromPayload
 * ajustar troca de frames
 * protocolo de comunicação (flags, espera, handshake, etc.)
 *
 * mostrar formato renato/bruno
 * 
 * (questões de projeto...)
 * preciso manter o bool? parece melhor só deixar zero ou um...
 * oferecer suporte a float?
 * instruções de OOP?
 * lançar erro se tamanho do array do payload for maior que do frame original?
 * 
 * (escrita...)
 * reestruturar o que já existe
 * escrever parte 2 (desenv., testes, conclusão)
 * escrever docs/apêndice para: serializer.py, handshake (com formato de payload) e CodeFrameProcessor
 * 
 * (limitações...)
 * não atende a funções lambda
 * não atende floats (?)
 * 
 * */

int main() {
    try {
        Code code = readCodeFromJsonFile("code.json");
        globals = &code.co_names;


        // **** starts here

        // generateInputPayloadCases();
        code.print();
        
        std::cout << std::endl << "____ depois: _____" << std::endl << std::endl;
        std::string payload = readPayloadFromFile("input_test_payload.bin");
        code.updateFromPayload(payload);
        code.print();

        // code.generatePayload();
            
        // CodeNavigator navigator;
        // navigator.push(&code);

        // const Code& newCode = code.getCodeFromConsts(2);
        // navigator.push(&newCode);
        // printf("\n");

        // const Code* currentCode = navigator.peek();
        // currentCode->print(); 
        printf("\n");

        // currentCode = navigator.pop();
        // currentCode = navigator.peek();
        // currentCode->print(); 

    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
    }

    return 0;
}

