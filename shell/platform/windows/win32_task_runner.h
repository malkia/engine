// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_WINDOWS_WIN32_TASK_RUNNER_H_
#define FLUTTER_SHELL_PLATFORM_WINDOWS_WIN32_TASK_RUNNER_H_

#include <windows.h>

#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "flutter/shell/platform/embedder/embedder.h"

namespace flutter {

typedef uint64_t (*CurrentTimeProc)();
// A custom task runner that integrates with user32 GetMessage semantics so that
// host app can own its own message loop and flutter still gets to process
// tasks on a timely basis.
class Win32TaskRunner {
 public:
  using TaskExpiredCallback = std::function<void(const FlutterTask*)>;
  // Creates a new task runner with the given main thread ID, current time
  // provider, and callback for tasks that are ready to be run.
  Win32TaskRunner(DWORD main_thread_id,
                  CurrentTimeProc get_current_time,
                  const TaskExpiredCallback& on_task_expired);

  ~Win32TaskRunner();

  // Returns if the current thread is the thread used by the win32 event loop.
  bool RunsTasksOnCurrentThread() const;

  std::chrono::nanoseconds ProcessTasks();

  // Post a Flutter engine tasks to the event loop for delayed execution.
  void PostTask(FlutterTask flutter_task, uint64_t flutter_target_time_nanos);

 private:
  using TaskTimePoint = std::chrono::steady_clock::time_point;
  struct Task {
    uint64_t order;
    TaskTimePoint fire_time;
    FlutterTask task;

    struct Comparer {
      bool operator()(const Task& a, const Task& b) {
        if (a.fire_time == b.fire_time) {
          return a.order > b.order;
        }
        return a.fire_time > b.fire_time;
      }
    };
  };

  // Returns a TaskTimePoint computed from the given target time from Flutter.
  TaskTimePoint TimePointFromFlutterTime(
      uint64_t flutter_target_time_nanos) const;

  DWORD main_thread_id_;
  CurrentTimeProc get_current_time_;
  TaskExpiredCallback on_task_expired_;
  std::mutex task_queue_mutex_;
  std::priority_queue<Task, std::deque<Task>, Task::Comparer> task_queue_;

  Win32TaskRunner(const Win32TaskRunner&) = delete;

  Win32TaskRunner& operator=(const Win32TaskRunner&) = delete;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_WINDOWS_WIN32_TASK_RUNNER_H_
