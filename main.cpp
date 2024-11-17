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
#include <thread>
#include <random>
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


int main(int argc, char* argv[]) {
    bool DEBUG = false;

    if (argc > 1 && std::string(argv[1]) == "-d") {
        std::cout << "\033[32mManager started on Debugger Mode...\033[0m" << std::endl;
        DEBUG = true;
    }

    try {
        Code code = readCodeFromJsonFile("code.json");
        Code::globals = code.co_names;
        const std::string ENQ = "ENQ";
        const std::string ACK = "ACK";

        // **** starts here

        std::ifstream file("master_instructions.bin", std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Erro ao abrir o arquivo!" << std::endl;
            return 1;
        }

        code.print();
        std::string whatever = readPayloadFromFile("master_instructions.bin");
        code.updateFromPayload(whatever);
        code.print();
        return 0;

        // Ler o conteúdo do arquivo binário em um vetor de bytes
        std::vector<unsigned char> fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // Criar um vetor para armazenar os trechos separados por nova linha
        std::vector<std::vector<unsigned char>> chunks;
        std::vector<unsigned char> currentChunk;

        // Percorrer o conteúdo do arquivo e dividir pelos caracteres de nova linha (0x0A)
        for (size_t i = 0; i < fileContent.size(); ++i) {
            if (fileContent[i] == 0x0A) {  // Se for uma nova linha (0x0A)
                if (!currentChunk.empty()) {
                    chunks.push_back(currentChunk);
                    currentChunk.clear();
                }
            } else {
                currentChunk.push_back(fileContent[i]);
            }
        }
        
        // Adicionar o último trecho, caso haja
        if (!currentChunk.empty()) {
            chunks.push_back(currentChunk);
        }

        // Agora, percorre o vetor de chunks e separa o primeiro byte como instruction e o restante como payload
        size_t index = 0;
        while (index < chunks.size()) {
            std::cout << "Sinal ENQ recebido.\n";
            std::cout << "Enviando ACK...\n";
            
            unsigned char instruction = chunks[index][0];
            std::vector<unsigned char> payload(chunks[index].begin() + 1, chunks[index].end());

            // Exibir os resultados
            std::cout << "Trecho " << index + 1 << ":" << std::endl;
            std::cout << "Instruction: " << std::hex << static_cast<int>(instruction) << std::dec << std::endl;
            std::cout << "Payload: ";
            for (const auto& byte : payload) {
                std::cout << std::hex << static_cast<int>(byte) << " ";
            }
            std::cout << std::dec << std::endl << std::endl;

            ++index;
        }

        /*
            * INITIALIZE: 0x02
            * MAKE_FUNCTION: 0x84
            * CALL_FUNCTION: 0x83
            * RETURN_VALUE: 0x53
        */

        // std::string line;
        // while (true) {
        //     inputFile.clear();
        //     inputFile.seekg(0, std::ios::beg);

        //     std::getline(inputFile, line);
        //     if (line == ENQ) {
        //         std::getline(inputFile, line);
        //         if (line == "0x84") {
        //             makeFn(currCode, args);
        //         } else if (line == "0x02") {
        //             std::cout << "Executando tarefa 2...\n";
        //         } else if (line == "0x03") {
        //             std::cout << "Executando tarefa 3...\n";
        //         } else {
        //             std::cout << "Comando desconhecido: " << line << '\n';
        //         }
        //     }

        //     if(DEBUG) code.print();

        //     std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Pausa para evitar loop contínuo
        // }

        // inputFile.close();
        // std::cout << "Processo finalizado.\n";

        // generateInputPayloadCases();
        // std::string payload = readPayloadFromFile("input_test_payload.bin");
        // code.updateFromPayload(payload);
        // code.generatePayload();
            
        // CodeNavigator navigator;
        // navigator.push(&code);

        // const Code& newCode = code.getCodeFromConsts(2);
        // navigator.push(&newCode);
        // printf("\n");

        // const Code* currentCode = navigator.peek();
        // currentCode->print(); 

        // currentCode = navigator.pop();
        // currentCode = navigator.peek();
        // currentCode->print(); 

    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
    }

    return 0;
}

