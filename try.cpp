#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <fstream>
#include "include/json.hpp"

using json = nlohmann::json;

class Code {
public:
    std::string co_code;
    std::vector<std::string> co_names;
    std::vector<std::string> co_varnames;
    std::vector<std::string> co_freevars;
    std::vector<std::string> co_cellvars;
    using ConstType = std::variant<std::string, double, std::shared_ptr<Code>, std::nullptr_t>;
    std::vector<ConstType> co_consts;

    Code(std::string code) : co_code(std::move(code)) {}

    void display() const {
	if (!co_code.empty()) {
	    std::cout << "co_code: " << co_code << std::endl;
	}

	if (!co_names.empty()) {
	    std::cout << "co_names: ";
	    for (const auto& name : co_names) {
		std::cout << name << " ";
	    }
	    std::cout << std::endl;
	}

	if (!co_varnames.empty()) {
	    std::cout << "co_varnames: ";
	    for (const auto& varname : co_varnames) {
		std::cout << varname << " ";
	    }
	    std::cout << std::endl;
	}

	if (!co_freevars.empty()) {
	    std::cout << "co_freevars: ";
	    for (const auto& freevar : co_freevars) {
		std::cout << freevar << " ";
	    }
	    std::cout << std::endl;
	}

	if (!co_cellvars.empty()) {
	    std::cout << "co_cellvars: ";
	    for (const auto& cellvar : co_cellvars) {
		std::cout << cellvar << " ";
	    }
	    std::cout << std::endl;
	}

	if (!co_consts.empty()) {
	    std::cout << "co_consts: ";
	    for (const auto& const_item : co_consts) {
		std::visit([](const auto& value) {
			using T = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<T, std::shared_ptr<Code>>) {
			std::cout << "Nested Code: ";
			if (value) value;
			} else if constexpr (std::is_same_v<T, std::string>) {
			std::cout << "String: " << value << " ";
			} else if constexpr (std::is_same_v<T, double>) {
			std::cout << "Number: " << value << " ";
			} else if constexpr (std::is_same_v<T, std::nullptr_t>) {
			std::cout << "null ";
			}
			}, const_item);
	    }
	    std::cout << std::endl;
	}
}

    void loadFromJson(const json& j) {
        co_code = j.at("co_code").get<std::string>();
        co_names = j.at("co_names").get<std::vector<std::string>>();
        co_varnames = j.at("co_varnames").get<std::vector<std::string>>();
        co_freevars = j.at("co_freevars").get<std::vector<std::string>>();
        co_cellvars = j.at("co_cellvars").get<std::vector<std::string>>();

        const auto& consts = j.at("co_consts");
        for (const auto& item : consts) {
            if (item.is_string()) {
                co_consts.emplace_back(item.get<std::string>());
            } else if (item.is_number()) {
                co_consts.emplace_back(item.get<double>());
            } else if (item.is_null()) {
                co_consts.emplace_back(nullptr);
            } else if (item.is_object()) {
                auto nested_code = std::make_shared<Code>("");
                nested_code->loadFromJson(item);
                co_consts.emplace_back(nested_code);
            }
        }
    }

    std::shared_ptr<Code> getNestedCode(size_t index) const {
        if (index >= co_consts.size()) {
            std::cerr << "Index out of range." << std::endl;
            return nullptr;
        }
        if (auto nested_code_ptr = std::get_if<std::shared_ptr<Code>>(&co_consts[index])) {
            return *nested_code_ptr;
        } else {
            std::cerr << "Item at the given index is not a Code object." << std::endl;
            return nullptr;
        }
    }
};

std::shared_ptr<Code> readCodeFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    json j;
    file >> j;

    auto code = std::make_shared<Code>("");
    code->loadFromJson(j);

    return code;
}

int main() {
    try {
        auto code = readCodeFromFile("code.json");
        std::cout << "Original Code:" << std::endl;
        code->display();

        auto nested_code = code->getNestedCode(2); // Exemplo de acesso ao primeiro item de co_consts
        if (nested_code) {
            std::cout << "\nAcessando o Code filho:" << std::endl;
            nested_code->display();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}

