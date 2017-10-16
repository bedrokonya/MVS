//
//  main.c
//  threads_parallel_merge_sort
//
//  Created by Ирина Дмитриева on 15.10.17.
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <time.h>
#include <assert.h>

int n; // количество чисел в сортируемом массиве
int P; // количество потоков
int m; // максимальный размер чанка

int cmpfunc (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}


typedef struct pthread_chunk_data_t {
    int l; // левая граница чанка
    int r; // правая граница чанка
    int* chunk_location;
    
} pthread_chunk_data_t;

typedef struct pthread_data_t {
    int l1;
    int r1;
    int l2;
    int r2;
    int* location;
} pthread_data_t;


// simple_merge -- старый добрый мердж для одного потока,
// сливает подмассивы initial[p1..r1] и initial[p2..r2]
// в output[p3..r3], однако не предполагается, что initial[p1..r1] и initial[p2..r2]
// граничат друг с другом
void simple_merge(int* initial, int l1, int r1, int l2, int r2) {
    int i = 0;
    int j = 0;
    int n1 = r1 - l1 + 1; // размер первого сливаемого подмассива
    int n2 = r2 - l2 + 1; // размер второго
    int* temp1 = malloc(n1 * sizeof(int));
    int* temp2 = malloc(n2 * sizeof(int));
    
    // инициализируем temp1 и temp2
    for(i = 0; i < n1; i++) {
        temp1[i] = initial[l1 + i];
    }
    for(i = 0; i < n2; i++) {
        temp2[i] = initial[l2 + i];
    }
    
    i = 0;
    
    while((i < n1) && (j < n2)) {
        if(temp1[i] < temp2[j]) {
            initial[l1 + i + j] = temp1[i];
            i++;
        } else {
            initial[l1 + i + j] = temp2[j];
            j++;
        }
    }
    while(i < n1) {
        initial[l1 + i + j] = temp1[i];
        i++;
    }
    while(j < n2) {
        initial[l1 + i + j] = temp2[j];
        j++;
    }
    
    free(temp1);
    free(temp2);
}


void* thread_sort_chunk(void* chunk_data_) {
    pthread_chunk_data_t* chunk_data = (pthread_chunk_data_t*) chunk_data_;
    int l = chunk_data->l;
    int r = chunk_data->r;
    int* chunk_location = chunk_data->chunk_location;
    qsort(&chunk_location[l], r - l + 1, sizeof(int), cmpfunc);
    return NULL;
}

void* thread_simple_merge(void* data_) {
    pthread_data_t* data = (pthread_data_t*) data_;
    int l1 = data->l1;
    int r1 = data->r1;
    int l2 = data->l2;
    int r2 = data->r2;
    int* location = data->location;
    
    simple_merge(location, l1, r1, l2, r2);
    return NULL;
}

// sort_chunks разбивает initial на чанки и параллельно сортирует их с помощью pthreads
void sort_chunks(int* initial, int n) {
    pthread_t* threads = (pthread_t*) malloc(P * sizeof(pthread_t));
    assert(threads);
    pthread_chunk_data_t* thread_data = malloc(P * sizeof(pthread_chunk_data_t));
    assert(thread_data);
    
    int cur_index = 0;
    int num_threads = 0;
    while(cur_index + P * m < n) {
        for(int i = 0; i < P; i++) {
            thread_data[i].l = cur_index;
            thread_data[i].r = cur_index + m - 1;
            thread_data[i].chunk_location = initial;
            pthread_create(&(threads[i]), NULL, thread_sort_chunk, &thread_data[i]);
            cur_index += m;
        }
        
        for(int i = 0; i < P; i++) {
            pthread_join(threads[i], NULL);
        }
    }
    //_____________
//    for (int i = 0; i < n; i++) {
//        printf("%d ", initial[i]);
//    }
//    printf("\n");
    //_____________
    
    //нам нужно отсортировать оставшийся хвост (если вдруг таковой имеется)
    qsort(&initial[cur_index], n - cur_index, sizeof(int), cmpfunc);
    
    //_____________
//    for (int i = 0; i < n; i++) {
//        printf("%d ", initial[i]);
//    }
//    printf("\n");
    //_____________
    
    free(threads);
    free(thread_data);
}


void p_merge_sort(int* initial, int n) {
    
    //_________________
    for (int i = 0; i < n; i++) {
        printf("%d ", initial[i]);
    }
    printf("\n");
    //_________________
    
    sort_chunks(initial, n);
    
    //________________
    for (int i = 0; i < n; i++) {
        printf("%d ", initial[i]);
    }
    printf("\n");

    //_________________
    
    pthread_t* threads = (pthread_t*) malloc(P * sizeof(pthread_t));
    assert(threads);
    pthread_data_t* thread_data = malloc(P * sizeof(pthread_chunk_data_t));
    assert(thread_data);
    
    
    for (int cur_size = m; cur_size < n; cur_size *= 2) {
        int cur_index = 0;
        int num_threads = P;
        while(cur_index + cur_size + 1 < n) {
            for(int i = 0; i < P; i++) {
                int l1 = cur_index;
                int r1 = cur_index + cur_size - 1;
                if (r1 >= n) {
                    num_threads = i;
                    break;
                }
                int l2 = r1 + 1;
                int r2 = l2 + cur_size - 1;
                if (r2 > n - 1) {
                    r2 = n - 1;
                }
                
                cur_index = r2 + 1;
                
                thread_data[i].l1 = l1;
                thread_data[i].r1 = r1;
                thread_data[i].l2 = l2;
                thread_data[i].r2 = r2;
                thread_data[i].location = initial;
                
                pthread_create(&(threads[i]), NULL, thread_simple_merge, &thread_data[i]);
            }
            
            for(int i = 0; i < num_threads; i++) {
                pthread_join(threads[i], NULL);
            }

            //_________________
            for (int i = 0; i < n; i++) {
                printf("%d ", initial[i]);
            }
            printf("\n");

            //_________________
        }
    }
    for(int i = 0; i < P; i++) {
                            
        thread_data[i].l1 = 0;
        thread_data[i].r1 = 0;
        thread_data[i].l2 = 0;
        thread_data[i].r2 = 0;
        threads[i] = 0;
    }
    
    //free(threads);
    //free(thread_data);
}




int main(int argc, char** argv) {
    
    
    if (argc != 4) {
        printf("Try better, there are supposed to be 3 of the arguments: n, m, P\n");
        return 0;
    }

    // считывание данных и выделение памяти
    n = atoi(argv[1]);
    m = atoi(argv[2]);
    P = atoi(argv[3]);
    //scanf("%d", &n);
    //scanf("%d", &m);
    //scanf("%d", &P);
    
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
    
    //int* result_merge_sort = (int *) malloc(sizeof(int) * n);
    //if (result_merge_sort == NULL) {
    //    printf("memory allocation error\n");
    //    free(array_for_quick_sort);
    //    free(array_for_merge_sort);
    //    exit(1);
    //}
    
    //omp_set_dynamic(0);
    //omp_set_num_threads(P);
    
    // Генерируем массив для сортировки
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        int t = rand() % 100;
        array_for_merge_sort[i] = t;
        array_for_quick_sort[i] = t;
    }
    
    
    for (int i = 0; i < n; i++) {
        printf("%d ",  array_for_merge_sort[i]);
    }
    printf("\n");
//    int array_for_merge_sort[20] = {5, 37, 7, 75, 94, 12, 1, 26, 16, 69, 1, 32, 42, 9, 24, 45, 37, 8, 34, 91 };
//    int array_for_quick_sort[20] = {5, 37, 7, 75, 94, 12, 1, 26, 16, 69, 1, 32, 42, 9, 24, 45, 37, 8, 34, 91 };
    
    FILE* file = fopen("data.txt", "w");
    for (int i = 0; i < n; i++) {
        fprintf(file, "%d ", array_for_merge_sort[i]);
    }
    fprintf(file, "\n");
    
    double start = clock();
    p_merge_sort(array_for_merge_sort, n);
    double end = clock();
    double merge_sort_elapsed = difftime(end, start) / CLOCKS_PER_SEC;
    printf("%f merge sort time\n", merge_sort_elapsed);
    
    for (int i = 0; i < n; i++) {
        fprintf(file, "%d ", array_for_merge_sort[i]);
    }
    fprintf(file, "\n\n");
    fclose(file);
    
    start = clock();
    qsort(&array_for_quick_sort[0], n, sizeof(int), cmpfunc);
    end = clock();
    
    for (int i = 0; i < n; i++) {
        printf("%d ",  array_for_quick_sort[i]);
    }
    printf("\n");
    
    
    double quicksort_elapsed = difftime(end, start) / CLOCKS_PER_SEC;
    printf("%f quicksort time\n", quicksort_elapsed);
    
    
    
    // Проверяем, корректно ли отработал наш мердж сорт
    for (int i = 0; i < n; i ++) {
        if (array_for_quick_sort[i] == array_for_merge_sort[i]) {
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
    fprintf(file, "%f %f %d %d %d\n", merge_sort_elapsed, quicksort_elapsed, n, m, P);
    fclose(file);
    
    
    free(array_for_quick_sort);
    free(array_for_merge_sort);
    //free(result_merge_sort);
    return 0;
}


