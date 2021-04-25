#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <error.h>
#include <limits.h>
#include <stdarg.h>
#include "mpi.h"

typedef int8_t city_t;

int NCITIES = -1; // number of cities in the file.
int *DIST = NULL; // one dimensional array used as a matrix of size (NCITIES * NCITIES).
int NPROCS = -1;
int dfsCount = 0;
int SYNC_GRAIN = 50000;


typedef struct path_struct_t {
  int cost;         // path cost.
  city_t *path;     // order of city visits (you may start from any city).
} path_t;
path_t *bestPath = NULL;

// path_t *tempPath;
// bool *remainingCities;


// set DIST[i,j] to value
inline static void set_dist(int i, int j, int value) {
  assert(value > 0);
  int offset = i * NCITIES + j;
  DIST[offset] = value;
  return;
}

// returns value at DIST[i,j]
inline static int get_dist(int i, int j) {
  int offset = i * NCITIES + j;
  return DIST[offset];
}

// prints results
void wsp_print_result() {
  printf("========== Solution ==========\n");
  printf("Cost: %d\n", bestPath->cost);
  printf("Path: ");
  for(int i = 0; i < NCITIES; i++) {
    if(i == NCITIES-1) printf("%d", bestPath->path[i]);
    else printf("%d -> ", bestPath->path[i]);
  }
  putchar('\n');
  putchar('\n');
  return;
}

void sync_best_path(int procID, path_t * myBestPath, int bestPathIdx) {
  if (procID == bestPathIdx) {
    MPI_Bcast(myBestPath->path, NCITIES, MPI_CHAR, bestPathIdx, MPI_COMM_WORLD);
    bestPath->path = memcpy(bestPath->path,
      myBestPath->path, NCITIES*sizeof(city_t));
  } else {
    MPI_Bcast(bestPath->path, NCITIES, MPI_CHAR, bestPathIdx, MPI_COMM_WORLD);
  }
}

int get_best_path_idx(int procID, int pCount, path_t * myBestPath) {
  int tmpCost;
  int bestCostIdx = procID;
  int bestCost = myBestPath->cost;
  MPI_Barrier(MPI_COMM_WORLD);
  for (int i = 0; i < pCount; i++) {
    if (procID == i) {
      MPI_Bcast(&myBestPath->cost, 1, MPI_INT, i, MPI_COMM_WORLD);
    } else {
      MPI_Bcast(&tmpCost, 1, MPI_INT, i, MPI_COMM_WORLD);
      if (tmpCost < bestCost) {
        bestCostIdx = i;
        bestCost = tmpCost;
      }
    }
  }
  return bestCostIdx;
}

void get_best_path_cost(path_t * myBestPath) {
  int bestCost = myBestPath->cost;
  MPI_Allreduce(&bestCost, &bestPath->cost, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
}

void sync_dones(char * doneProcs) {
  for (int i = 0; i < NPROCS; i++) {
    MPI_Bcast(&doneProcs[i], 1, MPI_CHAR, i, MPI_COMM_WORLD);
  }
}

bool allDone(char * doneProcs) {
  bool allDone = true;
  for (size_t i = 0; i < NPROCS; i++) {
    if (!doneProcs[i])
      allDone = false;
  }
  return allDone;
}

// returns true when all are done
void sync(path_t * myBestPath, char * doneProcs) {
  // printf("Calling sync\n");
  // MPI_Barrier(MPI_COMM_WORLD);
  get_best_path_cost(myBestPath);
  sync_dones(doneProcs);
  // MPI_Barrier(MPI_COMM_WORLD);
}

void pathDFS(city_t len, bool *visited, path_t *currPath, path_t *myBestPath, char * doneProcs) {
  ++dfsCount;
  if (dfsCount == SYNC_GRAIN) {
    // printf("Syncing from here \n");
    sync(myBestPath, doneProcs);
    dfsCount = 0;
  }
  if (len == NCITIES){
    if (currPath->cost < myBestPath->cost){
      myBestPath->cost = currPath->cost;
      memcpy(myBestPath->path, currPath->path, NCITIES*sizeof(city_t));
    }
    return;
  }

  city_t curr = currPath->path[len-1];
  for (city_t next=0; next<NCITIES; next++){
    if (!visited[next]){
      int newCost = currPath->cost + get_dist(curr, next);
      if (newCost >= myBestPath->cost) continue;
      else if (newCost >= bestPath->cost) continue;
      currPath->cost = newCost;
      currPath->path[len] = next;
      visited[next]=true;
      pathDFS(len+1, visited, currPath, myBestPath, doneProcs);
      visited[next]=false;
      currPath->cost -= get_dist(curr, next);
    }
  }
}

void compute_wsp(int procID, int pCount) {
  const int root = 0;

  //Total number of start paths of length 2
  int totalPaths = NCITIES * (NCITIES-1);

  //We partition by the first two cities,
  //which is more granular
  int delta = (totalPaths+pCount-1)/pCount;
  int pstart = procID * delta;
  int pend = (procID+1) * delta;
  if (pend > totalPaths) pend = totalPaths;

  char doneProcs[NPROCS];
  for (size_t i = 0; i < NPROCS; i++) {
    doneProcs[i] = 0;
  }

  //Each thread has bestPath, currPath, visited
  path_t *myBestPath = (path_t *)malloc(sizeof(path_t));
  myBestPath->path = (city_t *)malloc(sizeof(city_t) * NCITIES);
  bestPath->cost = INT_MAX;
  myBestPath->cost = INT_MAX;
  path_t *currPath = (path_t *)malloc(sizeof(path_t));
  currPath->path = (city_t *)malloc(sizeof(city_t) * NCITIES);
  bool *visited = (bool *)malloc(sizeof(bool) * NCITIES);

  //p is index of first two cities
  for (int p = pstart; p < pend; p++) {
    //Calculating city 1 and 2 from p
    city_t c1=p/(NCITIES-1);
    city_t c2=p%(NCITIES-1);
    if (c2>=c1) c2++;
    for (city_t v=0; v<NCITIES; v++){
      visited[v]=false;
    }
    visited[c1]=true;
    visited[c2]=true;
    currPath->cost=get_dist(c1,c2);
    currPath->path[0]=c1;
    currPath->path[1]=c2;
    pathDFS(2, visited, currPath, myBestPath, doneProcs);
  }
  doneProcs[procID] = 1;
  do {
    sync(myBestPath, doneProcs);
  } while (!allDone(doneProcs));
  // printf("All done\n");
  get_best_path_cost(myBestPath);
  int bestPathIdx = get_best_path_idx(procID, pCount, myBestPath);
  sync_best_path(procID, myBestPath, bestPathIdx);

  if (procID == root) {
    // Wait async with barrier after for all other procs to finish
    // memcpy(bestPath->path, myBestPath->path, NCITIES * sizeof(city_t));
  } else {
    // SendMsg to the root
  }
  free(currPath->path);
  free(currPath);
  free(myBestPath->path);
  free(myBestPath);
  free(visited);
}

void wsp_start(int procID, int nproc) {
  // Run computation
  // int startTime = MPI_Wtime();
  NPROCS = nproc;
  compute_wsp(procID, nproc);
  // int endTime = MPI_Wtime();

  // Compute running time
  MPI_Finalize();
}

static void error_exit(const char * f, ...) {
  va_list args;
  va_start(args, f);
  vfprintf(stderr, f, args);
  va_end(args);
  exit(1);
}

int main(int argc, char **argv) {
  if(argc < 2) {
    error_exit("Expecting one arguments: [file name]\n");
  }
  char *filename = argv[1];
  FILE *fp = fopen(filename, "r");
  if(fp == NULL) {
    error_exit("Expecting two arguments: -p [processor count] and [file name]\n");
  }
  int scan_ret;
  scan_ret = fscanf(fp, "%d", &NCITIES);
  if(scan_ret != 1) error_exit("Failed to read city count\n");
  if(NCITIES < 2) {
    error_exit("Illegal city count: %d\n", NCITIES);
  } 
  // Allocate memory and read the matrix
  DIST = (int*)calloc(NCITIES * NCITIES, sizeof(int));
  assert(DIST != NULL);
  for(int i = 1;i < NCITIES;i++) {
    for(int j = 0;j < i;j++) {
      int t;
      scan_ret = fscanf(fp, "%d", &t);
      if(scan_ret != 1) error_exit("Failed to read dist(%d, %d)\n", i, j);
      set_dist(i, j, t);
      set_dist(j, i, t);
    }
  }
  fclose(fp);
  bestPath = (path_t*)malloc(sizeof(path_t));
  bestPath->cost = 0;
  bestPath->path = (city_t*)calloc(NCITIES, sizeof(city_t));
  struct timespec before, after;
  clock_gettime(CLOCK_REALTIME, &before);
  int procID, nproc;
  // Initialize MPI
  MPI_Init(NULL, NULL);

  // Get process rank
  MPI_Comm_rank(MPI_COMM_WORLD, &procID);

  // Get total number of processes specificed at start of run
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
  wsp_start(procID, nproc);
  clock_gettime(CLOCK_REALTIME, &after);
  double delta_ms = (double)(after.tv_sec - before.tv_sec) * 1000.0 + (after.tv_nsec - before.tv_nsec) / 1000000.0;
  putchar('\n');
  if (procID == 0) {
    printf("============ Time ============\n");
    printf("Time: %.3f ms (%.3f s)\n", delta_ms, delta_ms / 1000.0);
    wsp_print_result();
  }
  return 0;
}