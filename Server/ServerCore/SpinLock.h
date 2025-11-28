#pragma once
class SpinLock {
public:
	void lock() 
	{
		bool expected = false;
		bool desired = true;


		while (flag.compare_exchange_strong(expected, desired) == false) 
		{
			expected = false;
			this_thread::yield();
		}

	}
	void unlock() {
		flag = false;
	}

private:
	atomic<bool> flag = false;
};