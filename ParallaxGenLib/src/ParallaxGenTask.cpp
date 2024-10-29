#include "ParallaxGenTask.hpp"

#include <spdlog/spdlog.h>

using namespace std;

ParallaxGenTask::ParallaxGenTask(string TaskName, const size_t &TotalJobs, const int &ProgressPrintModulo)
    : ProgressPrintModulo(ProgressPrintModulo), TaskName(std::move(TaskName)), TotalJobs(TotalJobs) {
  initJobStatus();
}

void ParallaxGenTask::completeJob(const PGResult &Result) {
  // Use lock_guard to make this method thread-safe
  const lock_guard<mutex> Lock(NumJobsCompletedMutex);

  // Check if NumJobsCompleted has Result for key
  if (NumJobsCompleted.find(Result) == NumJobsCompleted.end()) {
    NumJobsCompleted[Result] = 0;
  }

  NumJobsCompleted[Result]++;
  printJobStatus();
}

void ParallaxGenTask::initJobStatus() {
  LastPerc = 0;

  spdlog::info("{} Starting...", TaskName);
}

void ParallaxGenTask::printJobStatus(bool Force) {
  size_t CombinedJobs = getCompletedJobs();
  size_t Perc = CombinedJobs * FULL_PERCENTAGE / TotalJobs;
  if (Force || Perc != LastPerc) {
    LastPerc = Perc;

    if (Perc % ProgressPrintModulo == 0) {
      spdlog::info("{} Progress: {}/{} [{}%]", TaskName, CombinedJobs, TotalJobs, Perc);
    }
  }

  if (Perc == FULL_PERCENTAGE) {
    printJobSummary();
  }
}

void ParallaxGenTask::printJobSummary() {
  // Print each job status Result
  string OutputLog = TaskName + " Summary: ";
  for (const auto &Pair : NumJobsCompleted) {
    const string StateStr = PGResultStr[Pair.first];
    OutputLog += "[ " + StateStr + " : " + to_string(Pair.second) + " ] ";
  }
  OutputLog += "See log to see error messages, if any.";
  spdlog::info(OutputLog);
}

auto ParallaxGenTask::getCompletedJobs() -> size_t {
  // Initialize the Sum variable
  size_t Sum = 0;

  // Iterate through the unordered_map and sum the values
  for (const auto &Pair : NumJobsCompleted) {
    Sum += Pair.second;
  }

  return Sum;
}

auto ParallaxGenTask::isCompleted() -> bool {
  const lock_guard<mutex> Lock(NumJobsCompletedMutex);

  return getCompletedJobs() == TotalJobs;
}

void ParallaxGenTask::updatePGResult(PGResult &Result, const PGResult &CurrentResult, const PGResult &Threshold) {
  if (CurrentResult > Result) {
    if (CurrentResult > Threshold) {
      Result = Threshold;
    } else {
      Result = CurrentResult;
    }
  }
}
