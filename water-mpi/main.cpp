#include <assert.h>
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

#define MPI_CELL_DATA_TYPE MPI_INT
typedef int CellData;

int nproc;
const int NUMB_BOXES_X = 3;
const int NUMB_BOXES_Y = 3;
const int NUMB_BOXES = NUMB_BOXES_X * NUMB_BOXES_Y;
const int BOX_WIDTH = 100;
const int BOX_HEIGHT = 100;
CellData * boxData;
int boxID;
int xBoxPos;
int yBoxPos;



void computeSection();

/**
* @brief Synchronize the edges of each section across the different CUDA devices
*/
void sync_info() {
  // TODO: impl
  /*
    This can be synchronous with each frame. Then, it could be made potentially faster by synchronizing only with its neighbours
    So, it would like have to wait on its neighbour to get data and continue and its neighbour would have to weight on this to get data and continue. Then, ig how would 
    you either avoid locking (where both are synchronous w/o a set order) or avoid one section going a frame ahead and then giving the wrong data to its neighbours.
  */
}

static void send_async_to_box(int recieverID, CellData * data, int count, MPI_Request *req) {
  MPI_Isend(data, count, MPI_CELL_DATA_TYPE, recieverID, 0, MPI_COMM_WORLD, req);
}

static int calc_id(int x, int y) {
  return x + y * NUMB_BOXES_X;
}

static void get_box_col(CellData * col, int colNumb) {
  for (size_t i = 0; i < BOX_HEIGHT; i++) {
    col[i] = boxData[i * BOX_WIDTH + colNumb];
  }
}

void send_boundry_px_msg() {
  MPI_Request south_req;
  MPI_Request north_req;
  MPI_Request west_req;
  MPI_Request east_req;

  CellData * boxBottomData = &boxData[(BOX_HEIGHT - 1) * BOX_WIDTH];
  CellData * boxTopData = &boxData[0];
  CellData boxLeftData[BOX_HEIGHT];
  CellData boxRightData[BOX_HEIGHT];
  get_box_col(boxLeftData, 0);
  get_box_col(boxRightData, BOX_WIDTH - 1);


  if (yBoxPos == 0 && NUMB_BOXES_Y > 1) {
    send_async_to_box(calc_id(xBoxPos, yBoxPos + 1), boxBottomData, BOX_WIDTH, &south_req);
  } else if (yBoxPos < NUMB_BOXES_Y - 1) {
    send_async_to_box(calc_id(xBoxPos, yBoxPos - 1), boxTopData, BOX_WIDTH, &north_req);
    send_async_to_box(calc_id(xBoxPos, yBoxPos + 1), boxBottomData, BOX_WIDTH, &south_req);
  } else {
    // Send to the noth
    send_async_to_box(calc_id(xBoxPos, yBoxPos - 1), boxTopData, BOX_WIDTH, &north_req);
  }
  if (xBoxPos == 0) {
    send_async_to_box(calc_id(xBoxPos + 1, yBoxPos), boxRightData, BOX_WIDTH, &east_req);
  } else if (xBoxPos < NUMB_BOXES_X - 1) {
    send_async_to_box(calc_id(xBoxPos + 1, yBoxPos), boxRightData, BOX_WIDTH, &east_req);
    send_async_to_box(calc_id(xBoxPos - 1, yBoxPos), boxLeftData, BOX_WIDTH, &west_req);
  } else {
    send_async_to_box(calc_id(xBoxPos - 1, yBoxPos), boxLeftData, BOX_WIDTH, &west_req);
  }
}

/**
* @brief Run the cuda section to compute the process current section
*/
void run_cuda() {
  computeSection();
}


// static void error_exit(const char * f, ...) {
//   va_list args;
//   va_start(args, f);
//   vfprintf(stderr, f, args);
//   va_end(args);
//   exit(1);
// }

int main(int argc, char **argv) {
  MPI_Comm_rank(MPI_COMM_WORLD, &boxID);

  // Get total number of processes specificed at start of run
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
  assert(nproc == NUMB_BOXES);
  xBoxPos = boxID % NUMB_BOXES_X;
  yBoxPos = boxID / NUMB_BOXES_X;
  return 0;
}