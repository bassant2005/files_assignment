import java.util.*;

///This class implements the Preemptive priority Scheduler
///Scheduling Algorithm with Context Switching time.

public class PriorityScheduler {
    private List<Process> processes;

    /// Stores the order of execution
    private List<String> executionOrder;

    /// Time penalty when switching between processes
    private int contextSwitchTime ;

    /// wait counters for AGING
    private Map<String, Integer> waitCounters;  // processName → waiting time

    public PriorityScheduler(List<Process> processes, int contextSwitchTime) {
        this.processes = new ArrayList<>(processes);
        this.executionOrder = new ArrayList<>();
        this.contextSwitchTime = contextSwitchTime;

        this.waitCounters = new HashMap<>();
        /// Initialize waiting counters
        for (Process p : processes) {
            waitCounters.put(p.getName(), 0);
        }
    }
    /// Main method that runs Priority Scheduling
    public void runPriorityScheduler() {
        int time = 0;               // Current simulation time
        int completed = 0;         // Number of completed processes
        int n = processes.size();  // Total number of processes
        Process currentProcess = null;  // Process currently running on CPU

        // Loop until all processes are completed
        while (completed < n) {
            Process highestP = null;           // Holds process with highest priority
            int currentP = Integer.MAX_VALUE; // Used for comparison

            // === AGING (Prevent Starvation) ===
            for (Process p : processes) {
                if (p.getArrivalTime() <= time && p.getRemainingTime() > 0) {

                    if (currentProcess == null || !p.getName().equals(currentProcess.getName())) {
                        /// Process is waiting → increase wait counter
                        waitCounters.put(p.getName(), waitCounters.get(p.getName()) + 1);

                        /// Every 5 waiting units → priority improves
                        if (waitCounters.get(p.getName()) >= 5) {
                            p.setPriority(Math.max(1, p.getPriority() - 1));
                            waitCounters.put(p.getName(), 0); // reset aging counter
                        }
                    } else {
                        /// Process is currently running → reset its waiting counter
                        waitCounters.put(p.getName(), 0);
                    }
                }
            }

            /// Find the process with the highest priority (smalleat priority number) among arrived processes
            for (Process p : processes) {
                if (p.getArrivalTime() <= time && p.getRemainingTime() > 0) {

                    /// If this process has a highest priority (smallest priority number), select it
                    if (p.getPriority() < currentP) {
                        currentP = p.getPriority();
                        highestP = p;
                    }

                    /// Tie-breaking: if equal priority, pick the earlier arrival
                    else if (p.getPriority() == currentP && highestP != null) {
                        if (p.getArrivalTime() < highestP.getArrivalTime()) {
                            highestP = p;
                        }
                    }
                }
            }

            /// If no process is ready, CPU remains idle
            if (highestP == null) {
                time++;     // Move time forward
                continue;  // Restart loop
            }

            /// If CPU is switching to a different process
            if (currentProcess == null || !highestP.getName().equals(currentProcess.getName())) {

                /// Record this process in execution order
                executionOrder.add(highestP.getName());

                /// Apply context switching time if not the first process
                if (currentProcess != null) {
                    // increase waiting time for all ready processes
                    for (Process p : processes) {
                        if (p != highestP && p.getArrivalTime() <= time && p.getRemainingTime() > 0) {
                            p.setWaitingTime(p.getWaitingTime() + contextSwitchTime);
                        }
                    }
                    time += contextSwitchTime;
                }

                /// Update the currently running process
                currentProcess = highestP;
            }

            /// Execute the selected process for exactly 1 time unit (Preemptive)
            highestP.setRemainingTime(highestP.getRemainingTime() - 1);
            // waiting time for all OTHER ready processes
            for (Process p : processes) {
                if (p != highestP && p.getArrivalTime() <= time && p.getRemainingTime() > 0) {
                    p.setWaitingTime(p.getWaitingTime() + 1);
                }
            }
            time++;

            /// If the process has finished execution
            if (highestP.getRemainingTime() == 0) {

                /// Set completion time
                highestP.setCompletionTime(time);

                /// Mark process as completed
                highestP.setCompleted(true);

                /// Increase count of completed processes
                completed++;
            }
        }

        /// After finishing all processes, calculate waiting times
        computeFinalWaitingTimes();

        /// Print all results
        printResults();
    }

    /// Method to compute waiting times using the formula:
    /// Waiting Time = Turnaround Time - Burst Time
    private void computeFinalWaitingTimes() {
        for (Process p : processes) {
            int waiting = p.getTurnaroundTime() - p.getBurstTime();
            p.setWaitingTime(waiting);
        }
    }

    /// this is a temporary simulation
    private void printResults() {
        double totalWT = 0;
        double totalTAT = 0;

        System.out.println("Execution Order (process starts):");
        System.out.println(executionOrder);

        System.out.println("\nProcess Results:");
        for (Process p : processes) {
            System.out.printf("Process %s | Waiting Time = %d | Turnaround Time = %d | Completion = %d\n",
                    p.getName(), p.getWaitingTime(), p.getTurnaroundTime(), p.getCompletionTime());
            totalWT += p.getWaitingTime();
            totalTAT += p.getTurnaroundTime();
        }

        System.out.printf("\nAverage Waiting Time = %.1f\n", totalWT / processes.size());
        System.out.printf("Average Turnaround Time = %.1f\n", totalTAT / processes.size());
    }
}
// need update
class TestCases2 {

    // ================= TEST CASE 1 =================
    public static void runTestCase1() {
        Scheduler scheduler = new Scheduler();

        Process p1 = new Process("P1", 0, 8 , 3);
        scheduler.addProcess(p1);

        Process p2 = new Process("P2", 1, 4 , 1);
        scheduler.addProcess(p2);

        Process p3 = new Process("P3", 2, 2 , 4);
        scheduler.addProcess(p3);

        Process p4 = new Process("P4", 3, 1 , 2);
        scheduler.addProcess(p4);

        Process p5 = new Process("P5", 4, 3 , 5);
        scheduler.addProcess(p5);

        PriorityScheduler ps = new PriorityScheduler(scheduler.getProcesses(), 1);
        ps.runPriorityScheduler();
    }


    // ================= TEST CASE 2 =================
    public static void runTestCase2() {
        Scheduler scheduler = new Scheduler();

        Process p1 = new Process("P1", 0, 6 , 3);
        scheduler.addProcess(p1);

        Process p2 = new Process("P2", 0, 3 , 1);
        scheduler.addProcess(p2);

        Process p3 = new Process("P3", 0, 8 , 2);
        scheduler.addProcess(p3);

        Process p4 = new Process("P4", 0, 4 , 4);
        scheduler.addProcess(p4);

        Process p5 = new Process("P5", 0, 2 , 5);
        scheduler.addProcess(p5);

        PriorityScheduler ps = new PriorityScheduler(scheduler.getProcesses(), 1);
        ps.runPriorityScheduler();
    }


    // ================= TEST CASE 3 =================
    public static void runTestCase3() {
        Scheduler scheduler = new Scheduler();

        Process p1 = new Process("P1", 0, 10 , 5);
        scheduler.addProcess(p1);

        Process p2 = new Process("P2", 2, 5 , 1);
        scheduler.addProcess(p2);

        Process p3 = new Process("P3", 5, 3 , 2);
        scheduler.addProcess(p3);

        Process p4 = new Process("P4", 8, 7 , 1);
        scheduler.addProcess(p4);

        Process p5 = new Process("P5", 10, 2 , 3);
        scheduler.addProcess(p5);

        PriorityScheduler ps = new PriorityScheduler(scheduler.getProcesses(), 1);
        ps.runPriorityScheduler();
    }


    // ================= TEST CASE 4 =================
    public static void runTestCase4() {
        Scheduler scheduler = new Scheduler();

        Process p1 = new Process("P1", 0, 12 , 2);
        scheduler.addProcess(p1);

        Process p2 = new Process("P2", 4, 9 , 3);
        scheduler.addProcess(p2);

        Process p3 = new Process("P3", 8, 15 , 1);
        scheduler.addProcess(p3);

        Process p4 = new Process("P4", 12, 6 , 4);
        scheduler.addProcess(p4);

        Process p5 = new Process("P5", 16, 11 , 2);
        scheduler.addProcess(p5);

        Process p6 = new Process("P6", 20, 5 , 5);
        scheduler.addProcess(p6);

        PriorityScheduler ps = new PriorityScheduler(scheduler.getProcesses(), 1);
        ps.runPriorityScheduler();
    }


    // ================= TEST CASE 5 =================
    public static void runTestCase5() {
        Scheduler scheduler = new Scheduler();

        Process p1 = new Process("P1", 0, 3 , 3);
        scheduler.addProcess(p1);

        Process p2 = new Process("P2", 1, 2 , 1);
        scheduler.addProcess(p2);

        Process p3 = new Process("P3", 2, 4 , 2);
        scheduler.addProcess(p3);

        Process p4 = new Process("P4", 3, 1 , 4);
        scheduler.addProcess(p4);

        Process p5 = new Process("P5", 4, 3 , 5);
        scheduler.addProcess(p5);

        PriorityScheduler ps = new PriorityScheduler(scheduler.getProcesses(), 1);
        ps.runPriorityScheduler();
    }


    // ================= TEST CASE 6 =================
    public static void runTestCase6() {
        Scheduler scheduler = new Scheduler();

        Process p1 = new Process("P1", 0, 14,4);
        scheduler.addProcess(p1);

        Process p2 = new Process("P2", 3, 7 , 2);
        scheduler.addProcess(p2);

        Process p3 = new Process("P3", 6, 10 , 5);
        scheduler.addProcess(p3);

        Process p4 = new Process("P4", 9, 5 , 1);
        scheduler.addProcess(p4);

        Process p5 = new Process("P5", 12, 8 , 3);
        scheduler.addProcess(p5);

        Process p6 = new Process("P6", 15, 4 , 6);
        scheduler.addProcess(p6);

        PriorityScheduler ps = new PriorityScheduler(scheduler.getProcesses(), 1);
        ps.runPriorityScheduler();
    }


    // ================= RUN ALL =================
    public static void runAll() {
        System.out.println("\n================ TEST CASE 1 ================");
        runTestCase1();

        System.out.println("\n================ TEST CASE 2 ================");
        runTestCase2();

        System.out.println("\n================ TEST CASE 3 ================");
        runTestCase3();

        System.out.println("\n================ TEST CASE 4 ================");
        runTestCase4();

        System.out.println("\n================ TEST CASE 5 ================");
        runTestCase5();

        System.out.println("\n================ TEST CASE 6 ================");
        runTestCase6();
    }
}

class TestRunner2 {
    public static void main(String[] args) {
        TestCases2.runAll();
    }
}
