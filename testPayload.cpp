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
    const char* outputModel = "output_model.txt";

    // Abre o arquivo em modo binário
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile) {
        throw std::runtime_error("Erro ao abrir o arquivo para escrita!");
    }

    // Redireciona std::cout para o arquivo outputModel
    std::ofstream modelFile(outputModel);
    if (!modelFile) {
        throw std::runtime_error("Erro ao abrir o arquivo para saída do modelo!");
    }
    std::streambuf* originalCoutBuffer = std::cout.rdbuf(); // Salva o buffer original
    std::cout.rdbuf(modelFile.rdbuf()); // Redireciona para o arquivo

    // Insere instrução de START
    char INIT = 0x02;
    char recordSeparator = 0x1E; // Certifique-se de definir o valor de recordSeparator
    outfile.write(&INIT, sizeof(INIT));
    outfile.write(reinterpret_cast<const char*>(&recordSeparator), sizeof(recordSeparator));
    
    // primeiro frame
    Code codeObj, childCode;

    Code::globals = {50};
    codeObj.setCoNames(std::vector<VarType>{10});
    codeObj.setCoVarnames(std::vector<VarType>{30, 2, "two", childCode, true});
    codeObj.setCoFreevars(std::vector<VarType>{50, 60, "three", childCode, false});
    codeObj.setCoCellvars(std::vector<VarType>{70, 80, "four", childCode, true});
    generateCallFn(outfile, codeObj, 2, 1);
    codeObj.print();
    std::cout << std::endl;

    // segundo frame
    codeObj.setCoNames(std::vector<VarType>{10});
    codeObj.setCoVarnames(std::vector<VarType>{30, 2, "two", childCode, true});
    codeObj.setCoFreevars(std::vector<VarType>{50, 60, "three", childCode, false});
    codeObj.setCoCellvars(std::vector<VarType>{70, 80, "four", childCode, true});
    generateCallFn(outfile, codeObj, 2, 1);
    codeObj.print();
    std::cout << std::endl;

    // terceiro frame
    codeObj.setCoNames(std::vector<VarType>{10});
    codeObj.setCoVarnames(std::vector<VarType>{30, 2, "two", childCode, true});
    codeObj.setCoFreevars(std::vector<VarType>{50, 60, "three", childCode, false});
    codeObj.setCoCellvars(std::vector<VarType>{70, 80, "four", childCode, true});
    generateCallFn(outfile, codeObj, 2, 1);
    codeObj.print();
    std::cout << std::endl;

    // quarto frame
    codeObj.setCoNames(std::vector<VarType>{10});
    codeObj.setCoVarnames(std::vector<VarType>{30, 2, "two", childCode, true});
    codeObj.setCoFreevars(std::vector<VarType>{50, 60, "three", childCode, false});
    codeObj.setCoCellvars(std::vector<VarType>{70, 80, "four", childCode, true});
    generateCallFn(outfile, codeObj, 2, 1);
    codeObj.print();
    std::cout << std::endl;

    // retorna ao original
    generateReturn(outfile);
    generateReturn(outfile);
    generateReturn(outfile);
    generateReturn(outfile);

    // Restaura o buffer original de std::cout
    std::cout.rdbuf(originalCoutBuffer);

    // Fechar os arquivos
    outfile.close();
    modelFile.close();

    if (!outfile.good()) {
        throw std::runtime_error("Erro ao escrever no arquivo binário!");
    }
    if (!modelFile.good()) {
        throw std::runtime_error("Erro ao escrever no arquivo de modelo!");
    }

    std::cout << "Arquivo binário e modelo criados com sucesso!" << std::endl;

    return 0;
}

