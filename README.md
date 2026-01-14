# Syncronization in Windows

**Syncronization in Windows** is a C-based program designed for simulating a concurrent order processing system as part of a university project on **Operating Systems and Concurrent Programming**. The program implements **Windows API threading** and **synchronization primitives** (such as Mutexes and Semaphores) to manage shared resources and ensure data integrity in a multithreaded environment.

## Features

* **Concurrent Execution:** Simulates multiple threads operating simultaneously, representing different actors in an order management system (e.g., Producers and Consumers).
* **Process Synchronization:** Uses mechanisms to control access to shared memory, preventing race conditions and ensuring data consistency.
* **Resource Management:** Manages a shared buffer or queue of orders with limited capacity.
* **Status Logging:** Real-time console output tracking the creation, processing, and completion of orders.
* **Deadlock Prevention:** Implements logic to handle resource contention without freezing the application.

## Operations

**Thread Roles:**

* **Producer (Client):** Generates new orders and attempts to place them in the shared system buffer.
* **Consumer (Worker):** Retrieves orders from the buffer and processes them.

**System Operations:**

* **Initialize System:** Setup shared memory structures, mutexes, and semaphores.
* **Create Order:** Generate a new order with unique ID and details.
* **Buffer Insertion:** Safely add an order to the queue (waits if full).
* **Buffer Removal:** Safely remove an order from the queue (waits if empty).
* **Process Order:** Simulate the time taken to fulfill an order.
* **Context Switching:** Handle the switching between threads controlled by the Windows Scheduler.

## Technologies

* C Programming Language
* Windows API (Win32 Threads)
* Abstract Data Types (ADT)
* Synchronization Primitives (Mutex, Semaphore)
* Visual Studio (Solution/Project management)

## Installation

**Clone the repository:**
```bash
git clone [https://github.com/joaomartinscode/Syncronization-in-Windows.git](https://github.com/joaomartinscode/Syncronization-in-Windows.git)
