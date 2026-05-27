#include "particle_filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <windows.h>  // Добавить вверху файла вместе с другими #include

#define EPSILON 1e-6
#define TEST_PASSED 0
#define TEST_FAILED 1

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define RUN_TEST(test_func) do { \
    printf("Запуск %s... ", #test_func); \
    tests_run++; \
    if (test_func() == TEST_PASSED) { \
        printf("PASSED\n"); \
        tests_passed++; \
    } else { \
        printf("FAILED\n"); \
        tests_failed++; \
    } \
} while(0)

int assert_close(double a, double b, double eps) {
    if (fabs(a - b) < eps) return TEST_PASSED;
    fprintf(stderr, "\n  %.6f != %.6f (diff: %.6f)\n", a, b, fabs(a-b));
    return TEST_FAILED;
}

/* ==================== ТЕСТЫ ==================== */

int test_init_particles(void) {
    int N = 50;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    init_particles(p, N, 0.0, 1.0);
    
    for (int i = 0; i < N; i++) {
        if (assert_close(p[i].weight, 1.0/N, EPSILON) == TEST_FAILED) {
            free_particles(p);
            return TEST_FAILED;
        }
    }
    free_particles(p);
    return TEST_PASSED;
}

int test_normalize_weights(void) {
    int N = 10;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    for (int i = 0; i < N; i++) {
        p[i].x = i;
        p[i].weight = (i + 1) * 0.5;
    }
    
    normalize_weights(p, N);
    
    double sum = 0.0;
    for (int i = 0; i < N; i++) sum += p[i].weight;
    
    free_particles(p);
    return assert_close(sum, 1.0, 1e-5);
}

int test_update_weights(void) {
    int N = 5;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    for (int i = 0; i < N; i++) {
        p[i].x = i * 1.0;
        p[i].weight = 1.0;
    }
    
    update_weights(p, N, 2.0, 1.0);
    
    int max_idx = 0;
    for (int i = 1; i < N; i++)
        if (p[i].weight > p[max_idx].weight) max_idx = i;
    
    free_particles(p);
    return (max_idx == 2) ? TEST_PASSED : TEST_FAILED;
}

int test_estimate_position(void) {
    int N = 4;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    for (int i = 0; i < N; i++) {
        p[i].x = 5.0;
        p[i].weight = 0.25;
    }
    
    double est = estimate_position(p, N);
    free_particles(p);
    return assert_close(est, 5.0, EPSILON);
}

int test_predict(void) {
    int N = 10;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    for (int i = 0; i < N; i++) {
        p[i].x = 0.0;
        p[i].weight = 1.0/N;
    }
    
    predict(p, N, 1.0, 0.01);
    
    double sum = 0.0;
    for (int i = 0; i < N; i++) sum += p[i].x;
    
    free_particles(p);
    return assert_close(sum/N, 1.0, 0.5);
}

int test_resample(void) {
    int N = 50;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    init_particles(p, N, 0.0, 1.0);
    
    for (int i = 0; i < N; i++)
        p[i].weight = (i + 1) * 0.1;
    
    normalize_weights(p, N);
    resample(p, N);
    
    double sum = 0.0;
    for (int i = 0; i < N; i++) sum += p[i].weight;
    
    free_particles(p);
    return assert_close(sum, 1.0, 1e-5);
}

int test_save_csv(void) {
    int count = 5;
    SimulationResult* res = calloc(count, sizeof(SimulationResult));
    if (!res) return TEST_FAILED;
    
    for (int i = 0; i < count; i++) {
        res[i].step = i;
        res[i].true_pos = i;
        res[i].measurement = i + 0.1;
        res[i].estimate = i + 0.05;
    }
    
    int ret = save_results_to_csv(res, count, "output/test.csv");
    free(res);
    
    if (ret != 0) return TEST_FAILED;
    
    FILE* f = fopen("output/test.csv", "r");
    if (!f) return TEST_FAILED;
    
    char buf[256];
    fgets(buf, sizeof(buf), f);
    fclose(f);
    
    return (strstr(buf, "step") && strstr(buf, "true_position")) ? 
           TEST_PASSED : TEST_FAILED;
}

int test_rmse(void) {
    int count = 3;
    SimulationResult* res = calloc(count, sizeof(SimulationResult));
    if (!res) return TEST_FAILED;
    
    for (int i = 0; i < count; i++) {
        res[i].true_pos = 0.0;
        res[i].estimate = i + 1;
    }
    
    double rmse = calculate_rmse(res, count);
    double expected = sqrt((1.0 + 4.0 + 9.0) / 3.0);
    free(res);
    
    return assert_close(rmse, expected, 0.01);
}

int test_full_cycle(void) {
    int N = 50, steps = 20;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    init_particles(p, N, 0.0, 5.0);
    
    double true_pos = 0.0;
    for (int step = 0; step < steps; step++) {
        true_pos += 1.0;
        double z = true_pos + rand_gaussian(0, 1.0);
        
        predict(p, N, 1.0, 0.1);
        update_weights(p, N, z, 1.0);
        normalize_weights(p, N);
        resample(p, N);
        
        double est = estimate_position(p, N);
        
        if (step >= steps - 5 && fabs(est - true_pos) > 3.0) {
            free_particles(p);
            return TEST_FAILED;
        }
    }
    
    free_particles(p);
    return TEST_PASSED;
}

int test_allocation(void) {
    Particle* p1 = allocate_particles(0);
    if (p1 != NULL) { free_particles(p1); return TEST_FAILED; }
    
    Particle* p2 = allocate_particles(-5);
    if (p2 != NULL) { free_particles(p2); return TEST_FAILED; }
    
    Particle* p3 = allocate_particles(100);
    if (!p3) return TEST_FAILED;
    
    p3[0].x = 42.0;
    p3[99].weight = 0.5;
    
    if (p3[0].x != 42.0 || p3[99].weight != 0.5) {
        free_particles(p3);
        return TEST_FAILED;
    }
    
    free_particles(p3);
    return TEST_PASSED;
}

/* ==================== MAIN ==================== */

int main(void) {
        // Установить кодировку UTF-8 для консоли Windows
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif

    printf("=== Тесты фильтра частиц ===\n\n");
    srand(42);
    
    RUN_TEST(test_allocation);
    RUN_TEST(test_init_particles);
    RUN_TEST(test_normalize_weights);
    RUN_TEST(test_update_weights);
    RUN_TEST(test_estimate_position);
    RUN_TEST(test_predict);
    RUN_TEST(test_resample);
    RUN_TEST(test_save_csv);
    RUN_TEST(test_rmse);
    RUN_TEST(test_full_cycle);
    
    printf("\n=== Результаты ===\n");
    printf("Всего:  %d\n", tests_run);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Процент: %.1f%%\n", (double)tests_passed/tests_run*100);
    
    return (tests_failed == 0) ? 0 : 1;
}