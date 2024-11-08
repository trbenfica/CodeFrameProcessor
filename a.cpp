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
		    
// Estruturas e tipos previamente definidos
class Code;  // Declaração antecipada
using VarType = std::variant<Code, int, bool, std::string, std::nullptr_t>;

// Code* globals = nullptr;
std::vector<VarType>* globals = nullptr;

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
                        std::cout << "Code Object" << std::endl; // Você pode chamar um método print() específico se necessário
                    } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                        std::cout << "null ";
                    } else {
                        std::cout << val << " "; // Imprime o valor
                    }
                }, item);
            }
            std::cout << std::endl;
        };

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

    std::string toBinaryString(int number) const {
        return std::bitset<32>(number).to_string();  // Converte para 32 bits
    }

    std::string formatForMaster() const {
        const char GS = 29;  // ASCII Group Separator
        const char US = 31;  // ASCII Unit Separator
        const char NULL_CHAR = 0;  // ASCII NULL

        // Função auxiliar para converter um int em uma string binária de 32 bits

        std::ostringstream result;
        result << co_code << GS;  // co_code diretamente

        // Função lambda para formatar um vetor
        auto formatVector = [&](const std::vector<VarType>& vec) -> std::string {
            std::ostringstream oss;
            oss << vec.size();
            for (const auto& item : vec) {
                oss << US;
                   std::visit([&oss, this](const auto& val) {
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (std::is_same_v<T, Code>) {
                        oss << NULL_CHAR;  // Representação nula para objetos Code
                    } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                        oss << NULL_CHAR;  // Representação nula para nullptr
                    } else if constexpr (std::is_integral_v<T>) {
                        oss << toBinaryString(val);  // Inteiros em formato binário
                    } else {
                        oss << val;  // Outros tipos são adicionados diretamente
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
            std::visit([&result, this](const auto& val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, Code> || std::is_same_v<T, std::nullptr_t>) {
                    result << NULL_CHAR;  // NULL_CHAR para Code ou nullptr
                } else if constexpr (std::is_integral_v<T>) {
                    std::cout << "CAI";
                    result << toBinaryString(val);  // Inteiros em formato binário
                } else {
                    result << val;  // Outros tipos são adicionados diretamente
                } 
            }, co_consts[i]);
        }


        std::string resultString = result.str();
        std::ofstream outFile("output.bin", std::ios::binary);
        if (outFile) {
            outFile.write(resultString.data(), resultString.size());  // Escreve os dados da string no arquivo binário
        } else {
            std::cerr << "Erro ao abrir o arquivo para escrita!" << std::endl;
        }
        
        return result.str();
    }


    // Método para acessar um objeto Code em co_consts
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
};

// Função para ler o JSON e criar o objeto Code (permanece a mesma)
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


/* Backlog
 *
 * [OK] globals
 * [OK] formatCodeForMaster
 * decodeCodeFromMaster
 * updateframe
 * 
 * acertar protocolo de comunicação (flags, espera, etc.)
 * testar: mais profundidade

 * como mapear os tipos do objeto code?
 * instruções de OOP?
 *
 * */

int main() {
    try {
        Code code = readCodeFromJsonFile("code.json");
        globals = &code.co_names;
        // printf("globals: \n");
        // globals->print();
        // code.print(); 

        code.formatForMaster();
            
        CodeNavigator navigator;
        navigator.push(&code);

        const Code& newCode = code.getCodeFromConsts(2);
        navigator.push(&newCode);
        printf("\n");

        const Code* currentCode = navigator.peek();
        // currentCode->print(); 
        printf("\n");

        currentCode = navigator.pop();
        currentCode = navigator.peek();
        // currentCode->print(); 

    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
    }

    return 0;
}

