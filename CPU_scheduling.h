#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <queue>
#include <iomanip>

struct Queue {
    std::string QID;
    int timeSlice;
    std:: string schedulingPolicy;
};

struct Process {
    std :: string PID;
    int arrivalTime, burstTime, turnaroundTime, waitingTime, remainingTime, completionTime;
    std:: string QueueID;
};

struct Gantt{
    int start;
    int end;
    std::string queueID;
    std::string PID;
};


void readFile(std::string filename, std::vector<Queue>& queue, std::vector<Process>& processes);
void SJF (std::vector<Process>& processes, std::string QueueID, int& currentTime, int timeSlice, std::vector<Gantt>& ganttElement);
void SRTN(std::vector<Process>& processes, std::string QueueID, int& currentTime, int timeSlice, std::vector<Gantt>& ganttElement);
void runQueue(std::vector<Process>& processes, std::vector<Queue>& queues, std::vector<Gantt>& ganttElement);

