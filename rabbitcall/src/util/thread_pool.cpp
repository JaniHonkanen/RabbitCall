#include "pch.h"


bool ThreadPool::TaskSet::isEmptyNoSync() {
	return numQueuedTasks <= 0 && numRunningTasks <= 0;
}

ThreadPool::TaskSet::TaskSet(ThreadPool *pool)
	: pool(pool) {
}

void ThreadPool::TaskSet::addTask(const function<void()> &task) {
	pool->addTaskFromSet(this, task);
}

bool ThreadPool::TaskSet::isEmpty() {
	lock_guard<mutex> _(pool->lock);
	return isEmptyNoSync();
}

void ThreadPool::TaskSet::waitUntilEmpty() {
	unique_lock<mutex> currentLock(pool->lock);
	while (!isEmptyNoSync()) {
		emptyCondition.wait_for(currentLock, std::chrono::milliseconds(1000));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ThreadPool::TaskRef::TaskRef(const function<void()> &task, TaskSet *taskSet)
	: task(task), taskSet(taskSet), valid(true) {
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ThreadPool::addTaskFromSet(TaskSet *taskSet, const function<void()> &task) {
	lock_guard<mutex> _(lock);
	taskQueue.emplace_back(task, taskSet);
	threadWakeupCondition.notify_one();
	if (taskSet) {
		taskSet->numQueuedTasks++;
	}
}

void ThreadPool::runPoolThread() {
	try {
		TaskRef task;

		{
			unique_lock<mutex> currentLock(lock);
			numRunningThreads++;
		}

		while (true) {
			{
				unique_lock<mutex> currentLock(lock);

				if (stopping) {
					numRunningThreads--;
					threadStopWaitCondition.notify_all();
					break;
				}

				// Check if a task was completed on the previous round of the loop.
				if (task.valid) {
					if (task.taskSet) {
						if (--task.taskSet->numRunningTasks < 0) EXC("Number of running tasks in thread pool cannot become negative");
						if (task.taskSet->isEmptyNoSync()) {
							task.taskSet->emptyCondition.notify_all();
						}
					}
					task.valid = false;
				}
				
				if (taskQueue.empty()) {
					threadWakeupCondition.wait_for(currentLock, std::chrono::milliseconds(1000));
				}
				else {
					task = taskQueue.front();
					taskQueue.pop_front();
					if (task.taskSet) {
						if (--task.taskSet->numQueuedTasks < 0) EXC("Number of queued tasks in thread pool cannot become negative");
						task.taskSet->numRunningTasks++;
					}
				}
			}

			if (task.valid) {
				task.task();
			}
		}
	}
	catch (const exception &e) {
		LOG_ERROR(sb() << "Error in background thread: " << e.what());
		exit(applicationErrorReturnCode);
	}
	catch (...) {
		LOG_ERROR("Error in background thread");
		exit(applicationErrorReturnCode);
	}
}

ThreadPool::ThreadPool(int numThreads)
	: numThreads(numThreads) {
}

ThreadPool::~ThreadPool() {
	stop();
}

void ThreadPool::start() {
	for (int i = 0; i < numThreads; i++) {
		shared_ptr<thread> t = make_shared<thread>([this] {
			runPoolThread();
		});
		threads.push_back(t);
	}
}

void ThreadPool::stop() {
	{
		unique_lock<mutex> _(lock);
		stopping = true;
		threadWakeupCondition.notify_all();
	}

	bool successfullyStopped = false;
	double waitStartTime = getTimeSeconds();
	while (true) {
		unique_lock<mutex> currentLock(lock);
		if (numRunningThreads <= 0) {
			successfullyStopped = true;
			break;
		}
		if (getTimeSeconds() - waitStartTime >= 2.0) {
			cout << "Could not stop pool threads" << endl;
		}
		threadStopWaitCondition.wait_for(currentLock, std::chrono::milliseconds(10));
	}

	// If the threads successfully exited their loops, wait until the threads have actually stopped,
	// otherwise detach the threads so that they will be forcefully stopped the application exits
	// (must call detach() before destroying the thread object).
	for (auto &t : threads) {
		if (successfullyStopped) {
			t->join();
		}
		else {
			t->detach();
		}
	}

	threads.clear();
}

void ThreadPool::addTask(const function<void()> &task) {
	addTaskFromSet(nullptr, task);
}
