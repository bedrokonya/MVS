#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>

//const int AMOUNT_OF_SEEDS = 100;
//unsigned int seeds_for_threads[100];

typedef struct point_stats {
    int finished_way_in_B;
    int life_time;
} point_stats;

typedef struct random_walk_stats {
    int a;
    int b;
    int x;
    int N;
    double p;
    int P;
    double life_expectancy;
    double elapsed;
    double experimental_p;
} random_walk_stats;

int generate_step(random_walk_stats* stats) {
   // int thread_num = omp_get_thread_num();
    int rand_num = rand();
    if ((double) rand_num/RAND_MAX > stats->p) {
        //printf("step = -1\n");
        return -1;
    }
    else {
        //printf("step = 1\n");
        return 1;
    }
}

point_stats random_walk_single_point(random_walk_stats* stats) {
    point_stats result;
    int cur_position = stats->x;
    int life_time = 0;
    while(cur_position != stats->a && cur_position != stats->b) {
        int step = generate_step(stats);
        //printf("cur_position = %d\n", cur_position);
        cur_position += step;
        life_time++;
    }
    if (cur_position == stats->b) {
        result.finished_way_in_B = 1;
    } else {
        result.finished_way_in_B = 0;
    }
    result.life_time = life_time;
    return result;
}

int main(int argc, char** argv) {
    if (argc != 7) {
        printf("Not ehough arguments, try better\n");
        return 0;
    }
    printf("Enough arguments, well done!!!\n");
    
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    int x = atoi(argv[3]);
    int N = atoi(argv[4]);
    double p = atof(argv[5]);
    int P = atoi(argv[6]);
    random_walk_stats stats = {
        stats.a = a,
        stats.b = b,
        stats.x = x,
        stats.N = N,
        stats.p = p,
        stats.P = P,
    };
    
    //    scanf("%d %d %d %d %lf %d", &stats->a, &stats->b, &stats->x, &stats->N, &stats->p, &stats->P);
    // debug vivod
    printf("%d %d %d %d %f %d\n", stats.a, stats.b, stats.x, stats.N, stats.p, stats.P);
    
    
    int sum_life_time = 0;
    int amount_of_points_finished_in_B = 0;
    double start = omp_get_wtime();
    for (int i = 0; i < stats.N; i++) {
        point_stats p = random_walk_single_point(&stats);
        sum_life_time += p.life_time;
        if (p.finished_way_in_B) {
            amount_of_points_finished_in_B += 1;
        }
    }
    double end = omp_get_wtime();
    stats.elapsed = end - start;
    stats.life_expectancy = (double) sum_life_time / stats.N;
    stats.experimental_p = (double) amount_of_points_finished_in_B / stats.N;
    
    FILE* file = fopen("stats.txt", "w");
    //printf("gonna write in the file");
    printf("%f %f %f\n", stats.experimental_p, stats.elapsed, stats.life_expectancy);
    fprintf(file, "%f %f %f %d %d %d %d %f %d\n", stats.experimental_p, stats.elapsed, stats.life_expectancy, a, b, x, N, p, P);
    fclose(file);
    return 0;
}
