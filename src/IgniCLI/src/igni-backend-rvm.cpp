#include <iostream>
#include <string>

int main(const int argc, char** argv)
{
	if (argc >= 2 && std::string(argv[1]) == "--config")
	{
		std::cout << R"({
            "primitiveMapping": {
                "Int": "Int",
                "Double": "Double",
                "Bool": "Bool",
                "String": "String",
                "Any": "Any"
            },
            "ffi_annotations": [ "NativeName", "RvmNative" ]
        })" << std::endl;

		return 0;
	}

	if (argc >= 2 && std::string(argv[1]) == "--compile")
	{
		// TODO: Implement compilation
		std::cout << "[Backend] Compilation mode will be implemented here." << std::endl;
		return 0;
	}

	std::cerr << "Usage:" << std::endl;
	std::cerr << "  igni-backend-rvm --get-config" << std::endl;
	std::cerr << "  igni-backend-rvm --compile <input.json> -o <output>" << std::endl;
	return 1;
}