#include <iostream>
#include <string>

int main(const int argc, char** argv)
{
	if (argc >= 2 && std::string(argv[1]) == "--config")
	{
		std::cout << R"({
            "primitiveMapping": {
		        "Int": "System.Int64",
		        "Double": "System.Double",
		        "Bool": "System.Boolean",
		        "String": "System.String",
		        "Unit": "System.Void",
		        "Null": "System.Object",
		        "Any": "System.Object"
            },
			"ffi_annotations": [ "DotNetMethod", "DotNetBaseClass", "DotNetOpcode", "DllExport" ]
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
	std::cerr << "  igni-backend-dotnet --get-config" << std::endl;
	std::cerr << "  igni-backend-dotnet --compile <input.json> -o <output>" << std::endl;
	return 1;
}