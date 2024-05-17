# DeadLock_Detection
For Kernel Deadlock Detection integrating algoritms into operating system's kernel to enable:
- real-time monitoring
- resource allocation
- graph construction
- deadlock detection


# Wait-for Graph
- representing the dependencies between processes waiting for resources.
- detecting deadlocks by traversing the wait-for graph and identifying cycles.

# Kernel Hooks
Integration points within the kernel where the deadlock detector can intercept and monitor resource allocation, process state changes, and other relevant events.

# solution Mechanisms:
	
	- Rollback of Operations: In some cases, it might be necessary to roll back certain operations or transactions to a safe state to resolve deadlocks.
