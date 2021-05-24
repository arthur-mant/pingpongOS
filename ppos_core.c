
int task_count=0;
task_t task_atual, task_main;

void ppos_init() {

    setvbuf(stdout, 0, _IONBF, 0);

    task_main.id = 0;
    getcontext(task_main.context);
}

int task_create (task_t *task, void (*start_routine)(*void), void *arg) {

    if !(getcontext(&task->context))
        return -1;
    if !(makecontext(&task->context , start_routine, arg))
        return -2;
    task->id = ++task_count;

    return task.id;

}

int task_switch (task_t *task) {

    if !(swapcontext(&task_atual.context, &task->context))
        return -1;
    if !(getcontext(&task_atual.context))
        return -2;
    return 0;

}

void task_exit (int exit_code) {

    task_switch (&task_main);

}

int task_id () {

    return task_atual.id;

}
