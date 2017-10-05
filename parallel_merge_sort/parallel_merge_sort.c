#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <omp.h>

int MAX_SIZE_OF_CHUNK = 10;

void swap(int* a, int* b) {
    int temp;
    temp = *a;
    *a = *b;
    *b = temp;
    return;
}

int my_max(int a, int b) {
    if (a > b) {
        return a;
    }
    else {
        return b;
    }
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

int cmpfunc (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}

// Предполагается, что два сливаемых подмассива находятся в одном и том же массиве, однако не предполагается, что они граничат друг с другом. 
// Сливает два отсортированных подмассива initial[р1..r1] и initial[p2..r2] в подмассив output[p3..r3].
void p_merge(int* initial, int p1, int r1, int p2, int r2, int* output, int p3) {
    int n1 = r1 - p1 + 1;
    int n2 = r2 - p2 + 1;
    if (n1 < n2) {  // Гарантируем, что n1 >= n2
        swap(&p1, &p2);
        swap(&r1, &r2);
        swap(&n1, &n2);
    }
    if (n1 == 0) { // Если оба пусты
        return;
    }
    else { 
        int q1 = (p1 + r1) / 2;
        int q2 = binary_search(initial[q1], initial, p2, r2); // Находим такую точку q2 в подмассиве initial[p2..r2], такую, что все эелементы в initial[p2..q2-1] меньше initial[q1], а все элементы initial[q2..r2] не менее initial[q1]
        int q3 = p3 + (q1 - p1) + (q2 - p2);
        output[q3] = initial[q1];
        #pragma omp parallel
        {
            #pragma omp single nowait
            {
                #pragma omp task
                p_merge(initial, p1, q1 - 1, p2, q2 - 1, output, p3);
                #pragma omp task
                p_merge(initial, q1 + 1, r1, q2, r2, output, q3 + 1);
            }
        }
    }
    return;
}

// p_merge_sort сортирует элементы в initial[p..r] и сохраняет их в output[s..s + r - p]
void p_merge_sort(int* initial, int p, int r, int* output, int s) {
    int n = r - p + 1;
    if (r - p > MAX_SIZE_OF_CHUNK) {
        int* temp =  (int *) malloc(sizeof(int) * n);
        int q = (p + r) / 2;
        int t = q - p + 1; // определяем начальный индекс в temp, указывающий, где будут храниться отсортированные элементы подмассива initial[q+1..r]
        #pragma omp parallel
        {
            #pragma omp single nowait
            {
                #pragma omp task
                p_merge_sort(initial, p, q, temp, 0);
                
                #pragma omp task
                p_merge_sort(initial, q + 1, r, temp, t);
            }
        }
        p_merge(temp, 0, t - 1, t, n - 1, output, s);
        free(temp);
        return;
    }
    else {
        qsort(&initial[p], r - p + 1, sizeof(int), cmpfunc);
        memcpy(&output[s], &initial[p], (r - p + 1) * sizeof(int));
        return;
    }
}

int main(int argc, char** argv) {
    int n; // количество чисел в сортируемом массиве
    int P; // количество потоков
    int m; // максимальный размер чанка

    // считывание данных и выделение памяти
    if (argc != 4) {
        printf("Try better, there are supposed to be 3 of the arguments: n, m, P\n");
        return 0;
    } else {
        printf("Right amount of the arguments, yeeeeee\n");
    }
    n = atoi(argv[1]);
    m = atoi(argv[2]);
    P = atoi(argv[3]);
    MAX_SIZE_OF_CHUNK = m;

    int* array_for_quick_sort = (int *) malloc(sizeof(int) * n);
    if (array_for_quick_sort == NULL) {
        printf("memory allocation error\n");
        exit(1);
    }

    int* array_for_merge_sort = (int *) malloc(sizeof(int) * n);
    if (array_for_merge_sort == NULL) {
        printf("memory allocation error\n");
        free(array_for_quick_sort);
        exit(1);
    }

    int* result_merge_sort = (int *) malloc(sizeof(int) * n);
    if (result_merge_sort == NULL) {
        printf("memory allocation error\n");
        free(array_for_quick_sort);
        free(array_for_merge_sort);
        exit(1);
    }
    
    omp_set_dynamic(0);
    omp_set_num_threads(P);
    
    // генерируем массив для сортировки
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        int t = rand() % 10000;
        array_for_merge_sort[i] = t;
        array_for_quick_sort[i] = t;
    }
    
    FILE* file = fopen("data.txt", "a");
    fseek(file, 0, SEEK_END);
    for (int i = 0; i < n; i++) {
        fprintf(file, "%d ", array_for_merge_sort[i]);
    }
    fprintf(file, "\n");
    
    double start = omp_get_wtime();
    p_merge_sort(array_for_merge_sort, 0, n - 1, result_merge_sort, 0);
    double end = omp_get_wtime();
    double merge_sort_elapsed = end - start;
    printf("%f merge sort time\n", merge_sort_elapsed);

    start = omp_get_wtime();
    qsort(&array_for_quick_sort[0], n, sizeof(int), cmpfunc);
    end = omp_get_wtime();
    double quicksort_elapsed = end - start;
    printf("%f quicksort time\n", quicksort_elapsed);

    // проверяем, корректно ли отработал наш мердж сорт
    for (int i = 0; i < n; i ++) {
        if (array_for_quick_sort[i] == result_merge_sort[i]) {
            if (i == n - 1) {
                printf("all good bro!!!!\n");
            }
        }
        else {
            printf("all bad bro!!!!\n");
            break;
        }
    }
    file = fopen("stats.txt", "a");
    fseek(file, 0, SEEK_END);
    fprintf(file, "%f %d %d %d", merge_sort_elapsed, n, m, P);
    fclose(file);

    
    
    free(array_for_quick_sort);
    free(array_for_merge_sort);
    free(result_merge_sort);
    return 0;
}

