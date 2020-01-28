#pragma once

struct CppSourceDirectory {
	vector<shared_ptr<CppSourceDirectory>> subdirs;
	vector<shared_ptr<CppFile>> files;

	void initByScanningDirectoryForSourceFilesRecursivelyAndConcurrently(const Path &baseDir, const Path &relativeDir, FileSet *fileSet, ThreadPool::TaskSet *taskSet, ErrorList *errorList);
	void forEachFileRecursively(const function<void(const shared_ptr<CppFile> &)> &f);
};

