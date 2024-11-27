#include "Code.hpp"
#include <random>

uint8_t recordSeparator[2] = {0x03, 0x02};

/**
 * Esta função gera uma instrução de CALL_FUNCTION no fluxo.
 * 
 * @param outfile O arquivo de saída onde a função será escrita.
 * @param codeObj é o objeto Code a ser convertido.
 * @param dstVector Vetor de destino.
 * @param dstIndexVector Índice do vetor de destino.
 */
void generateCallFn(std::ofstream& outfile, Code& codeObj, uint8_t dstVector, uint8_t dstIndexVector) {
    if (!outfile.is_open()) {
        throw std::runtime_error("Arquivo não está aberto para escrita.");
    }

    uint8_t fixedByte = 0x83;
    
    std::string serializedStr = codeObj.generateInputTestPayload();

    outfile.write(reinterpret_cast<const char*>(&fixedByte), sizeof(fixedByte));
    outfile.write(reinterpret_cast<const char*>(&dstVector), sizeof(dstVector));
    outfile.write(reinterpret_cast<const char*>(&dstIndexVector), sizeof(dstIndexVector));
    outfile.write(reinterpret_cast<const char*>(serializedStr.data()), serializedStr.size());
    outfile.write(reinterpret_cast<const char*>(&recordSeparator), sizeof(recordSeparator));
}

// Gera uma instrução de RETURN no fluxo
void generateReturn(std::ofstream& outfile) {
    if (!outfile.is_open()) {
        throw std::runtime_error("Arquivo não está aberto para escrita.");
    }

    uint8_t returnValue = 0x53;

    outfile.write(reinterpret_cast<const char*>(&returnValue), sizeof(returnValue));
    outfile.write(reinterpret_cast<const char*>(&recordSeparator), sizeof(recordSeparator));
}

int main() {
    // Nome do arquivo binário a ser criado
    const char* filename = "master_instructions.bin";

    // Abre o arquivo em modo binário
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile) {
        throw std::runtime_error("Erro ao abrir o arquivo para escrita!");
    }

    // Insere instrução de START
    char INIT = 0x02;
    outfile.write(&INIT, sizeof(INIT));
    outfile.write(reinterpret_cast<const char*>(&recordSeparator), sizeof(recordSeparator));
    
    // primeiro frame
    Code codeObj, childCode;

    Code::globals = {};
    codeObj.setCoNames(std::vector<VarType>{0});
    codeObj.setCoVarnames(std::vector<VarType>{});
    codeObj.setCoFreevars(std::vector<VarType>{});
    codeObj.setCoCellvars(std::vector<VarType>{});
    generateCallFn(outfile, codeObj, 1, 0);

  
    generateReturn(outfile);

    // Fechar os arquivos
    outfile.close();

    if (!outfile.good()) {
        throw std::runtime_error("Erro ao escrever no arquivo binário!");
    }

    std::cout << "Arquivo binário criado com sucesso!" << std::endl;

    return 0;
}

