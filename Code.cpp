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
#include "Code.hpp"

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

std::vector<VarType> Code::globals;

Code::Code(const std::string& code) : co_code(code) {}

void Code::setCoCode(const std::string& code) { co_code = code; }
void Code::setCoNames(const std::vector<VarType>& names) { co_names = names; }
void Code::setCoVarnames(const std::vector<VarType>& varnames) { co_varnames = varnames; }
void Code::setCoFreevars(const std::vector<VarType>& freevars) { co_freevars = freevars; }
void Code::setCoCellvars(const std::vector<VarType>& cellvars) { co_cellvars = cellvars; }
void Code::setCoConsts(const std::vector<VarType>& consts) { co_consts = consts; }

void Code::print() const {
    std::cout << std::endl << "co_code: " << co_code << std::endl;

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
                } else if constexpr (std::is_floating_point_v<T>) {
                    std::cout << val << " ";
                } else {
                    std::cout << "\"" << val << "\"" << " ";
                }
            }, item);
        }
        std::cout << std::endl;
    };

    std::cout << "globals: ";
    printVector(Code::globals);

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
Code& Code::getCodeFromVariable(size_t vector, size_t index) {
    if (index >= co_consts.size()) {
        throw std::out_of_range("Index out of range for co_consts");
    }

    std::vector<VarType>* targetVector = nullptr;

    switch (vector) {
        case 0:
            targetVector = &Code::globals;
            break;
        case 1:
            targetVector = &co_names;
            break;
        case 2:
            targetVector = &co_varnames;
            break;
        case 3:
            targetVector = &co_freevars;
            break;
        case 4:
            targetVector = &co_cellvars;
            break;
        default:
            throw std::invalid_argument("Invalid dstVector value.");
    }

    // Verifica se o índice está dentro dos limites do vetor
    if (index >= targetVector->size()) {
        throw std::out_of_range("Index out of range for targetVector");
    }

    // Verifica se o elemento no índice é do tipo int
    if (!std::holds_alternative<int>((*targetVector)[index])) {
        throw std::runtime_error("Code::getCodeFromVariable -> elemento no índice não é um int");
    }

    // Recupera o índice do elemento em co_consts
    int constsIndex = std::get<int>((*targetVector)[index]);

    // Verifica se o elemento em constsIndex é do tipo Code
    if (std::holds_alternative<Code>(co_consts[constsIndex])) {
        return std::get<Code>(co_consts[constsIndex]); // Retorna a referência ao objeto Code
    } else {
        throw std::runtime_error("Tentativa de acesso a Code filho, porém o index não é de um objeto Code");
    }
}

    
std::string Code::generatePayload() const {
    const char GS = 29;  // ASCII Group Separator
    const char US = 31;  // ASCII Unit Separator
    const char NULL_CHAR = 0;  // ASCII NULL

    // Função para converter uma string de 3 bits em um byte
    auto bitsToByte = [](const std::string& bits) -> char {
        return static_cast<char>(std::bitset<3>(bits).to_ulong());
    };

    std::ostringstream result;
    result << GS;
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
    result << formatVector(Code::globals) << GS;
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

    result << GS;
    
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

void Code::updateFromPayload(const std::string& payload) {
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
                result.emplace_back(nullptr); // Insere um objeto Code vazio (ou implemente um código de deserialização)
            } else if (typeName == "nullptr_t") {
                result.emplace_back(nullptr); // Insere um nullptr
            } else if (typeName == "int") {
                result.emplace_back(static_cast<int>(static_cast<unsigned char>(value[0])));
            } else if (typeName == "string") {
                result.emplace_back(value); // Adiciona diretamente como string
            } else if (typeName == "bool") {
                result.emplace_back(value == "1"); // Converte "1" para true, "0" para false
            } else if (typeName == "float") {
                result.emplace_back(static_cast<float>(static_cast<unsigned char>(value[0])));
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
    if (segments.size() > 0) Code::globals = parseVector(segments[0]);
    if (segments.size() > 1) co_names = parseVector(segments[1]);
    if (segments.size() > 2) co_varnames = parseVector(segments[2]);
    if (segments.size() > 3) co_freevars = parseVector(segments[3]);
    if (segments.size() > 4) co_cellvars = parseVector(segments[4]);
}


std::string Code::generateInputTestPayload() const {
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
    result << GS;
    result << formatVector(Code::globals) << GS;
    result << formatVector(co_names) << GS;
    result << formatVector(co_varnames) << GS;
    result << formatVector(co_freevars) << GS;
    result << formatVector(co_cellvars) << GS;

    return result.str();
}
