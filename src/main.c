#include "particle_filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <windows.h>  // Добавить вверху файла вместе с другими #include

/**
 * @brief Вывод справки по использованию программы
 */
void print_usage(const char* program_name) {
    printf("Использование: %s [опции]\n", program_name);
    printf("\nОпции:\n");
    printf("  -n <число>       Количество частиц (по умолчанию: 100)\n");
    printf("  -s <число>       Количество шагов симуляции (по умолчанию: 200)\n");
    printf("  -q <число>       Дисперсия шума процесса Q (по умолчанию: 0.1)\n");
    printf("  -r <число>       Дисперсия шума датчика R (по умолчанию: 1.0)\n");
    printf("  -v <число>       Скорость движения объекта (по умолчанию: 1.0)\n");
    printf("  -seed <число>    Сид генератора случайных чисел (по умолчанию: время)\n");
    printf("  -o <файл>        Имя выходного CSV файла (по умолчанию: output/output.csv)\n");
    printf("  -h               Показать эту справку\n");
    printf("\nПример:\n");
    printf("  %s -n 100 -s 200 -q 0.1 -r 1.0 -v 1.0 -seed 42\n", program_name);
}

/**
 * @brief Структура для хранения параметров симуляции
 */
typedef struct {
    int num_particles;
    int num_steps;
    double Q;           // Дисперсия шума процесса
    double R;           // Дисперсия шума датчика
    double velocity;    // Скорость объекта
    unsigned int seed;
    const char* output_file;
} SimulationParams;

/**
 * @brief Инициализация параметров значениями по умолчанию
 */
void init_default_params(SimulationParams* params) {
    params->num_particles = 100;
    params->num_steps = 200;
    params->Q = 0.1;
    params->R = 1.0;
    params->velocity = 1.0;
    params->seed = (unsigned int)time(NULL);
    params->output_file = "output/output.csv";
}

/**
 * @brief Парсинг аргументов командной строки
 */
int parse_arguments(int argc, char* argv[], SimulationParams* params) {
    int opt;
    
    while ((opt = getopt(argc, argv, "n:s:q:r:v:o:h")) != -1) {
        switch (opt) {
            case 'n':
                params->num_particles = atoi(optarg);
                if (params->num_particles <= 0) {
                    fprintf(stderr, "Ошибка: количество частиц должно быть > 0\n");
                    return -1;
                }
                break;
            case 's':
                params->num_steps = atoi(optarg);
                if (params->num_steps <= 0) {
                    fprintf(stderr, "Ошибка: количество шагов должно быть > 0\n");
                    return -1;
                }
                break;
            case 'q':
                params->Q = atof(optarg);
                if (params->Q < 0) {
                    fprintf(stderr, "Ошибка: Q должно быть >= 0\n");
                    return -1;
                }
                break;
            case 'r':
                params->R = atof(optarg);
                if (params->R <= 0) {
                    fprintf(stderr, "Ошибка: R должно быть > 0\n");
                    return -1;
                }
                break;
            case 'v':
                params->velocity = atof(optarg);
                break;
            case 'seed':
                params->seed = (unsigned int)atoi(optarg);
                break;
            case 'o':
                params->output_file = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
            default:
                print_usage(argv[0]);
                return -1;
        }
    }
    
    return 0;
}

/**
 * @brief Основная функция программы
 */
int main(int argc, char* argv[]) {
    // Установить кодировку UTF-8 для консоли Windows
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif

    // Инициализация параметров
    SimulationParams params;
    init_default_params(&params);
    
    // Парсинг аргументов
    if (parse_arguments(argc, argv, &params) != 0) {
        return 1;
    }
    
    // Инициализация генератора случайных чисел
    srand(params.seed);
    
    printf("=== Фильтр частиц для отслеживания объекта в 1D пространстве ===\n\n");
    printf("Параметры симуляции:\n");
    printf("  Количество частиц: %d\n", params.num_particles);
    printf("  Количество шагов:  %d\n", params.num_steps);
    printf("  Шум процесса Q:    %.4f\n", params.Q);
    printf("  Шум датчика R:     %.4f\n", params.R);
    printf("  Скорость:          %.4f\n", params.velocity);
    printf("  Seed:              %u\n", params.seed);
    printf("  Выходной файл:     %s\n\n", params.output_file);
    
    // Выделение памяти
    Particle* particles = allocate_particles(params.num_particles);
    if (particles == NULL) {
        fprintf(stderr, "Критическая ошибка: не удалось выделить память для частиц\n");
        return 1;
    }
    
    SimulationResult* results = (SimulationResult*)calloc(params.num_steps, 
                                                           sizeof(SimulationResult));
    if (results == NULL) {
        fprintf(stderr, "Критическая ошибка: не удалось выделить память для результатов\n");
        free_particles(particles);
        return 1;
    }
    
    // Инициализация частиц (начальное положение около 0)
    printf("Инициализация фильтра...\n");
    init_particles(particles, params.num_particles, 0.0, 5.0);
    
    // Начальное положение объекта
    double true_position = 0.0;
    
    printf("Запуск симуляции...\n\n");
    printf("%-6s %-15s %-15s %-15s\n", "Step", "True Pos", "Measurement", "Estimate");
    printf("--------------------------------------------------------------\n");
    
    // Главный цикл симуляции
    for (int step = 0; step < params.num_steps; step++) {
        // 1. Движение объекта (модель)
        true_position += params.velocity + rand_gaussian(0, sqrt(params.Q));
        
        // 2. Получение зашумлённого измерения
        double measurement = true_position + rand_gaussian(0, sqrt(params.R));
        
        // 3. Шаг фильтра частиц
        // Predict: предсказание положения частиц
        predict(particles, params.num_particles, params.velocity, params.Q);
        
        // Update: обновление весов на основе измерения
        update_weights(particles, params.num_particles, measurement, params.R);
        
        // Normalize: нормализация весов
        normalize_weights(particles, params.num_particles);
        
        // Resample: перевыборка частиц
        resample(particles, params.num_particles);
        
        // Estimate: оценка положения
        double estimate = estimate_position(particles, params.num_particles);
        
        // Сохранение результатов
        results[step].step = step;
        results[step].true_pos = true_position;
        results[step].measurement = measurement;
        results[step].estimate = estimate;
        
        // Вывод каждого 10-го шага (чтобы не засорять консоль)
        if (step % 10 == 0 || step == params.num_steps - 1) {
            printf("%-6d %-15.4f %-15.4f %-15.4f\n", 
                   step, true_position, measurement, estimate);
        }
    }
    
    printf("\n");
    
    // Сохранение результатов в CSV
    printf("Сохранение результатов в %s...\n", params.output_file);
    if (save_results_to_csv(results, params.num_steps, params.output_file) != 0) {
        fprintf(stderr, "Предупреждение: не удалось сохранить результаты\n");
    } else {
        printf("Результаты успешно сохранены.\n");
    }
    
    // Вычисление и вывод статистики
    double rmse = calculate_rmse(results, params.num_steps);
    printf("\n=== Статистика ===\n");
    printf("  Среднеквадратичная ошибка (RMSE): %.6f\n", rmse);
    
    // Вывод последних значений
    printf("\nПоследние 5 шагов:\n");
    printf("%-6s %-15s %-15s %-15s\n", "Step", "True Pos", "Measurement", "Estimate");
    printf("--------------------------------------------------------------\n");
    for (int i = params.num_steps - 5; i < params.num_steps; i++) {
        printf("%-6d %-15.4f %-15.4f %-15.4f\n",
               results[i].step, results[i].true_pos, 
               results[i].measurement, results[i].estimate);
    }
    
    // Освобождение памяти
    free_particles(particles);
    free(results);
    
    printf("\nСимуляция завершена успешно.\n");
    return 0;
}