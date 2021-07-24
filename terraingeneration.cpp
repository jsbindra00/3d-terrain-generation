
#include "GLTools.h"
int main() {
	if (!nr::driver::init::InitProgram(500, 500, "testing")) {
		std::cout << "oh no" << std::endl;
		return -1;
	}
	nr::driver::Render();
}