#include "stdafx.h"
#include "RawClause.h"
#include "Problem.h"
#include "Solver2Sat.h"
#include "Pipeline.h"
#include "ShadowProblem.h"
using namespace std;

const char* const gcInpFn = "input.3cnf";
const char* const gcOutFn = "output.txt";
char gBuf[1 << 10];

int64_t gnUsedVars = -1;
Problem gInitial;
Pipeline<Problem> problems;
mutex gmSolution;
const bool gbSelfCheck = true;

void CheckAndPrintSolution(const Problem& cur) {
  //// Check
  int64_t failureClause = -1;
  for (int64_t i = 0; i < int64_t(gInitial._cl3.size()); i++) {
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
  for (int64_t i = 1; i < int64_t(cur._varVal.size()); i++) {
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
    if (gbSelfCheck) {
      for (int64_t i = 0; i < int64_t(cur._cl3.size()); i++) {
        for (int8_t j = 0; j < 3; j++) {
          const int64_t var = cur._cl3[i]._vars[j];
          if(!cur._vr3.Contains(var, i, cur)) {
            fprintf(stderr, "Checking failed for variable %lld in 3-clause %lld.\n", var, i);
          }
        }
      }
      for (int64_t i = 0; i < int64_t(cur._cl2.size()); i++) {
        for (int8_t j = 0; j < 2; j++) {
          const int64_t var = cur._cl2[i]._vars[j];
          if (!cur._vr2.Contains(var, i, cur)) {
            fprintf(stderr, "Checking failed for variable %lld in 2-clause %lld.\n", var, i);
          }
        }
      }
    }

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

    Problem left = cur;
    Problem right = cur;
    ShadowProblem shadowLeft(cur, left);
    ShadowProblem shadowRight(cur, right);

    Problem bestLeft, bestRight;
    bool maybeBestLeft = false, maybeBestRight = false;
    int64_t bestTotCl3 = (cur._cl3.size() + 1) * 2;
    for (int64_t i = 0; i < int64_t(cur._cl3.size()); i++) {
      for (int8_t j = 0; j < 3; j++) {
        int64_t totCl3 = 0;
        bool maybeLeft = false;
        bool maybeRight = false;
        
        shadowLeft.Restore();

        //// DEBUG-PRINT
        //for (int64_t m = 0; m < cur._vrc._avlNp._nodes.size(); m++) {
        //  if (memcmp(&cur._vrc._avlNp._nodes[m], &left._vrc._avlNp._nodes[m], sizeof(AVLNode))) {
        //    printf(" %lld ", m);
        //  }
        //}

        left._cl2.emplace_back();
        int8_t at = 0;
        Clause2 &cl2back = left._cl2.ModifyBack(&shadowLeft._cl2);
        for (int8_t k = 0; k < 3; k++) {
          if (k == j) continue;
          const int64_t var = cur._cl3[i]._vars[k];
          cl2back._vars[at] = var;
          left._vr2.Add(var, left._cl2.size() - 1, left);
          at++;
        }
        left.RemoveClause3(i);
        if (left.ActSingleSigned(cur._cl3[i]._vars[j])) {
          totCl3 += left._cl3.size();
          maybeLeft = true;
        }

        shadowRight.Restore();
        right.RemoveClause3(i);
        if (right.ApplyVar(cur._cl3[i]._vars[j])) {
          bool maybeSat = true;
          for (int8_t k = 0; k < 3; k++) {
            if (k == j) continue;
            if (!right.ActSingleSigned(cur._cl3[i]._vars[k])) {
              maybeSat = false;
              break;
            }
          }
          if(maybeSat) {
            totCl3 += right._cl3.size();
            maybeRight = true;
          }
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
    if (bestTotCl3 >= (int64_t(cur._cl3.size()) + 1) * 2) { // Unsatisfiable
      continue;
    }
    if (maybeBestLeft) {
      bestLeft._pShadow = nullptr;
      problems.Push(bestLeft);
    }
    if (maybeBestRight) {
      bestRight._pShadow = nullptr;
      problems.Push(bestRight);
    }
  }
}

int main()
{
  SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

  int64_t nVars = -1, nClauses = -1;
  FastVector<Clause3> clauses;
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
            clauses.ModifyBack(nullptr)._vars[i] = curClause[i];
            usedVar[abs(curClause[i])] = true;
          }
          for (int8_t i = int8_t(curClause.size()); i < 3; i++) {
            clauses.ModifyBack(nullptr)._vars[i] = 0;
          }
          curClause.clear();
          break;
        }
        if (abs(var) > nVars) {
          fprintf(stderr, "Variable out of range: %lld\n", var);
          quick_exit(9);
        }
        curClause.emplace_back(var);
        pos += offs;
      }
    }
    if (int64_t(clauses.size()) != nClauses) {
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
  gInitial._nKnown = 0;
  gInitial._vrc.Init(nVars);
  gInitial._vr3.Init(gInitial);
  for (int64_t i = 0; i < int64_t(gInitial._cl3.size()); i++) {
    for (int8_t j = 0; j < 3; j++) {
      const int64_t var = gInitial._cl3[i]._vars[j];
      if (var == 0) {
        break;
      }
      //printf("%lld %d %lld\n", i, (int)j, var); // DEBUG-PRINT
      //if (i == 533) {
      //  printf("");
      //}
      gInitial._vr3.Add(var, i, gInitial);
      if (!gInitial._vr3.Contains(var, i, gInitial)) {
        fprintf(stderr, "Failed to mark variable %lld in clause %lld\n", var, i);
      }
    }
  }
  gInitial._vr2.Init(gInitial);

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

