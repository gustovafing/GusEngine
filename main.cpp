#include "utils/logger.h"
#include "core/engine.h"
#include "core/globals.h"
#include <iostream>
int32_t main(int32_t argc, char* argv[]) {
	std::string current_exec = argv[0];
	std::vector<std::string> args;
	if (argc > 1) {
		args.assign(argv + 1, argv + argc);
	}

	try {
		Engine engine;
		engine.Run(args);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}