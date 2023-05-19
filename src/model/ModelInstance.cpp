#include "ModelInstance.h"

bool ModelInstance::checkDrawability() {
	bool flag = true;
	if (model == nullptr)
		flag = false;
	return flag;
}