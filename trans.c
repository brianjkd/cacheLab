/*
 *  bjkeeleydebonis@wpi.edu
 *  tdha@wpi.edu
*/

/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int blockSize; // changes based on input matrix
    int blockColIndex, blockRowIndex;
    int row, col;
    int blockRowEnd, blockColEnd;
    int temp, diagonalIndex;

    /*
        Change block size based on input matrix.
    */
    if (M == 64 && N == 64){
        blockSize = 4;
    }
    else if (M == 32 && N == 32){
        blockSize = 8;
    }
    else { // odd-sized matrix.
        blockSize = 16;
    }
    /*
        This transpose algorithm does not work well for 64 x 64 matrix. Oh well.
        Simply use blocking. Outer 2 loops partition matrix into blocks, inner
        2 loops navigate block and perform the tranpose.
    */
    for (blockColIndex = 0; blockColIndex < M; blockColIndex += blockSize){
        for (blockRowIndex = 0; blockRowIndex < N; blockRowIndex += blockSize){
            blockRowEnd = blockRowIndex + blockSize;
            // handle case where (row >= N) in odd-sized matrix
            for (row = blockRowIndex; (row < blockRowEnd) && (row < N); row++){
               blockColEnd = blockColIndex + blockSize;
               // handle case where (col >= M) in odd-sized matrix
               for (col = blockColIndex; (col < blockColEnd) && (col < M); col++){
                    if(row != col)
                    {
                        B[col][row] = A[row][col];
                    }
                    else 
                    {
                        temp = A[row][col]; // cache it
                        diagonalIndex = row;
                    }
               }
               if (blockColIndex == blockRowIndex)  
               {
                    B[diagonalIndex][diagonalIndex] = temp;  // do not need to move diagonal
               }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

