#include <assert.h>
#include <error.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mpi.h"

#define MPI_CELL_DATA_TYPE MPI_INT
typedef int CellData;

int nproc;
const int NUMB_BOXES_X = 3;
const int NUMB_BOXES_Y = 3;
const int NUMB_BOXES = NUMB_BOXES_X * NUMB_BOXES_Y;
const int BOX_WIDTH = 101;
const int BOX_HEIGHT = 101;
CellData *boxData;
CellData *wholeData;
int boxID;
int xBoxPos;
int yBoxPos;

void computeSection();

/**
 * @brief Synchronize the edges of each section across the different CUDA
 * devices
 */
void sync_info() {
  // TODO: impl
  /*
    This can be synchronous with each frame. Then, it could be made potentially
    faster by synchronizing only with its neighbours So, it would like have to
    wait on its neighbour to get data and continue and its neighbour would have
    to weight on this to get data and continue. Then, ig how would you either
    avoid locking (where both are synchronous w/o a set order) or avoid one
    section going a frame ahead and then giving the wrong data to its
    neighbours.
  */
}

static void send_async_to_box(int recieverID, CellData *data, int count,
                              MPI_Request *req) {
  MPI_Isend(data, count, MPI_CELL_DATA_TYPE, recieverID, 0, MPI_COMM_WORLD,
            req);
}

static int calc_id(int x, int y) { return x + y * NUMB_BOXES_X; }

static void set_box_col(CellData *col, int colNumb) {
  for (size_t i = 0; i < BOX_HEIGHT; i++) {
    boxData[i * BOX_WIDTH + colNumb] = col[i];
  }
}
static void get_box_col(CellData *col, int colNumb) {
  for (size_t i = 0; i < BOX_HEIGHT; i++) {
    col[i] = boxData[i * BOX_WIDTH + colNumb];
  }
}

static void receive_data(int senderID, CellData *data, int count) {
  MPI_Recv(data, count, MPI_CELL_DATA_TYPE, senderID, 0, MPI_COMM_WORLD,
           MPI_STATUS_IGNORE);
}

static void box_data_from_whole(CellData *box, int proc) {
  int startY = BOX_HEIGHT * (proc / NUMB_BOXES_X);
  int startX = BOX_WIDTH * (proc % NUMB_BOXES_X);
  for (size_t y = 0; y < BOX_HEIGHT; y++) {
    for (size_t x = 0; x < BOX_WIDTH; x++) {
      int row = y + startY;
      int col = x + startX;
      box[y * BOX_WIDTH + x] =
          wholeData[(BOX_WIDTH * NUMB_BOXES_X) * row + col];
    }
  }
}

static void receive_init_data() {
  receive_data(0, boxData, BOX_HEIGHT * BOX_WIDTH);
}

static void send_init_data_to_slaves() {
  CellData boxData[BOX_WIDTH * BOX_HEIGHT];
  MPI_Request reqs[nproc - 1];
  for (int proc = 1; proc < nproc; proc++) {
    box_data_from_whole(boxData, proc);
    MPI_Isend(boxData, BOX_WIDTH * BOX_HEIGHT, MPI_CELL_DATA_TYPE, proc, 0,
              MPI_COMM_WORLD, &reqs[proc - 1]);
  }
  for (int i = 1; i < nproc; i++) {
    MPI_Wait(&reqs[i - 1], MPI_STATUS_IGNORE);
  }
}

static void sync_data() {
  if (boxID == 0) {
    CellData tmp[BOX_WIDTH * BOX_HEIGHT];
    for (size_t i = 1; i < nproc; i++) {
      receive_data(i, tmp, BOX_WIDTH * BOX_HEIGHT);
    }
  } else {
    MPI_Send(boxData, BOX_WIDTH * BOX_HEIGHT, MPI_CELL_DATA_TYPE, 0, 0, MPI_COMM_WORLD);
  }
}

// TODO: does sending over the 0th and last of every bottom/ top/ left/ right
// boundary matter?
void update_curr_boundary() {

  CellData *boxBottomBoundaryData = &boxData[(BOX_HEIGHT - 1) * BOX_WIDTH];
  CellData *boxTopBoundaryData = &boxData[(BOX_HEIGHT - 1) * BOX_WIDTH];
  CellData boxLeftBoundaryData[BOX_HEIGHT];
  CellData boxRightBoundaryData[BOX_HEIGHT];

  if (NUMB_BOXES_Y > 1) {
    if (yBoxPos == 0) {
      receive_data(calc_id(xBoxPos, yBoxPos + 1), boxBottomBoundaryData,
                   BOX_WIDTH);
    } else if (yBoxPos < NUMB_BOXES_Y - 1) {
      receive_data(calc_id(xBoxPos, yBoxPos - 1), boxTopBoundaryData,
                   BOX_WIDTH);
      receive_data(calc_id(xBoxPos, yBoxPos + 1), boxBottomBoundaryData,
                   BOX_WIDTH);
    } else {
      receive_data(calc_id(xBoxPos, yBoxPos - 1), boxTopBoundaryData,
                   BOX_WIDTH);
    }
  }
  if (NUMB_BOXES_X > 0) {
    if (xBoxPos == 0) {
      receive_data(calc_id(xBoxPos + 1, yBoxPos), boxRightBoundaryData,
                   BOX_WIDTH);
      set_box_col(boxRightBoundaryData, BOX_WIDTH - 1);
    } else if (xBoxPos < NUMB_BOXES_X - 1) {
      receive_data(calc_id(xBoxPos + 1, yBoxPos), boxRightBoundaryData,
                   BOX_WIDTH);
      receive_data(calc_id(xBoxPos - 1, yBoxPos), boxLeftBoundaryData,
                   BOX_WIDTH);
      set_box_col(boxRightBoundaryData, BOX_WIDTH - 1);
      set_box_col(boxLeftBoundaryData, 0);
    } else {
      receive_data(calc_id(xBoxPos - 1, yBoxPos), boxLeftBoundaryData,
                   BOX_WIDTH);
      set_box_col(boxLeftBoundaryData, 0);
    }
  }
}

void send_boundry_px_msg(MPI_Request *south_req, MPI_Request *north_req,
                         MPI_Request *west_req, MPI_Request *east_req) {
  CellData *boxBottomData = &boxData[(BOX_HEIGHT - 2) * BOX_WIDTH];
  CellData *boxTopData = &boxData[1];
  CellData boxLeftData[BOX_HEIGHT];
  CellData boxRightData[BOX_HEIGHT];
  get_box_col(boxLeftData, 1);
  get_box_col(boxRightData, BOX_WIDTH - 2);

  if (NUMB_BOXES_Y > 1) {
    if (yBoxPos == 0) {
      send_async_to_box(calc_id(xBoxPos, yBoxPos + 1), boxBottomData, BOX_WIDTH,
                        south_req);
    } else if (yBoxPos < NUMB_BOXES_Y - 1) {
      send_async_to_box(calc_id(xBoxPos, yBoxPos - 1), boxTopData, BOX_WIDTH,
                        north_req);
      send_async_to_box(calc_id(xBoxPos, yBoxPos + 1), boxBottomData, BOX_WIDTH,
                        south_req);
    } else {
      // Send to the noth
      send_async_to_box(calc_id(xBoxPos, yBoxPos - 1), boxTopData, BOX_WIDTH,
                        north_req);
    }
  }
  if (NUMB_BOXES_X > 0) {
    if (xBoxPos == 0) {
      send_async_to_box(calc_id(xBoxPos + 1, yBoxPos), boxRightData, BOX_WIDTH,
                        east_req);
    } else if (xBoxPos < NUMB_BOXES_X - 1) {
      send_async_to_box(calc_id(xBoxPos + 1, yBoxPos), boxRightData, BOX_WIDTH,
                        east_req);
      send_async_to_box(calc_id(xBoxPos - 1, yBoxPos), boxLeftData, BOX_WIDTH,
                        west_req);
    } else {
      send_async_to_box(calc_id(xBoxPos - 1, yBoxPos), boxLeftData, BOX_WIDTH,
                        west_req);
    }
  }
}

void wait_all_sends(MPI_Request *south_req, MPI_Request *north_req,
                    MPI_Request *west_req, MPI_Request *east_req) {
  if (NUMB_BOXES_Y > 1) {
    if (yBoxPos == 0) {
      MPI_Wait(south_req, MPI_STATUS_IGNORE);
    } else if (yBoxPos < NUMB_BOXES_Y - 1) {
      MPI_Wait(south_req, MPI_STATUS_IGNORE);
      MPI_Wait(north_req, MPI_STATUS_IGNORE);
    } else {
      MPI_Wait(north_req, MPI_STATUS_IGNORE);
    }
  }
  if (NUMB_BOXES_X > 0) {
    if (xBoxPos == 0) {
      MPI_Wait(east_req, MPI_STATUS_IGNORE);
    } else if (xBoxPos < NUMB_BOXES_X - 1) {
      MPI_Wait(east_req, MPI_STATUS_IGNORE);
      MPI_Wait(west_req, MPI_STATUS_IGNORE);
    } else {
      MPI_Wait(west_req, MPI_STATUS_IGNORE);
    }
  }
}

/**
 * @brief Run the cuda section to compute the process current section
 */
void run_cuda() { computeSection(); }

// static void error_exit(const char * f, ...) {
//   va_list args;
//   va_start(args, f);
//   vfprintf(stderr, f, args);
//   va_end(args);
//   exit(1);
// }

void run_iters(int iters) {
  MPI_Request south_req;
  MPI_Request north_req;
  MPI_Request west_req;
  MPI_Request east_req;
  for (size_t i = 0; i < iters; i++) {
    run_cuda();
    send_boundry_px_msg(&south_req, &north_req, &west_req, &east_req);
    update_curr_boundary();
    wait_all_sends(&south_req, &north_req, &west_req, &east_req);
  }
}


void init_data() { boxData = (CellData  *) calloc(BOX_HEIGHT * BOX_WIDTH, sizeof(CellData)); }

int main(int argc, char **argv) {

  init_data();
  MPI_Comm_rank(MPI_COMM_WORLD, &boxID);

  // Get total number of processes specificed at start of run
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
  assert(nproc == NUMB_BOXES);
  xBoxPos = boxID % NUMB_BOXES_X;
  yBoxPos = boxID / NUMB_BOXES_X;

  if (boxID == 0) {
    send_init_data_to_slaves();
    // Copy the top left box into the boxData
    for (int y = 0; y < NUMB_BOXES; y++)
      for (int x = 0; x < NUMB_BOXES; x++)
        boxData[y * BOX_WIDTH + x] = wholeData[y * BOX_WIDTH * NUMB_BOXES_X + x];
  } else {
    receive_init_data();
  }

  run_iters(3);

  sync_data();
  return 0;
}