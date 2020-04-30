#include <iostream>
#include <bits/stdc++.h>
#include <cstdlib>
#include <pthread.h>
#include <time.h>

using namespace std;

struct return_value
{
  int *result;
  int elementValue;
  int row_index;
  int col_index;
};

struct thread_data
{
  int **matrixA;
  int **matrixB;
  int rowIndex;
  int colIndex;
  int numOfColsA;
  int numOfColsB;
  bool useColIndex;
};

void *calculateElement(void *threadid);
void createThreads(int numOfThreads, int numOfRowsA, int numOfColumnsA, int numOfRowsB, int numOfColumnsB, int **matrixA, int **matrixB);

int main()
{
  // open the file for reading
  FILE *file = fopen("input.txt", "r");
  int numOfRowsA, numOfColumnsA;
  int numOfRowsB, numOfColumnsB;

  // read dimensions of matrix A
  fscanf(file, "%d", &numOfRowsA);
  fscanf(file, "%d", &numOfColumnsA);

  // allocate memory for matrix A
  int **matrixA = (int **)malloc(numOfRowsA * sizeof(int *));
  for (int i = 0; i < numOfRowsA; i++)
    matrixA[i] = (int *)malloc(numOfColumnsA * sizeof(int));

  // set elements of matrix A by reading them from file
  for (int i = 0; i < numOfRowsA; i++)
    for (int j = 0; j < numOfColumnsA; j++)
      fscanf(file, "%d", &matrixA[i][j]);

  // read dimensions of matrix B
  fscanf(file, "%d", &numOfRowsB);
  fscanf(file, "%d", &numOfColumnsB);

  // allocate memory for matrix B
  int **matrixB = (int **)malloc(numOfRowsB * sizeof(int *));
  for (int i = 0; i < numOfRowsB; i++)
    matrixB[i] = (int *)malloc(numOfColumnsB * sizeof(int));

  // set elements of matrix B by reading them from file
  for (int i = 0; i < numOfRowsB; i++)
    for (int j = 0; j < numOfColumnsB; j++)
      fscanf(file, "%d ", &matrixB[i][j]);

  fclose(file);

  // perform matrix multiplication element by element
  createThreads(numOfRowsA * numOfColumnsB, numOfRowsA, numOfColumnsA, numOfRowsB, numOfColumnsB, &*matrixA, &*matrixB);

  // perform matrix multiplication row by row
  createThreads(numOfRowsA, numOfRowsA, numOfColumnsA, numOfRowsB, numOfColumnsB, &*matrixA, &*matrixB);

  // free the memory that was allocated to both matrices
  for (int i = 0; i < numOfRowsA; i++)
    free(matrixA[i]);
  free(matrixA);

  for (int i = 0; i < numOfRowsB; i++)
    free(matrixB[i]);
  free(matrixB);

  return 0;
}

// this function creates n threads (depending on type of multithreading)
void createThreads(int numOfThreads, int numOfRowsA, int numOfColumnsA, int numOfRowsB, int numOfColumnsB, int **matrixA, int **matrixB)
{
  // get starting system time
  clock_t time = clock();

  // boolean value determines whether we're working element by element or row by row
  bool elementByElement = numOfThreads != numOfRowsA;

  // create n threads
  pthread_t threads[numOfThreads];

  //array of n structs (one for each thread)
  struct thread_data td[numOfThreads];
  int threadCount = 0;

  // variable that dictates the upper limit of the inner for loop
  int limit;
  if (elementByElement)
    limit = numOfColumnsB;
  else
    limit = 1;

  for (int i = 0; i < numOfRowsA; i++)
  {
    for (int j = 0; j < limit; j++)
    {
      // populate the struct
      td[threadCount].matrixA = matrixA;
      td[threadCount].matrixB = matrixB;
      td[threadCount].rowIndex = i;
      td[threadCount].colIndex = j;
      td[threadCount].numOfColsA = numOfColumnsA;
      td[threadCount].numOfColsB = numOfColumnsB;
      td[threadCount].useColIndex = elementByElement;

      // create a thread
      if (pthread_create(&threads[threadCount], NULL, calculateElement, (void *)(&td[threadCount])))
      {
        printf("Unable to create a thread.");
        exit(-1);
      }

      threadCount++;
    }
  }

  // allocate memory for a struct that holds the return value of a thread
  struct return_value *ret_val = (struct return_value *)malloc(sizeof(struct return_value));
  void *status;
  int resultMatrix[numOfRowsA][numOfColumnsB];

  // receive the return values of each thread and set the result value(s) in resultMatrix
  for (int i = 0; i < numOfThreads; i++)
  {
    pthread_join(threads[i], &status);
    ret_val = (struct return_value *)status;

    if (elementByElement)
      resultMatrix[ret_val->row_index][ret_val->col_index] = ret_val->result[0];
    else
      for (int j = 0; j < numOfColumnsB; j++)
        resultMatrix[ret_val->row_index][j] = ret_val->result[j];
  }

  // get elapsed time in seconds
  time = clock() - time;
  double elapsed_time = ((double)time) / CLOCKS_PER_SEC;

  // open output file in writing mode for the first process, append mode for the second process
  FILE *file = fopen("output.txt", elementByElement ? "w" : "a");

  // write the resultMatrix values into the output file
  for (int i = 0; i < numOfRowsA; i++)
  {
    for (int j = 0; j < numOfColumnsB; j++)
      fprintf(file, "%d ", resultMatrix[i][j]);
    fprintf(file, "\n");
  }
  // write the elapsed time value into the output file
  fprintf(file, "END%d    [%f]", elementByElement == true ? 1 : 2, elapsed_time);
  fprintf(file, "\n");
  fclose(file);

  // free allocated memory
  free(ret_val);
}

// this function performs the actual matrix multiplication and returns a result
void *calculateElement(void *thread_arg)
{
  struct thread_data *current_data = (struct thread_data *)thread_arg;
  struct return_value *ret_val = (struct return_value *)malloc(sizeof(struct return_value));

  int sum = 0;
  int **matrixA = current_data->matrixA;
  int **matrixB = current_data->matrixB;
  int rowIndex = current_data->rowIndex;
  int colIndex = current_data->colIndex;
  int numOfColsA = current_data->numOfColsA;
  int numOfColsB = current_data->numOfColsB;
  bool useColIndex = current_data->useColIndex;

  int row[numOfColsA];
  int col[numOfColsA];
  int result[numOfColsB];

  // if element by element
  if (useColIndex)
  {
    // get sum of current row from matrix A * current col of matrix B
    for (int i = 0; i < numOfColsA; i++)
    {
      row[i] = matrixA[rowIndex][i];
      col[i] = matrixB[i][colIndex];
      result[0] += row[i] * col[i];
    }
  }
  else
  {
    // get sum of current row from matrix A * each column of matrix B
    for (int i = 0; i < numOfColsB; i++)
    {
      result[i] = 0;
      for (int j = 0; j < numOfColsA; j++)
      {
        row[j] = matrixA[rowIndex][j];
        col[j] = matrixB[j][i];
        result[i] += row[j] * col[j];
      }
    }
  }

  // store results in ret_val struct and return it 
  ret_val->result = result;
  ret_val->row_index = rowIndex;
  ret_val->col_index = colIndex;

  pthread_exit((void *)ret_val);
  free(ret_val);
}
