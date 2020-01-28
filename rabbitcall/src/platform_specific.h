#pragma once

void setProcessPriorityToLow();
FILE * openFileOrThrow(const filesystem::path &path, const string &mode, int64_t *fileSizeOut);

