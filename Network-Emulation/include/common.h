/*******************************************************************************
* common.h
* 
* Utility functions for CSEE4119 Programming Assignment 2.
*
* Author: Yan-Song Chen
* Date  : Jul 2nd, 2018
*******************************************************************************/
#include <sys/time.h>
#include <iostream>
#include <unistd.h>     // errno
#include <cstring>      // strerror
#include <atomic>       // spinlock

#ifndef HW6_COMMMON_H
#define HW6_COMMMON_H
const size_t bufferSize = 2048;
const useconds_t timeoutDuration = 500000;

// Print specified time format
inline void __printmytime() {
	struct timeval timestamp;
	if (gettimeofday(&timestamp, nullptr) == -1) {
		std::cerr << "error: gettimeofday() failed\n" << strerror(errno)
        << std::endl;
		exit(1);
	}
  timestamp.tv_usec = timestamp.tv_usec % 1000; // us to ms
	std::cout << '[' << timestamp.tv_sec << '.' << timestamp.tv_usec << "] ";
}
#define MY_INFO_STREAM __printmytime();std::cout
#define MY_ERROR_STREAM __printmytime();std::cerr

inline void grab_lock(std::atomic_flag& lock) {
  while (lock.test_and_set(std::memory_order_acquire));
}

inline void release_lock(std::atomic_flag& lock) {
  lock.clear(std::memory_order_release);
}

#endif