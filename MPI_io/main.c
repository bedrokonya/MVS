#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <mpi.h>

const int MASTER = 0;

typedef struct ctx_type {
    int l;
    int a;
    int b;
    int n;
    int N;
    int rank;
    int size;
} ctx_t;


typedef struct point_type {
    int x;
    int y;
    int r;
} point_t;

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

void run(void* _ctx) {
    ctx_t* ctx = _ctx;
    unsigned int seed = create_seeds(ctx->rank, ctx->size);
    
    int* data = calloc(ctx->size * ctx->l * ctx->l, sizeof(int));
    assert(data);
    for (int i = 0; i < ctx->N; ++i) {
        int x = rand_r(&seed) % ctx->l;
        int y = rand_r(&seed) % ctx->l;
        int r = rand_r(&seed) % ctx->size;
        data[(y * ctx->l + x) * ctx->size + r] += 1;
    }

    
    MPI_File file;
    MPI_File_open(MPI_COMM_WORLD, "data.bin", MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &file);
    MPI_Datatype type;
    MPI_Type_vector(ctx->l, ctx->l * ctx->size, ctx->l * ctx->size * ctx->a, MPI_INT, &type);
    MPI_Type_commit(&type);
    
    int y_rank = ctx->rank / ctx->a;
    int x_rank = ctx->rank % ctx->a;
    int offset = (y_rank * ctx->a * ctx->l * ctx->l * ctx->size + x_rank * ctx->l * ctx->size) * sizeof(int);
    
    MPI_File_set_view(file, offset, MPI_INT, type, "native", MPI_INFO_NULL);
    MPI_File_write(file, data, ctx->l * ctx->l * ctx->size, MPI_INT, MPI_STATUS_IGNORE);
    MPI_Type_free(&type);
    MPI_File_close(&file);
    
    free(data);
}

void check_for_correctness(ctx_t* _ctx) {
	ctx_t* ctx = _ctx;
	int l, a, b, N;
	int i;
	int* process_data;
	FILE* file;
	int* buffer;
	int x, y, t, s, r;
	int is_correct = 1;
	l = ctx->l;
	a = ctx->a;
	b = ctx->b;
	N = ctx->N;
	process_data = calloc(a * b, sizeof(int));
	buffer = malloc(l*l*a*a*b*b * sizeof(int));
	file = fopen("data.bin", "rb");
	fread(buffer, sizeof(int), l*l*a*a*b*b, file);

	for (x = 0; x < a; x++)
		for (y = 0; y < b; y++)
			for (t = 0; t < l; t++)
				for (s = 0; s < l; s++)
					for (r = 0; r < a * b; r++)
						process_data[a * y + x] += buffer[(y * l + s) * a * l * a * b + (x * l + t) * a * b + r];

	for (i = 0; i < a * b; ++i) {
		if (process_data[i] != N) {
			is_correct = 0;
			break;
		}
	}
	if(is_correct) {
		printf("All good bro!!!!\n");
	} else {
		printf("All bad bro!!!!\n");
	}

	fclose(file);
	free(buffer);
	free(process_data);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank;
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    ctx_t ctx = {
        .l = atoi(argv[1]),
        .a = atoi(argv[2]),
        .b = atoi(argv[3]),
        .N = atoi(argv[4]),
        .rank = rank,
        .size = size
    };
    
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    run(&ctx);
    struct timespec finish;
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double elapsed_time;
    elapsed_time = (finish.tv_sec - start.tv_sec);
    elapsed_time += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

    
    if (ctx.rank == MASTER) {
        FILE* file = fopen("stats.txt", "w+");
        if (file == NULL) {
            fprintf(stderr, "Couldn't open the file. All bad bro!!!!\n");
            exit(1);
        } else {
			check_for_correctness(&ctx);
            fprintf(file, "%d %d %d %d %0.2f\n", ctx.l, ctx.a, ctx.b, ctx.N, elapsed_time);
			fclose(file);
        }
    }
	
    MPI_Finalize();
    return 0;
}
