use crate::{println, memory};

const MAX_TASKS: usize = 16;
const STACK_PAGES: usize = 1;
const STACK_SIZE: usize = STACK_PAGES * 4096;

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum TaskState {
    Ready,
    Running,
    Blocked,
    Zombie,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct TaskContext {
    pub rsp: u64,
    pub rbp: u64,
    pub rbx: u64,
    pub r12: u64,
    pub r13: u64,
    pub r14: u64,
    pub r15: u64,
}

#[derive(Clone, Copy, Debug)]
pub struct Task {
    pub id: u64,
    pub name: &'static str,
    pub state: TaskState,
    pub context: TaskContext,
    pub stack_bottom: u64,
    pub entry: Option<fn()>,
}

unsafe impl Sync for Task {}

static mut TASKS: [Option<Task>; MAX_TASKS] = [None; MAX_TASKS];
static mut CURRENT_TASK_IDX: usize = 0;
static mut NEXT_ID: u64 = 1;


core::arch::global_asm!(
    ".globl switch_context",
    "switch_context:",
    "push rbp",
    "push rbx",
    "push r12",
    "push r13",
    "push r14",
    "push r15",
    "mov [rdi], rsp",
    "mov rsp, [rsi]",
    "pop r15",
    "pop r14",
    "pop r13",
    "pop r12",
    "pop rbx",
    "pop rbp",
    "ret",
);

extern "C" {
    fn switch_context(old: *mut TaskContext, new: *const TaskContext);
}

pub unsafe fn init() {
    TASKS[0] = Some(Task {
        id: 0,
        name: "idle/shell",
        state: TaskState::Running,
        context: TaskContext {
            rsp: 0,
            rbp: 0,
            rbx: 0,
            r12: 0,
            r13: 0,
            r14: 0,
            r15: 0,
        },
        stack_bottom: 0,
        entry: None,
    });
    CURRENT_TASK_IDX = 0;
    NEXT_ID = 1;
    println!("[SCHEDULER] Initialized, task 0 (idle/shell) running");
}

/// Spawn a new kernel thread. Returns task ID on success.
pub unsafe fn spawn(name: &'static str, entry: fn()) -> Option<u64> {
    let idx = (1..MAX_TASKS).find(|&i| TASKS[i].is_none())?;
    let id = NEXT_ID;
    NEXT_ID += 1;

    let stack = memory::allocator::alloc_page()?;
    let stack_top = stack + STACK_SIZE as u64;

    // Set up initial stack for new task.
    // switch_context pops in order: r15, r14, r13, r12, rbx, rbp, then ret.
    let initial_rsp = stack_top - 8 * 7;

    core::ptr::write((stack_top - 8) as *mut u64, task_wrapper as *const () as u64);
    core::ptr::write((stack_top - 16) as *mut u64, 0); // rbp
    core::ptr::write((stack_top - 24) as *mut u64, 0); // rbx
    core::ptr::write((stack_top - 32) as *mut u64, 0); // r12
    core::ptr::write((stack_top - 40) as *mut u64, 0); // r13
    core::ptr::write((stack_top - 48) as *mut u64, 0); // r14
    core::ptr::write((stack_top - 56) as *mut u64, 0); // r15

    TASKS[idx] = Some(Task {
        id,
        name,
        state: TaskState::Ready,
        context: TaskContext {
            rsp: initial_rsp,
            rbp: 0,
            rbx: 0,
            r12: 0,
            r13: 0,
            r14: 0,
            r15: 0,
        },
        stack_bottom: stack,
        entry: Some(entry),
    });

    println!("[SCHEDULER] Spawned task {}: '{}' at idx {}", id, name, idx);
    Some(id)
}

/// Entry point for new tasks. Called via ret from switch_context.
/// We entered from interrupt context (interrupts disabled), so sti is needed.
extern "C" fn task_wrapper() {
    unsafe {
        x86_64::instructions::interrupts::enable();
        let idx = CURRENT_TASK_IDX;
        if let Some(ref task) = TASKS[idx] {
            if let Some(entry) = task.entry {
                entry();
            }
        }
        // If entry returned, mark as zombie and yield away forever.
        if let Some(ref mut task) = TASKS[idx] {
            println!("[SCHEDULER] Task {} '{}' exited", task.id, task.name);
            task.state = TaskState::Zombie;
        }
        loop {
            yield_cpu();
        }
    }
}

/// Voluntarily give up the CPU.
pub fn yield_cpu() {
    unsafe {
        schedule();
    }
}

/// Switch to the next ready task. Called from yield_cpu() or timer interrupt.
pub unsafe fn schedule() {
    let current_idx = CURRENT_TASK_IDX;

    // Simple round-robin: find next ready task.
    let mut next_idx = current_idx;
    for i in 1..MAX_TASKS {
        let idx = (current_idx + i) % MAX_TASKS;
        if let Some(ref task) = TASKS[idx] {
            if task.state == TaskState::Ready {
                next_idx = idx;
                break;
            }
        }
    }

    if next_idx == current_idx {
        return; // No other ready task.
    }

    // Mark current as ready.
    if let Some(ref mut task) = TASKS[current_idx] {
        task.state = TaskState::Ready;
    }

    // Mark next as running.
    if let Some(ref mut task) = TASKS[next_idx] {
        task.state = TaskState::Running;
    }

    let old = TASKS[current_idx].as_mut().unwrap() as *mut Task;
    let new = TASKS[next_idx].as_mut().unwrap() as *mut Task;

    CURRENT_TASK_IDX = next_idx;

    switch_context(&mut (*old).context, &(*new).context);

    // When we return here, we are executing in the context of the task
    // that was just switched back to. The caller (schedule in the old task)
    // will return to its caller (yield_cpu or timer_interrupt_rust).
}

/// List all tasks.
pub unsafe fn list_tasks() {
    println!("Tasks:");
    for i in 0..MAX_TASKS {
        if let Some(ref task) = TASKS[i] {
            let state_str = match task.state {
                TaskState::Ready => "Ready",
                TaskState::Running => "Running",
                TaskState::Blocked => "Blocked",
                TaskState::Zombie => "Zombie",
            };
            let marker = if i == CURRENT_TASK_IDX { "*" } else { " " };
            println!(
                " {} [{}] {}: {} (state: {})",
                marker, i, task.id, task.name, state_str
            );
        }
    }
}

