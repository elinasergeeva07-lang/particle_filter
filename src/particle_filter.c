#include "particle_filter.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#define M_PI 3.14159265358979323846
/* ==========================================================================
   Вспомогательные функции
   ========================================================================== */

double rand_uniform(void) {
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

double rand_gaussian(double mean, double std_dev) {
    // Преобразование Бокса-Мюллера
    double u1 = rand_uniform();
    double u2 = rand_uniform();
    
    // Защита от log(0)
    if (u1 < 1e-10) {
        u1 = 1e-10;
    }
    
    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    return mean + z0 * std_dev;
}

double gaussian_likelihood(double x, double mean, double var) {
    // w = exp(-0.5 * (x - mean)^2 / var)
    double diff = x - mean;
    double exponent = -0.5 * (diff * diff) / var;
    return exp(exponent);
}

Particle* allocate_particles(int N) {
    if (N <= 0) {
        fprintf(stderr, "Ошибка: некорректное количество частиц %d\n", N);
        return NULL;
    }
    
    Particle* particles = (Particle*)calloc(N, sizeof(Particle));
    if (particles == NULL) {
        fprintf(stderr, "Ошибка выделения памяти: %s\n", strerror(errno));
        return NULL;
    }
    
    return particles;
}

void free_particles(Particle* particles) {
    if (particles != NULL) {
        free(particles);
    }
}

/* ==========================================================================
   Основные функции фильтра частиц
   ========================================================================== */

void init_particles(Particle* particles, int N, double mean, double std_dev) {
    if (particles == NULL || N <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры инициализации\n");
        return;
    }
    
    for (int i = 0; i < N; i++) {
        particles[i].x = rand_gaussian(mean, std_dev);
        particles[i].weight = 1.0 / N; // Равные начальные веса
    }
}

void predict(Particle* particles, int N, double velocity, double Q) {
    if (particles == NULL || N <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры предсказания\n");
        return;
    }
    
    double std_dev = sqrt(Q);
    
    for (int i = 0; i < N; i++) {
        // Движение частицы + шум процесса
        particles[i].x += velocity + rand_gaussian(0, std_dev);
    }
}

void update_weights(Particle* particles, int N, double z, double R) {
    if (particles == NULL || N <= 0 || R <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры обновления весов\n");
        return;
    }
    
    for (int i = 0; i < N; i++) {
        // Вычисляем правдоподобие: насколько частица близка к измерению
        particles[i].weight = gaussian_likelihood(particles[i].x, z, R);
    }
}

void normalize_weights(Particle* particles, int N) {
    if (particles == NULL || N <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры нормализации\n");
        return;
    }
    
    // Вычисляем сумму весов
    double sum = 0.0;
    for (int i = 0; i < N; i++) {
        sum += particles[i].weight;
    }
    
    // Защита от деления на ноль
    if (sum < 1e-10) {
        // Если все веса нулевые, присваиваем равные веса
        double uniform_weight = 1.0 / N;
        for (int i = 0; i < N; i++) {
            particles[i].weight = uniform_weight;
        }
        return;
    }
    
    // Нормализуем
    for (int i = 0; i < N; i++) {
        particles[i].weight /= sum;
    }
}

void resample(Particle* particles, int N) {
    if (particles == NULL || N <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры перевыборки\n");
        return;
    }
    
    // Создаём временный массив для новых частиц
    Particle* temp_particles = allocate_particles(N);
    if (temp_particles == NULL) {
        fprintf(stderr, "Ошибка: не удалось выделить память для ресемплинга\n");
        return;
    }
    
    // Систематический ресемплинг
    double step = 1.0 / N;
    double u0 = rand_uniform() * step;
    
    int j = 0;
    double cumsum = particles[0].weight;
    
    for (int i = 0; i < N; i++) {
        double u = u0 + i * step;
        
        // Находим частицу, соответствующую u
        while (u > cumsum && j < N - 1) {
            j++;
            cumsum += particles[j].weight;
        }
        
        // Копируем частицу
        temp_particles[i].x = particles[j].x;
        temp_particles[i].weight = 1.0 / N; // Сбрасываем веса
    }
    
    // Копируем обратно
    memcpy(particles, temp_particles, N * sizeof(Particle));
    
    // Освобождаем память
    free_particles(temp_particles);
}

double estimate_position(Particle* particles, int N) {
    if (particles == NULL || N <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры оценки\n");
        return 0.0;
    }
    
    double estimate = 0.0;
    for (int i = 0; i < N; i++) {
        estimate += particles[i].x * particles[i].weight;
    }
    
    return estimate;
}

/* ==========================================================================
   Функции для работы с результатами
   ========================================================================== */

int save_results_to_csv(SimulationResult* results, int count, const char* filename) {
    if (results == NULL || filename == NULL || count <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры сохранения\n");
        return -1;
    }
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Ошибка открытия файла %s: %s\n", filename, strerror(errno));
        return -1;
    }
    
    // Заголовок CSV
    fprintf(file, "step,true_position,measurement,estimate\n");
    
    // Данные
    for (int i = 0; i < count; i++) {
        fprintf(file, "%d,%.6f,%.6f,%.6f\n",
                results[i].step,
                results[i].true_pos,
                results[i].measurement,
                results[i].estimate);
    }
    
    fclose(file);
    return 0;
}

double calculate_rmse(SimulationResult* results, int count) {
    if (results == NULL || count <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры расчёта RMSE\n");
        return -1.0;
    }
    
    double sum_sq_error = 0.0;
    for (int i = 0; i < count; i++) {
        double error = results[i].estimate - results[i].true_pos;
        sum_sq_error += error * error;
    }
    
    return sqrt(sum_sq_error / count);
}