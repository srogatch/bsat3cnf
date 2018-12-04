#include "stdafx.h"
#include "RawClause.h"
#include "Problem.h"
#include "Solver2Sat.h"
#include "Pipeline.h"
using namespace std;

const char* const gcInpFn = "input.3cnf";
const char* const gcOutFn = "output.txt";
char gBuf[1 << 10];

int64_t gnUsedVars = -1;
Problem gInitial;
Pipeline<Problem> problems;
mutex gmSolution;

void CheckAndPrintSolution(const Problem& cur) {
  //// Check
  int64_t failureClause = -1;
  for (int64_t i = 0; i < gInitial._cl3.size(); i++) {
    bool satisfied = false;
    for (int8_t j = 0; j < 3; j++) {
      const int64_t signedVar = gInitial._cl3[i]._vars[j];
      if (signedVar == 0) {
        break;
      }
      const int64_t absVar = abs(signedVar);
      if (cur._varVal[absVar] == Problem::SignToBool(signedVar)) {
        satisfied = true;
        break;
      }
    }
    if (!satisfied) {
      failureClause = i;
      break;
    }
  }

  //// Print
  unique_lock<mutex> msl(gmSolution);
  FILE *fpout = fopen(gcOutFn, "wt");
  for (int64_t i = 1; i < cur._varVal.size(); i++) {
    fprintf(fpout, "%d ", cur._varVal[i] ? 1 : 0);
  }
  fprintf(fpout, "\n");
  if (failureClause >= 0) {
    fprintf(fpout, "Check failed at %lld!!!!!\n", failureClause);
  }
  fclose(fpout);
  quick_exit(0);
}

void Worker() {
  Problem cur;
  while (problems.Pop(cur)) {
    if (cur._nKnown == gnUsedVars) { // Solution found
      CheckAndPrintSolution(cur);
      continue; // should be unreachable
    }
    if (cur._cl3.size() == 0) { // reduced to 2-sat problem
      Solver2Sat s2s(cur);
      if (!s2s.Solve(cur)) {
        continue;
      }
      CheckAndPrintSolution(cur);
      continue; // should be unreachable
    }
    Problem bestLeft, bestRight;
    bool maybeBestLeft = false, maybeBestRight = false;
    int64_t bestTotCl3 = (cur._cl3.size() + 1) * 2;
    for (int64_t i = 0; i < cur._cl3.size(); i++) {
      for (int8_t j = 0; j < 3; j++) {
        int64_t totCl3 = 0;
        bool maybeLeft = false;
        bool maybeRight = false;
        Problem left = cur;
        left._cl2.emplace_back();
        int8_t at = 0;
        for (int8_t k = 0; k < 3; k++) {
          if (k == j) continue;
          left._cl2.back()._vars[at] = cur._cl3[i]._vars[k];
          at++;
        }
        left.RemoveClause3(i);
        if (left.EliminateSingleSigned()) {
          totCl3 += left._cl3.size();
          maybeLeft = true;
        }

        Problem right = cur;
        right.RemoveClause3(i);
        if (right.ApplyVar(cur._cl3[i]._vars[j]) && right.EliminateSingleSigned()) {
          totCl3 += right._cl3.size();
          maybeRight = true;
        }
        if ((maybeLeft || maybeRight) && totCl3 < bestTotCl3) {
          maybeBestLeft = maybeLeft;
          if (maybeLeft) {
            bestLeft = left;
          }
          maybeBestRight = maybeRight;
          if (maybeRight) {
            bestRight = right;
          }
          bestTotCl3 = totCl3;
        }
      }
    }
    if (bestTotCl3 >= (cur._cl3.size() + 1) * 2) { // Unsatisfiable
      continue;
    }
    if (maybeBestLeft) {
      problems.Push(bestLeft);
    }
    if (maybeBestRight) {
      problems.Push(bestRight);
    }
  }
}

int main()
{
  int64_t nVars = -1, nClauses = -1;
  vector<Clause3> clauses;
  {
    vector<bool> usedVar;
    ifstream ifs(gcInpFn, ifstream::in);
    string line;
    bool bProbDef = false;
    vector<int64_t> curClause;
    while (getline(ifs, line)) {
      if (sscanf(line.c_str(), "%s", gBuf) < 1) {
        continue; // empty line
      }
      if (!_stricmp(gBuf, "c")) {
        continue; // a comment line
      }
      if (!_stricmp(gBuf, "p")) {
        if (bProbDef) {
          fprintf(stderr, "Duplicate problem definition.\n");
          return 1;
        }
        if (sscanf(line.c_str(), "%*s%s%lld%lld", gBuf, &nVars, &nClauses) != 3) {
          fprintf(stderr, "Error in problem definition.\n");
          return 2;
        }
        bProbDef = true;
        usedVar.resize(nVars + 1, false);
        continue;
      }
      if (!bProbDef) {
        fprintf(stderr, "A clause seems to appear before a problem definition.\n");
        return 3;
      }
      int64_t pos = 0;
      for (;;) {
        int64_t offs;
        int64_t var;
        if (sscanf(line.c_str() + pos, "%lld%lln", &var, &offs) != 1) {
          break;
        }
        if (var == 0) {
          if (curClause.size() > 3) {
            fprintf(stderr, "Too many variables in a clause: %lld", int64_t(curClause.size()));
            return 4;
          }
          clauses.emplace_back();
          for (int8_t i = 0; i < int8_t(curClause.size()); i++) {
            clauses.back()._vars[i] = curClause[i];
            usedVar[abs(curClause[i])] = true;
          }
          for (int8_t i = int8_t(curClause.size()); i < 3; i++) {
            clauses.back()._vars[i] = 0;
          }
          curClause.clear();
          break;
        }
        curClause.emplace_back(var);
        pos += offs;
      }
    }
    if (clauses.size() != nClauses) {
      fprintf(stderr, "Read %lld clauses instead of %lld\n", (int64_t)clauses.size(), (int64_t)nClauses);
      return 5;
    }
    gnUsedVars = 0;
    for (int64_t i = 1; i < nVars; i++) {
      if (usedVar[i]) {
        gnUsedVars++;
      }
    }
  }

  gInitial._varKnown.resize(nVars + 1, false);
  gInitial._varVal.resize(nVars + 1);
  gInitial._cl3 = clauses;
  gInitial._cl2.clear();
  gInitial._nKnown = 0;

  Problem normalized = gInitial;
  if (!normalized.NormalizeInput()) {
    FILE *fpout = fopen(gcOutFn, "wt");
    fprintf(fpout, "Unsatisfiable\n");
    fclose(fpout);
    return 0;
  }
  
  const int64_t nWorkers = thread::hardware_concurrency();
  problems.SetWorkerCount(nWorkers);
  problems.Push(normalized);
  vector<thread> workers;
  for (int64_t i = 0; i < nWorkers; i++) {
    workers.emplace_back(&Worker);
  }
  for (int64_t i = 0; i < nWorkers; i++) {
    workers[i].join();
  }

  FILE *fpout = fopen(gcOutFn, "wt");
  fprintf(fpout, "Unsatisfiable\n");
  fclose(fpout);
  return 0;
}

