// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
// Copyright (C) 2010 Yahoo

#pragma once

#include <memory>

namespace vespalib {

/**
 * An executor decouples the execution of a task from the request of
 * executing that task. Also, tasks are typically executed
 * concurrently in multiple threads.
 **/
class Executor
{
public:
    /**
     * A task that can be executed by an executor.
     **/
    struct Task {
        typedef std::unique_ptr<Task> UP;
        virtual void run() = 0;
        virtual ~Task() {}
    };

    /**
     * Execute the given task using one of the internal threads some
     * time in the future. The task may also be rejected in which case
     * it will be returned from this function. A task will be rejected
     * if the executor has been shut down or if the task limit for
     * this executor has been reached.
     *
     * @return the task if rejected, UP(0) if accepted
     * @param task the task to execute
     **/
    virtual Task::UP execute(Task::UP task) = 0;

    /**
     * Empty.
     **/
    virtual ~Executor() {}
};

} // namespace vespalib

