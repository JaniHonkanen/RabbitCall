// This file was auto-generated by RabbitCall - do not modify manually.


#include "pch.h"

#include "../file_set_test/house/included_file_in_partition2.h"

#include "rabbitcall_generated_main.h"

namespace RabbitCallInternalNamespace {
	
}

using namespace RabbitCallInternalNamespace;

void RabbitCallInternalNamespace::initPartition_partition2(std::string &versionString) {
	versionString += "partition2=1.0.1";
}

_RC_FUNC_EXC(rabbitcall_global_partition2Test(int *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = partition2Test();)
