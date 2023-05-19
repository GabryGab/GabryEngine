#include "ProjectDirectory.h"

std::filesystem::path dir = std::filesystem::current_path();
std::string project_directory = dir.string();