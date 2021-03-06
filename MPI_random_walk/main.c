
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <mpi.h>

const int MASTER = 0;
const int COEF = 2;
const int EXCHANGE_PERIOD = 50;

const int UP = 11;
const int DOWN = 12;
const int LEFT = 13;
const int RIGHT = 14;


typedef struct ctx_type {
    int l;
    int a;
    int b;
    int n;
    int N;
    double p_u;
    double p_d;
    double p_l;
    double p_r;
    int rank;
    int size;
} ctx_t;

typedef struct point_type {
    int x;
    int y;
    int lifetime;
} point_t;


int get_next_process_rank(void* _ctx, int direction) {
    ctx_t* ctx = _ctx;
    int y_rank= ctx->rank / ctx->a;
    int x_rank = ctx->rank % ctx->a;
    
    switch (direction) {
        case 11:
            y_rank++;
            if (y_rank >= ctx->b) {
                y_rank = 0;
            }
            break;
        case 12:
            y_rank--;
            if (y_rank < 0) {
                y_rank = ctx->b - 1;
            }
            break;
        case 13:
            x_rank--;
            if (x_rank < 0) {
                x_rank = ctx->a - 1;
            }
            break;
        case 14:
            x_rank++;
            if (x_rank >= ctx->a) {
                x_rank = 0;
            }
            break;
        default:
            fprintf(stdout, "Something must have gone wrong: invalid direction\n");
    }
    return y_rank * ctx->a + x_rank;
}


int get_next_step_direction(void* _ctx) {
    ctx_t* ctx = _ctx;
    double rand_up = ctx->p_u * rand();
    double rand_down = ctx->p_d * rand();
    double rand_left = ctx->p_l * rand();
    double rand_right = ctx->p_r * rand();
    
    if ((rand_up >= rand_down) && (rand_up >= rand_left) && (rand_up >= rand_right)) {
        return UP;
    } else if ((rand_down >= rand_up) && (rand_down >= rand_left) && (rand_down >= rand_right)) {
        return DOWN;
    } else if ((rand_right >= rand_down) && (rand_right >= rand_up) && (rand_right >= rand_left)) {
        return RIGHT;
    } else {
        return LEFT;
    }
}


int create_seeds(int rank, int size) {
    int* buf = NULL;
    int seed;
    int sendcount = 1;
    int recvcount = 1;
    if (rank == MASTER) {
        srand(time(NULL));
        buf = calloc(size, sizeof(int));
        assert(buf);
        for (int i = 0; i < size; ++i) {
            buf[i] = rand();
        }
    }
    MPI_Scatter(buf, sendcount, MPI_INT, &seed, recvcount, MPI_INT, MASTER, MPI_COMM_WORLD);
    free(buf);
    return seed;
}


void get_stats(void* _ctx, int* final_allocation, struct timespec start) {
    ctx_t* ctx = _ctx;
    FILE* file;
    int num_processed_points = 0;
    
    file = fopen("stats.txt", "w+");
    if (file == NULL) {
        printf("Couldnt' open the file. All bad bro!!!!\n");
        exit(1);
    }
    
    struct timespec finish;
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double elapsed_time;
    elapsed_time = (finish.tv_sec - start.tv_sec);
    elapsed_time += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    fprintf(file, "%d %d %d %d %d %.2f %.2f %.2f %.2f %.4fs\n",
            ctx->l, ctx->a, ctx->b, ctx->n, ctx->N,
            ctx->p_l, ctx->p_r, ctx->p_u, ctx->p_d, elapsed_time);
    for (int i = 0; i < ctx->size; ++i) {
        fprintf(file, "process rank = %d, number of points = %d\n",
                i, final_allocation[i]);
        num_processed_points += final_allocation[i];
    }
    if (num_processed_points == ctx->N * ctx->size) {
        fprintf(file, "The total number of processed points is %d.\n",
                num_processed_points);
        printf("All good bro!!!!\n");
    }
    fclose(file);
}

void finalize(void* _ctx, int* completed_size, int* is_work_finished, struct timespec start) {
    ctx_t* ctx = _ctx;
    
    int num_processed_points = 0;
    MPI_Reduce(completed_size, &num_processed_points, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    
    if ((ctx->rank == 0) && (num_processed_points == ctx->size * ctx->N)) {
        *is_work_finished = 1;
    }
    MPI_Bcast(is_work_finished, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    if (*is_work_finished == 1) {
        int* final_allocation = calloc(ctx->size, sizeof(int));
        assert(final_allocation);
        
        MPI_Gather(completed_size, 1, MPI_INT, final_allocation, 1, MPI_INT,
                   MASTER, MPI_COMM_WORLD);
        
        if (ctx->rank == 0) {
            get_stats(ctx, final_allocation, start);
        }
        
        free(final_allocation);
    }
}

void push(point_t** array, int* size, int* capacity, point_t* element) {
    if (*size < *capacity) {
        (*array)[*size] = *element;
        (*size)++;
    } else {
        *array = realloc(*array, (*capacity) * COEF * sizeof(point_t));
        *capacity *= COEF;
        (*array)[*size] = *element;
        (*size)++;
    }
}

void pop(point_t** array, int* size, int index) {
    (*array)[index] = (*array)[(*size) - 1];
    (*size)--;
}

void random_walk(void* _ctx) {
    ctx_t* ctx = _ctx;
    
    int size_send_to_left = 0;
    int capacity_send_to_left = ctx->N;
    point_t* batch_send_to_left = malloc(capacity_send_to_left * sizeof(point_t));
    assert(batch_send_to_left);

    int size_send_to_right = 0;
    int capacity_send_to_right = ctx->N;
    point_t* batch_send_to_right = malloc(capacity_send_to_right * sizeof(point_t));
    assert(batch_send_to_right);

    int size_send_to_up = 0;
    int capacity_send_to_up = ctx->N;
    point_t* batch_send_to_up = malloc(capacity_send_to_up * sizeof(point_t));
    assert(batch_send_to_up);

    int size_send_to_down = 0;
    int capacity_send_to_down = ctx->N;
    point_t* batch_send_to_down = malloc(capacity_send_to_down * sizeof(point_t));
    assert(batch_send_to_down);

    int completed_size = 0;
    int completed_capacity = ctx->N;
    point_t* completed_batch = malloc(completed_capacity * sizeof(point_t));
    assert(completed_batch);

    int incompleted_size = ctx->N;
    int incompleted_capacity = ctx->N;
    point_t* incompleted_batch = malloc(incompleted_capacity * sizeof(point_t));
    assert(incompleted_batch);

    
    int size_recv_from_left = 0;
    int size_recv_from_right = 0;
    int size_recv_from_up = 0;
    int size_recv_from_down = 0;
    int index_point;
    int flag;
    int seed;
    int direction;
    int i;
    
    seed = create_seeds(ctx->rank, ctx->size);
    srand(seed);
    
    int left_process_rank= get_next_process_rank(ctx, LEFT);
    int right_process_rank = get_next_process_rank(ctx, RIGHT);
    int up_process_rank = get_next_process_rank(ctx, UP);
    int down_process_rank = get_next_process_rank(ctx, DOWN);
    
    for (i = 0; i < ctx->N; ++i) {
        incompleted_batch[i].x = rand() % ctx->l;
        incompleted_batch[i].y = rand() % ctx->l;
        incompleted_batch[i].lifetime = 0;
    }
    
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    while (1) {
        index_point = 0;
        while (index_point < incompleted_size) {
            point_t* cur_point = incompleted_batch + index_point;
            flag = 1;
            for (i = 0; i < EXCHANGE_PERIOD; i++) {
                if (cur_point->lifetime == ctx->n) {
                    push(&completed_batch, &completed_size, &completed_capacity, cur_point);
                    pop(&incompleted_batch, &incompleted_size, index_point);
                    flag = 0;
                    break;
                }
                cur_point->lifetime++;
                direction = get_next_step_direction(ctx);
                if (direction == LEFT) {
                    cur_point->x--;
                } else if (direction == RIGHT) {
                    cur_point->x++;
                } else if (direction == UP) {
                    cur_point->y++;
                } else if (direction == DOWN) {
                    cur_point->y--;
                }

                if (cur_point->x >= ctx->l) {
                    cur_point->x = 0;
                    push(&batch_send_to_right, &size_send_to_right, &capacity_send_to_right, cur_point);
                    pop(&incompleted_batch, &incompleted_size, index_point);
                    flag = 0;
                    break;
                }
                if (cur_point->x < 0) {
                    cur_point->x = ctx->l - 1;
                    push(&batch_send_to_left, &size_send_to_left, &capacity_send_to_left, cur_point);
                    pop(&incompleted_batch, &incompleted_size, index_point);
                    flag = 0;
                    break;
                }
                if (cur_point->y >= ctx->l) {
                    cur_point->y = 0;
                    push(&batch_send_to_up, &size_send_to_up, &capacity_send_to_up, cur_point);
                    pop(&incompleted_batch, &incompleted_size, index_point);
                    flag = 0;
                    break;
                }
                if (cur_point->y < 0) {
                    cur_point->y = ctx->l - 1;
                    push(&batch_send_to_down, &size_send_to_down, &capacity_send_to_down, cur_point);
                    pop(&incompleted_batch, &incompleted_size, index_point);
                    flag = 0;
                    break;
                }
            }
            if (flag == 1) {
                index_point++;
            }
        }
        

        MPI_Request* exchange_sizes = malloc(8 * sizeof(MPI_Request));
        assert(exchange_sizes);
        
        MPI_Isend(&size_send_to_left, 1, MPI_INT, left_process_rank, 0, MPI_COMM_WORLD,
                  exchange_sizes + 0);
        MPI_Isend(&size_send_to_right, 1, MPI_INT, right_process_rank, 1, MPI_COMM_WORLD,
                  exchange_sizes + 1);
        MPI_Isend(&size_send_to_up, 1, MPI_INT, up_process_rank, 2, MPI_COMM_WORLD,
                  exchange_sizes + 2);
        MPI_Isend(&size_send_to_down, 1, MPI_INT, down_process_rank, 3, MPI_COMM_WORLD,
                  exchange_sizes + 3);
        
        MPI_Irecv(&size_recv_from_left, 1, MPI_INT, left_process_rank, 1, MPI_COMM_WORLD,
                  exchange_sizes + 4);
        MPI_Irecv(&size_recv_from_right, 1, MPI_INT, right_process_rank, 0, MPI_COMM_WORLD,
                  exchange_sizes + 5);
        MPI_Irecv(&size_recv_from_up, 1, MPI_INT, up_process_rank, 3, MPI_COMM_WORLD,
                  exchange_sizes + 6);
        MPI_Irecv(&size_recv_from_down, 1, MPI_INT, down_process_rank, 2, MPI_COMM_WORLD,
                  exchange_sizes + 7);
        
        MPI_Waitall(8, exchange_sizes, MPI_STATUS_IGNORE);
        free(exchange_sizes);
        
        point_t* batch_recv_from_left = malloc(size_recv_from_left* sizeof(point_t));
        assert(batch_recv_from_left);

        point_t* batch_recv_from_right = malloc(size_recv_from_right * sizeof(point_t));
        assert(batch_recv_from_right);

        point_t* batch_recv_from_up = malloc(size_recv_from_up * sizeof(point_t));
        assert(batch_recv_from_up);

        point_t* batch_recv_from_down = malloc(size_recv_from_down * sizeof(point_t));
        assert(batch_recv_from_down);

        MPI_Request* exchange_points = malloc(8 * sizeof(MPI_Request));
        assert(exchange_points);
        
        MPI_Isend(batch_send_to_left, size_send_to_left * sizeof(point_t), MPI_BYTE,
                  left_process_rank, 0, MPI_COMM_WORLD, exchange_points + 0);
        MPI_Isend(batch_send_to_right, size_send_to_right * sizeof(point_t), MPI_BYTE,
                  right_process_rank, 1, MPI_COMM_WORLD, exchange_points + 1);
        MPI_Isend(batch_send_to_up, size_send_to_up * sizeof(point_t), MPI_BYTE,
                  up_process_rank, 2, MPI_COMM_WORLD, exchange_points + 2);
        MPI_Isend(batch_send_to_down, size_send_to_down * sizeof(point_t), MPI_BYTE,
                  down_process_rank, 3, MPI_COMM_WORLD, exchange_points + 3);
        
        MPI_Irecv(batch_recv_from_left, size_recv_from_left * sizeof(point_t), MPI_BYTE,
                  left_process_rank, 1, MPI_COMM_WORLD, exchange_points + 4);
        MPI_Irecv(batch_recv_from_right, size_recv_from_right * sizeof(point_t), MPI_BYTE,
                  right_process_rank, 0, MPI_COMM_WORLD, exchange_points + 5);
        MPI_Irecv(batch_recv_from_up, size_recv_from_up * sizeof(point_t), MPI_BYTE,
                  up_process_rank, 3, MPI_COMM_WORLD, exchange_points + 6);
        MPI_Irecv(batch_recv_from_down, size_recv_from_down * sizeof(point_t), MPI_BYTE,
                  down_process_rank, 2, MPI_COMM_WORLD, exchange_points + 7);
        
        MPI_Waitall(8, exchange_points, MPI_STATUS_IGNORE);
        
        size_send_to_left = size_send_to_right = size_send_to_up = size_send_to_down = 0;
        
        free(exchange_points);
        
        for (i = 0; i < size_recv_from_left; ++i) {
            push(&incompleted_batch, &incompleted_size,
                 &incompleted_capacity, batch_recv_from_left + i);
        }
        for (i = 0; i < size_recv_from_right; ++i) {
            push(&incompleted_batch, &incompleted_size,
                 &incompleted_capacity, batch_recv_from_right + i);
        }
        for (i = 0; i < size_recv_from_up; ++i) {
            push(&incompleted_batch, &incompleted_size,
                 &incompleted_capacity, batch_recv_from_up + i);
        }
        for (i = 0; i < size_recv_from_down; ++i) {
            push(&incompleted_batch, &incompleted_size,
                 &incompleted_capacity, batch_recv_from_down + i);
        }
        
        free(batch_recv_from_left);
        free(batch_recv_from_right);
        free(batch_recv_from_up);
        free(batch_recv_from_down);
        
        int is_work_finished = 0;
        finalize(ctx, &completed_size, &is_work_finished, start);
        if (is_work_finished == 1) {
            break;
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    free(batch_send_to_left);
    free(batch_send_to_right);
    free(batch_send_to_up);
    free(batch_send_to_down);
    free(incompleted_batch);
    free(completed_batch);
}


int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank;
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    ctx_t ctx = {
        .l = atoi(argv[1]),
        .a = atoi(argv[2]),
        .b = atoi(argv[3]),
        .n = atoi(argv[4]),
        .N = atoi(argv[5]),
        .p_l = atof(argv[6]),
        .p_r = atof(argv[7]),
        .p_u = atof(argv[8]),
        .p_d = atof(argv[9]),
        .rank = rank,
        .size = size
    };
    
    random_walk(&ctx);
    MPI_Finalize();
    return 0;
}

