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
        } else if (item.is_number_float()) {
            consts.push_back(item.get<float>());
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
    std::stack<Code*> navigationStack;

public:
    // Adiciona um novo objeto Code à pilha
    void push(Code* code) {
        navigationStack.push(code);
    }

    // Retorna o objeto Code no topo da pilha
    Code* pop() {
        if (navigationStack.empty()) {
            throw std::runtime_error("Navigation stack is empty.");
        }
        Code* top = navigationStack.top();
        navigationStack.pop();
        return top;
    }

    // Retorna o objeto Code no topo da pilha sem removê-lo
    Code* peek() {
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

std::vector<std::vector<uint8_t>> splitByDelimiters(const std::vector<uint8_t>& data, uint8_t delimiter1, uint8_t delimiter2) {
    std::vector<std::vector<uint8_t>> result;
    std::vector<uint8_t> currentSegment;

    for (size_t i = 0; i < data.size(); ++i) {
        if (i + 1 < data.size() && data[i] == delimiter1 && data[i + 1] == delimiter2) {
            // Encontrei os delimitadores (0x03 seguido de 0x02), salvei o segmento atual
            if (!currentSegment.empty()) {
                result.push_back(currentSegment);
                currentSegment.clear();
            }
            ++i; // Skip the second delimiter
        } else {
            currentSegment.push_back(data[i]);
        }
    }
    // Adiciono o último segmento, se existir
    if (!currentSegment.empty()) {
        result.push_back(currentSegment);
    }

    return result;
}

void printBinaryString(const std::string& binaryString) {
    std::cout << "\033[36m" << "Payload generated:" << std::endl;

    for (unsigned char c : binaryString) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) 
                  << static_cast<int>(c) << " ";
    }
    std::cout << std::dec << "\033[0m" << std::endl; // Retorna o manipulador para decimal
}

int main(int argc, char* argv[]) {
    bool DEBUG = false;

    if (argc > 1 && std::string(argv[1]) == "-d") {
        std::cout << "\n\033[33mManager started on Debugger Mode...\033[0m" << std::endl << std::endl;
        DEBUG = true;
    }

    try {
        Code code = readCodeFromJsonFile("code.json");
        Code::globals = code.co_names;
        const std::string ENQ = "ENQ";
        const std::string ACK = "ACK";

        const char* filename = "master_instructions.bin";
        std::ifstream file(filename, std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return 1;
        }

        std::vector<uint8_t> fileData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        auto segments = splitByDelimiters(fileData, 0x03, 0x02);

        /*
            * INITIALIZE: 0x02
            * RETURN_VALUE: 0x53
            * CALL_FUNCTION: 0x83
        */

        CodeNavigator navigator;
        navigator.push(&code);

        Code* currCode = &code;
        for (const auto& segment : segments) {
            if (segment.empty()) {
                continue;
            }

            // Extrai a instrução e o payload
            uint8_t instruction = segment[0];
            std::vector<uint8_t> payload(segment.begin() + 1, segment.end());

            if(instruction == 0x83) {
                if(DEBUG) std::cout << "--> Instruction: 0x" << std::hex << static_cast<int>(instruction) << std::dec << " (CALL_FUNCTION)" << std::endl;
                std::vector<uint8_t> args(segment.begin() + 1, segment.begin() + 3);
                std::vector<uint8_t> payload(segment.begin() + 4, segment.end());
                std::string payloadString(payload.begin(), payload.end());

                currCode->updateFromPayload(payloadString);
                if(DEBUG) std::cout << "updated current frame from received payload..." << std::endl;

                navigator.push(&currCode->getCodeFromVariable(args[0], args[1]));
                currCode = navigator.peek();
                if(DEBUG) printBinaryString(currCode->generatePayload());
            } else if(instruction == 0x53) {
                if(DEBUG) std::cout << "--> Instruction: 0x" << std::hex << static_cast<int>(instruction) << std::dec << " (RETURN)" << std::endl;
                navigator.pop();
                currCode = navigator.peek();
            } else if(instruction == 0x02) {
                if(DEBUG) std::cout << "\033[32m--> Received START signal, sending first frame:\033[0m" << std::endl;
            } else {
                throw std::runtime_error("Instrução desconhecida");
            }

            if(DEBUG) currCode->print();
            std::cout << std::endl << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
    }

    return 0;
}

