#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <variant>
#include "include/json.hpp"

// Estruturas e tipos previamente definidos
class Code;  // Declaração antecipada
using VarType = std::variant<Code, int, bool, std::string, std::nullptr_t>;

class Code {
private:
    std::string co_code;
    std::vector<VarType> co_names;
    std::vector<VarType> co_varnames;
    std::vector<VarType> co_freevars;
    std::vector<VarType> co_cellvars;
    std::vector<VarType> co_consts;

public:
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
    // Criando o objeto Code a partir do Frame
    Code codeObj;

    // Transferindo os campos
    codeObj.setCoCode(jsonData["co_code"].get<std::string>());

    // Para co_names, co_varnames, co_freevars e co_cellvars, apenas contamos as strings
    size_t namesSize = jsonData["co_names"].size();
    size_t varnamesSize = jsonData["co_varnames"].size();
    size_t freevarsSize = jsonData["co_freevars"].size();
    size_t cellvarsSize = jsonData["co_cellvars"].size();

    // Inicializa os vetores com nullptr
    codeObj.setCoNames(std::vector<VarType>(namesSize, nullptr));
    codeObj.setCoVarnames(std::vector<VarType>(varnamesSize, nullptr));
    codeObj.setCoFreevars(std::vector<VarType>(freevarsSize, nullptr));
    codeObj.setCoCellvars(std::vector<VarType>(cellvarsSize, nullptr));

    // Para co_consts, transferimos os valores
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
            // Caso seja um Frame aninhado, cria um novo objeto Code
            consts.push_back(readCodeFromJson(item)); // Chamada recursiva
        }
    }
    codeObj.setCoConsts(consts);

    return codeObj; // Retorna o objeto Code criado
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

int main() {
    try {
        Code code = readCodeFromJsonFile("code.json");
        code.print(); // Imprime os dados do objeto Code

        // Acessando um objeto Code a partir de co_consts
        const Code& newCode = code.getCodeFromConsts(2);
        newCode.print(); // Imprime os dados do novo objeto Code

    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
    }

    return 0;
}

