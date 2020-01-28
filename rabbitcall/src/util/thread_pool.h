#pragma once

class ThreadPool {

public:
	// A set of tasks that can be run in multiple threads. The main thread can wait until all tasks in the set have completed,
	// including sub-tasks enqueued by the tasks themselves (e.g. prosessing a directory tree recursively).
	class TaskSet {
		ThreadPool *pool = nullptr;
		int numQueuedTasks = 0;
		int numRunningTasks = 0;
		condition_variable emptyCondition;

		bool isEmptyNoSync();

	public:
		explicit TaskSet(ThreadPool *pool);

		void addTask(const function<void()> &task);
		bool isEmpty();
		void waitUntilEmpty();

		friend class ThreadPool;
	};

private:
	struct TaskRef {
		function<void()> task;
		TaskSet *taskSet = nullptr;
		bool valid = false;

		TaskRef() = default;
		TaskRef(const function<void()> &task, TaskSet *taskSet);
	};

	int numThreads = 0;
	int numRunningThreads = 0;
	deque<TaskRef> taskQueue;
	bool stopping = false;

	mutex lock;
	condition_variable threadWakeupCondition;
	condition_variable threadStopWaitCondition;

	vector<shared_ptr<thread>> threads;

	void addTaskFromSet(TaskSet *taskSet, const function<void()> &task);
	void runPoolThread();

public:
	explicit ThreadPool(int numThreads);
	~ThreadPool();

	void start();
	void stop();
	void addTask(const function<void()> &task);
};



