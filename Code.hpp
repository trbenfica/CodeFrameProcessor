#ifndef CODE_H
#define CODE_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <variant>
#include <stack>
#include <sstream>
#include <bitset>
#include <iomanip>
#include <unordered_map>
#include <stdexcept>

// Declarações antecipadas
class Code;

// Tipos de dados personalizados
using VarType = std::variant<Code, int, bool, std::string, std::nullptr_t, float>;

// Funções auxiliares
void writeBinaryInt32(std::ostream& os, uint32_t value);
void hexToBinaryStream(const std::string& hex, std::ostream& outStream);

// Classe PayloadType para mapeamento de tipos e códigos
class PayloadType {
public:
    static const std::unordered_map<std::string, std::string> typeMap;
    static const std::unordered_map<std::string, std::string> codeMap;
};

// Declaração da classe Code
class Code {
public:
    // Campos
    std::string co_code;
    std::vector<VarType> co_names;
    std::vector<VarType> co_varnames;
    std::vector<VarType> co_freevars;
    std::vector<VarType> co_cellvars;
    std::vector<VarType> co_consts;
    static std::vector<VarType> globals;

    // Construtores
    explicit Code(const std::string& code = "");

    // Métodos set
    void setCoCode(const std::string& code);
    void setCoNames(const std::vector<VarType>& names);
    void setCoVarnames(const std::vector<VarType>& varnames);
    void setCoFreevars(const std::vector<VarType>& freevars);
    void setCoCellvars(const std::vector<VarType>& cellvars);
    void setCoConsts(const std::vector<VarType>& consts);

    // Métodos de impressão
    void print() const;

    // Acessar objetos Code aninhados
    const Code& getCodeFromVariable(size_t vector, size_t index) const;

    // Modificações do master
    void processMakeFn(int constsIndex, int dstVector, int dstIndexVector);

    // Geração de payloads
    std::string generatePayload() const;
    std::string generateInputTestPayload() const;
    void updateFromPayload(const std::string& payload);
};

#endif
