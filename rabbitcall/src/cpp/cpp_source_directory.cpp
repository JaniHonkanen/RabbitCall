#include "pch.h"


void CppSourceDirectory::initByScanningDirectoryForSourceFilesRecursivelyAndConcurrently(const Path &baseDir, const Path &relativeDir, FileSet *fileSet, ThreadPool::TaskSet *taskSet, ErrorList *errorList) {
	subdirs.clear();
	files.clear();

	// Sort the files and subdirectories so that the output is the same regardless of in which order the file system lists the files.
	map<string, shared_ptr<CppFile>> filesByName;
	set<string> directories;

	for (const filesystem::directory_entry &entry : filesystem::directory_iterator((baseDir / relativeDir).path())) {
		Path filename(entry.path().filename());
		string filenameString = filename.toString();
		Path relativePath(relativeDir / filename);
		string relativePathString = relativePath.toString();

		if (entry.is_directory()) {
			if (filenameString != "." && filenameString != "..") {
				if (!fileSet || fileSet->canDirectoryContainPathsInSet(relativePathString)) {
					directories.insert(relativePathString);
				}
			}
		}
		else {
			if (!fileSet || fileSet->isPathInSet(relativePathString)) {
				string extensionLowerCase = boost::algorithm::to_lower_copy(Path(filenameString).path().extension().string());
				bool isHeaderFile = (extensionLowerCase == ".h" || extensionLowerCase == ".hpp");
				bool isSourceFile = (extensionLowerCase == ".c" || extensionLowerCase == ".cpp" || extensionLowerCase == ".cxx" || extensionLowerCase == ".cc");
				if (isHeaderFile || isSourceFile) {
					
					filesByName[relativePathString] = make_shared<CppFile>((baseDir / relativePath).toString(), entry.file_size(), entry.last_write_time().time_since_epoch().count(), isHeaderFile);
				}
			}
		}
	}

	for (auto &entry : filesByName) {
		files.push_back(entry.second);
	}

	for (const string &path : directories) {
		// Put the subdirectories in the list in original order even if they are handled in threads in a different order.
		shared_ptr<CppSourceDirectory> subdir = make_shared<CppSourceDirectory>();
		subdirs.push_back(subdir);

		auto task = [=] {
			errorList->runWithExceptionCheck([&] {
				subdir->initByScanningDirectoryForSourceFilesRecursivelyAndConcurrently(baseDir, Path(path), fileSet, taskSet, errorList);
			});
		};
		
		if (taskSet) {
			taskSet->addTask(task);
		}
		else {
			task();
		}
	}
}

void CppSourceDirectory::forEachFileRecursively(const function<void(const shared_ptr<CppFile> &)> &f) {
	for (const shared_ptr<CppFile> &file : files) {
		f(file);
	}

	for (const auto &dir : subdirs) {
		dir->forEachFileRecursively(f);
	}
}


