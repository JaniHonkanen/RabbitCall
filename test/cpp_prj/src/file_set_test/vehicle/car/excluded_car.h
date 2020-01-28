#pragma once

typedef int TestTypeDef;

// Should cause an error if RabbitCall tries to parse this file despite being excluded, because typedef is not supported.
FXP struct ExcludedCarStruct {
	TestTypeDef i;
};

