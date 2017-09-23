// m - max size of a chunk
// P - number of OMP threads
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <time.h>
#include <math.h>
//#include <omp.h>
const int MAX_SIZE_OF_CHUNK = 3;

void swap(int* a, int* b) {
    int temp;
    temp = *a;
    *a = *b;
    *b = temp;
    return;
}

int my_max(int a, int b) {
    if (a > b)
        return a;
    else
        return b;
}

int binary_search(int x, int* arr, int p, int r) {
    int low = p;
    int high = my_max(p, r + 1);
    while (low < high) {
        int mid = (low + high) / 2;
        if (x <= arr[mid]) {
            high = mid;
        }
        else {
            low = mid + 1;
        }
    }
    return high;
}

int cmpfunc (const void * a, const void * b)
{
    return ( *(int*)a - *(int*)b );
}


void p_merge(int* initial, int p1, int r1, int p2, int r2, int* output, int p3) {
    int n1 = r1 - p1 + 1;
    int n2 = r2 - p2 + 1;
    if (n1 < n2) {
        swap(&p1, &p2);
        swap(&r1, &r2);
        swap(&n1, &n2);
    }
    if (n1 == 0) {
        return;
    }
    else {
        int q1 = floor((p1 + r1) / 2.0);
        int q2 = binary_search(initial[q1], initial, p2, r2);
        int q3 = p3 + (q1 - p1) + (q2 - p2);
        output[q3] = initial[q1];
        //        #pragma omp parallel
        //      }
        //          #pragma omp single nowait
        //              {
        //                  #pragma omp task
        p_merge(initial, p1, q1 - 1, p2, q2 - 1, output, p3);
        //                  #pragma omp task
        p_merge(initial, q1 + 1, r1, q2, r2, output, q3 + 1);
    }
    //      }
    return;
}

// p_merge_sort сортирует элементы в initial[p..r] и сохраняет их в output[s..s + r - p]

void p_merge_sort(int* initial, int p, int r, int* output, int s) {
    int n = r - p + 1;
    if (r - p > MAX_SIZE_OF_CHUNK) {
        int* temp =  (int *) malloc(sizeof(int) * n);
        int q = floor((p + r) / 2.0);
        int t = q - p + 1;
        //#pragma omp parallel
        {
            //#pragma omp single nowait
            {
                //#pragma omp task
                p_merge_sort(initial, p, q, temp, 0);
                
                //#pragma omp task
                p_merge_sort(initial, q + 1, r, temp, t);
                
                //_____________
                printf("%d\n", n);
                //_____________
                
                for (int i = 0; i < n; i ++) {
                    printf("%d ", temp[i]);
                }
                printf("\n");
            }
        }
        p_merge(temp, 0, t - 1, t, n - 1, output, s);
        return;
    }
    else {
        
        // _______________
        for (int i = 0 ; i < r - p + 1; i++) {
            printf("%d ", initial[p + i]);
        }
        printf("\n");
        // _________________
        
        qsort(&initial[p], r - p + 1, sizeof(int), cmpfunc);
        


        for (int i = 0; i < r - p + 1; i++) {
            output[s + i] = initial[p + i];
        }
        
        
        //__________________
        for (int i = 0 ; i < r - p + 1; i++) {
            printf("%d ", output[s + i]);
        }
        printf("\n");
        //_________________
        
        //почему с memcopy не компилится???? я не понимат
        //memcopy(output, &initial[p], r - p + 1);
        
        return;
    }
}

int main() {
    int n;
    scanf("%d", &n);
    int* array = (int *) malloc(sizeof(int) * n);
    int* result = (int *) malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        int t = rand() % 100;
        array[i] = t;
    }
    for (int i = 0; i < n; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
    p_merge_sort(array, 0, n - 1, result, 0);
    for (int i = 0; i < n; i++) {
        printf("%d ", result[i]);
    }
    return 0;
}

