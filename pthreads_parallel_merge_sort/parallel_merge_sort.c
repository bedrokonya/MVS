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


typedef struct pthread_chunks_data_t {
    int start_index; // начало куска, который нужно отсортировать
    int chunks_amount_for_thread; // количество чанков, которые нужно отсортировать
    int* chunks_location; // указатель на массив, в котором чанки находятся
    
} pthread_chunks_data_t;

typedef struct pthread_data_t {
    int l1; // начало первого сливаемого куска
    int r1;
    int l2; // начало второго сливаемого куска
    int r2;
    int* location; // указатель на массив, в котором они находятся
} pthread_data_t;


// simple_merge -- старый добрый мердж для одного потока,
// сливает подмассивы initial[l1..r1] и initial[l2..r2]
// в initial[l1..r2]
void simple_merge(int* initial, int l1, int r1, int l2, int r2) {
    int i = 0;
    int j = 0;
    int n1 = r1 - l1 + 1; // размер первого сливаемого подмассива
    int n2 = r2 - l2 + 1; // размер второго сливаемого подмассива
    int* temp1 = malloc(n1 * sizeof(int));
    int* temp2 = malloc(n2 * sizeof(int));
    
    // инициализируем temp1 и temp2
    memcpy(temp1, &initial[l1], n1 * sizeof(int));
    memcpy(temp2, &initial[l2], n2 * sizeof(int));
    
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

// функция для создания потока, который будет соритировать чанки с помощью quicksort'a
void* thread_sort_chunks(void* chunks_data_) {
    pthread_chunks_data_t* chunks_data = (pthread_chunks_data_t*) chunks_data_;
    int start_index = chunks_data->start_index;
    int chunks_amount_for_thread = chunks_data->chunks_amount_for_thread;
    int* chunks_location = chunks_data->chunks_location;
    
    int cur_index = start_index;
    for (int i = 0; i < chunks_amount_for_thread; i++) {
        qsort(&chunks_location[cur_index], m, sizeof(int), cmpfunc);
        cur_index += m;
    }
    return NULL;
}

// функция для для создания потока, который будет мерджить подмассивы
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


// sort_chunks параллельно сортирует чанки размера m с помощью quicksort
void sort_chunks(int* initial, int n) {
    pthread_t* threads = (pthread_t*) malloc(P * sizeof(pthread_t));
    assert(threads);
    pthread_chunks_data_t* thread_data = malloc(P * sizeof(pthread_chunks_data_t));
    assert(thread_data);
    
    int chunks_amount = n / m;
    int chunks_amount_for_thread = chunks_amount / P;
    
    int cur_index = 0;
    
    //создаем потоки
    for(int i = 0; i < P; i++) {
        thread_data[i].start_index = cur_index;
        thread_data[i].chunks_amount_for_thread = chunks_amount_for_thread;
        thread_data[i].chunks_location = initial;
        pthread_create(&(threads[i]), NULL, thread_sort_chunks, &thread_data[i]);
        cur_index += m * chunks_amount_for_thread;
    }
    
    // ждем, когда потоки отработают
    for(int i = 0; i < P; i++) {
        pthread_join(threads[i], NULL);
    }
    
    //нам нужно отсортировать оставшийся хвост (если вдруг таковой имеется)
    qsort(&initial[cur_index], n - cur_index, sizeof(int), cmpfunc);
    
    free(threads);
    free(thread_data);
}

// p_merge_sort сортирует n элементов в массиве initial 
void p_merge_sort(int* initial, int n) {
    
    sort_chunks(initial, n);
    
    pthread_t* threads = malloc(P * sizeof(pthread_t));
    assert(threads);
    pthread_data_t* thread_data = malloc(P * sizeof(pthread_data_t));
    assert(thread_data);
    
    // параллельное слияние подмассивов
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
        }
    }
    
    free(threads);
    free(thread_data);
}



int main(int argc, char** argv) {
    
    if (argc != 4) {
        printf("Try better, there are supposed to be 3 of the arguments: n, m, P\n");
        return 0;
    }
    
    // Считывание данных и выделение памяти
    n = atoi(argv[1]);
    m = atoi(argv[2]);
    P = atoi(argv[3]);
    
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
    
    // Генерируем массив для сортировки
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        int t = rand() % 10000;
        array_for_merge_sort[i] = t;
        array_for_quick_sort[i] = t;
    }
    
    FILE* file = fopen("data.txt", "w");
    for (int i = 0; i < n; i++) {
        fprintf(file, "%d ", array_for_merge_sort[i]);
    }
    fprintf(file, "\n");
    
    
    struct timespec start, finish;
    double merge_sort_elapsed;
    clock_gettime(CLOCK_MONOTONIC, &start);
    p_merge_sort(array_for_merge_sort, n);
    clock_gettime(CLOCK_MONOTONIC, &finish);
    merge_sort_elapsed = (finish.tv_sec - start.tv_sec);
    merge_sort_elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("%f merge sort time\n", merge_sort_elapsed);
    
    for (int i = 0; i < n; i++) {
        fprintf(file, "%d ", array_for_merge_sort[i]);
    }
    
    fprintf(file, "\n\n");
    fclose(file);
    
    double quicksort_elapsed;
    clock_gettime(CLOCK_MONOTONIC, &start);
    qsort(&array_for_quick_sort[0], n, sizeof(int), cmpfunc);
    clock_gettime(CLOCK_MONOTONIC, &finish);
    quicksort_elapsed = (finish.tv_sec - start.tv_sec);
    quicksort_elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
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
    
    return 0;
}

