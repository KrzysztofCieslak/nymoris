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
    pub kernel_stack_top: u64,
    pub entry: Option<fn()>,
    pub is_userspace: bool,
    pub userspace_entry: u64,
    pub userspace_stack: u64,
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

pub unsafe fn init(kernel_stack_top: u64) {
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
        kernel_stack_top,
        entry: None,
        is_userspace: false,
        userspace_entry: 0,
        userspace_stack: 0,
    });
    CURRENT_TASK_IDX = 0;
    NEXT_ID = 1;
    crate::gdt::set_rsp0(kernel_stack_top);
    println!("[SCHEDULER] Initialized, task 0 (idle/shell) running");
}

/// Spawn a new kernel thread. Returns task ID on success.
pub unsafe fn spawn(name: &'static str, entry: fn()) -> Option<u64> {
    let idx = (1..MAX_TASKS).find(|&i| TASKS[i].is_none())?;
    let id = NEXT_ID;
    NEXT_ID += 1;

    let stack = memory::allocator::alloc_page()?;
    let stack_top = stack + STACK_SIZE as u64;

    let initial_rsp = stack_top - 8 * 7;

    core::ptr::write((stack_top - 8) as *mut u64, task_wrapper as *const () as u64);
    core::ptr::write((stack_top - 16) as *mut u64, 0);
    core::ptr::write((stack_top - 24) as *mut u64, 0);
    core::ptr::write((stack_top - 32) as *mut u64, 0);
    core::ptr::write((stack_top - 40) as *mut u64, 0);
    core::ptr::write((stack_top - 48) as *mut u64, 0);
    core::ptr::write((stack_top - 56) as *mut u64, 0);

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
        kernel_stack_top: stack_top,
        entry: Some(entry),
        is_userspace: false,
        userspace_entry: 0,
        userspace_stack: 0,
    });

    println!("[SCHEDULER] Spawned task {}: '{}' at idx {}", id, name, idx);
    Some(id)
}

/// Spawn a userspace task. Returns task ID on success.
pub unsafe fn spawn_userspace(
    name: &'static str,
    entry: u64,
    user_stack: u64,
) -> Option<u64> {
    let idx = (1..MAX_TASKS).find(|&i| TASKS[i].is_none())?;
    let id = NEXT_ID;
    NEXT_ID += 1;

    // Allocate a kernel stack for this userspace task (interrupts, syscalls).
    let kstack = memory::allocator::alloc_page()?;
    let kstack_top = kstack + STACK_SIZE as u64;

    let initial_rsp = kstack_top - 8 * 7;

    core::ptr::write((kstack_top - 8) as *mut u64, userspace_trampoline as *const () as u64);
    core::ptr::write((kstack_top - 16) as *mut u64, 0);
    core::ptr::write((kstack_top - 24) as *mut u64, 0);
    core::ptr::write((kstack_top - 32) as *mut u64, 0);
    core::ptr::write((kstack_top - 40) as *mut u64, 0);
    core::ptr::write((kstack_top - 48) as *mut u64, 0);
    core::ptr::write((kstack_top - 56) as *mut u64, 0);

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
        stack_bottom: kstack,
        kernel_stack_top: kstack_top,
        entry: None,
        is_userspace: true,
        userspace_entry: entry,
        userspace_stack: user_stack,
    });

    println!(
        "[SCHEDULER] Spawned userspace task {}: '{}' at idx {} (entry={:x}, stack={:x})",
        id, name, idx, entry, user_stack
    );
    Some(id)
}

/// Entry point for new kernel tasks. Called via ret from switch_context.
extern "C" fn task_wrapper() {
    unsafe {
        x86_64::instructions::interrupts::enable();
        let idx = CURRENT_TASK_IDX;
        if let Some(ref task) = TASKS[idx] {
            if let Some(entry) = task.entry {
                entry();
            }
        }
        if let Some(ref mut task) = TASKS[idx] {
            println!("[SCHEDULER] Task {} '{}' exited", task.id, task.name);
            task.state = TaskState::Zombie;
        }
        loop {
            yield_cpu();
        }
    }
}

/// Entry point for userspace tasks. Called via ret from switch_context.
extern "C" fn userspace_trampoline() {
    unsafe {
        println!("[SCHEDULER] userspace_trampoline entered");
        let idx = CURRENT_TASK_IDX;
        let (entry, stack) = if let Some(ref task) = TASKS[idx] {
            (task.userspace_entry, task.userspace_stack)
        } else {
            println!("[SCHEDULER] userspace_trampoline: no task!");
            loop {}
        };

        // Set RSP0 for this userspace task's syscalls.
        if let Some(ref task) = TASKS[idx] {
            crate::gdt::set_rsp0(task.kernel_stack_top);
        }

        println!("[SCHEDULER] calling enter_userspace({:x}, {:x})", entry, stack);
        enter_userspace(entry, stack);
    }
}

core::arch::global_asm!(
    ".globl enter_userspace_asm",
    "enter_userspace_asm:",
    // Arguments: RDI = entry, RSI = stack_top
    // Get user CS (0x2B) and SS (0x23) from GDT selectors
    "mov rax, 0x23",        // user SS
    "push rax",
    "push rsi",             // user stack top
    "push 0x202",           // RFLAGS with IF=1
    "mov rax, 0x2B",        // user CS
    "push rax",
    "push rdi",             // entry point
    "iretq",
);

extern "C" {
    fn enter_userspace_asm(entry: u64, stack_top: u64);
}

/// Enter ring 3 via iretq. Never returns.
pub unsafe fn enter_userspace(entry: u64, stack_top: u64) -> ! {
    enter_userspace_asm(entry, stack_top);
    loop {
        x86_64::instructions::hlt();
    }
}

/// Mark the current task as Zombie and switch away. Called from exit syscall.
pub unsafe fn exit_current_task() -> ! {
    let idx = CURRENT_TASK_IDX;
    if let Some(ref mut task) = TASKS[idx] {
        println!("[SCHEDULER] Task {} '{}' exited", task.id, task.name);
        task.state = TaskState::Zombie;
    }
    schedule();
    // Should never return - if we do, halt.
    loop {
        x86_64::instructions::hlt();
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
        return;
    }

    if let Some(ref mut task) = TASKS[current_idx] {
        task.state = TaskState::Ready;
    }

    if let Some(ref mut task) = TASKS[next_idx] {
        task.state = TaskState::Running;
    }

    let old = TASKS[current_idx].as_mut().unwrap() as *mut Task;
    let new = TASKS[next_idx].as_mut().unwrap() as *mut Task;

    CURRENT_TASK_IDX = next_idx;

    // Update RSP0 for the new task (critical for userspace syscalls).
    if let Some(ref task) = TASKS[next_idx] {
        crate::gdt::set_rsp0(task.kernel_stack_top);
    }

    switch_context(&mut (*old).context, &(*new).context);
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
            let kind = if task.is_userspace { "U" } else { "K" };
            println!(
                " {} [{}] {}: {} (state: {}, type: {})",
                marker, i, task.id, task.name, state_str, kind
            );
        }
    }
}
