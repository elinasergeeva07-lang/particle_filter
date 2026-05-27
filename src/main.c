// Подключаем заголовочный файл с объявлениями наших функций
#include "particle_filter.h"

// Стандартные библиотеки C
#include <stdio.h>      // ввод-вывод (printf, fprintf)
#include <stdlib.h>     // выделение памяти, atoi, atof
#include <time.h>       // работа со временем (для seed)
#include <unistd.h>     // getopt для разбора аргументов
#include <math.h>       // математические функции (sqrt, exp)
#include <string.h>     // работа со строками
#include <errno.h>      // коды ошибок
#include <windows.h>    // для SetConsoleOutputCP (русский текст в Windows)

// Функция выводит справку по использованию программы
// Вызывается при запуске с флагом -h
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

// Структура для хранения всех параметров симуляции
// Чтобы не передавать кучу аргументов в функции, собираем их в одну структуру
typedef struct {
    int num_particles;      // сколько частиц используем (точность фильтра)
    int num_steps;          // сколько шагов будет в симуляции
    double Q;               // шум процесса (насколько неточно движение)
    double R;               // шум датчика (насколько врёт измерение)
    double velocity;        // скорость объекта (метры за шаг)
    unsigned int seed;      // число для генерации случайных чисел (повторяемость)
    const char* output_file; // имя файла для сохранения результатов
} SimulationParams;

// Заполняет структуру параметров значениями по умолчанию
// Вызывается в начале main(), потом можно переопределить через аргументы
void init_default_params(SimulationParams* params) {
    params->num_particles = 100;      // 100 частиц — хороший баланс точности и скорости
    params->num_steps = 200;          // 200 шагов — достаточно для демонстрации
    params->Q = 0.1;                  // маленький шум движения
    params->R = 1.0;                  // большой шум датчика (чтобы фильтр был полезен)
    params->velocity = 1.0;           // объект движется на 1 метр за шаг
    params->seed = (unsigned int)time(NULL); // случайный seed из текущего времени
    params->output_file = "output/output.csv"; // путь для сохранения
}

// Разбирает аргументы командной строки и заполняет структуру params
// Возвращает 0 при успехе, -1 при ошибке
int parse_arguments(int argc, char* argv[], SimulationParams* params) {
    int opt;
    
    // getopt перебирает опции в формате -n 100 -s 200 и т.д.
    // Строка "n:s:q:r:v:o:h" означает, что эти опции принимают аргументы (двоеточие)
    while ((opt = getopt(argc, argv, "n:s:q:r:v:o:h")) != -1) {
        switch (opt) {
            case 'n':  // количество частиц
                params->num_particles = atoi(optarg);  // строка -> число
                // Проверка: частиц должно быть больше 0
                if (params->num_particles <= 0) {
                    fprintf(stderr, "Ошибка: количество частиц должно быть > 0\n");
                    return -1;
                }
                break;
            case 's':  // количество шагов
                params->num_steps = atoi(optarg);
                if (params->num_steps <= 0) {
                    fprintf(stderr, "Ошибка: количество шагов должно быть > 0\n");
                    return -1;
                }
                break;
            case 'q':  // шум процесса
                params->Q = atof(optarg);  // строка -> дробное число
                if (params->Q < 0) {
                    fprintf(stderr, "Ошибка: Q должно быть >= 0\n");
                    return -1;
                }
                break;
            case 'r':  // шум датчика
                params->R = atof(optarg);
                if (params->R <= 0) {
                    fprintf(stderr, "Ошибка: R должно быть > 0\n");
                    return -1;
                }
                break;
            case 'v':  // скорость
                params->velocity = atof(optarg);
                break;
            case 'seed':  // сид для случайных чисел
                params->seed = (unsigned int)atoi(optarg);
                break;
            case 'o':  // имя выходного файла
                params->output_file = optarg;
                break;
            case 'h':  // справка
                print_usage(argv[0]);
                exit(0);  // завершаем программу после вывода справки
            default:  // неизвестная опция
                print_usage(argv[0]);
                return -1;
        }
    }
    
    return 0;
}

// Главная функция программы — точка входа
int main(int argc, char* argv[]) {
    // Включаем поддержку русского текста в Windows (чтобы не было иероглифов)
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif

    // Создаём структуру и заполняем значениями по умолчанию
    SimulationParams params;
    init_default_params(&params);
    
    // Переопределяем параметры, если пользователь указал их в командной строке
    if (parse_arguments(argc, argv, &params) != 0) {
        return 1;  // ошибка в аргументах — завершаем программу
    }
    
    // Инициализируем генератор случайных чисел (чтобы каждый запуск был разным)
    srand(params.seed);
    
    // Печатаем заголовок и все параметры симуляции — удобно для отладки
    printf("=== Фильтр частиц для отслеживания объекта в 1D пространстве ===\n\n");
    printf("Параметры симуляции:\n");
    printf("  Количество частиц: %d\n", params.num_particles);
    printf("  Количество шагов:  %d\n", params.num_steps);
    printf("  Шум процесса Q:    %.4f\n", params.Q);
    printf("  Шум датчика R:     %.4f\n", params.R);
    printf("  Скорость:          %.4f\n", params.velocity);
    printf("  Seed:              %u\n", params.seed);
    printf("  Выходной файл:     %s\n\n", params.output_file);

    // Создаём массив частиц (основа фильтра)
    Particle* particles = allocate_particles(params.num_particles);
    // Проверяем, что память выделилась успешно
    if (particles == NULL) {
        fprintf(stderr, "Критическая ошибка: не удалось выделить память для частиц\n");
        return 1;
    }
    
    // Создаём массив для хранения результатов каждого шага
    SimulationResult* results = (SimulationResult*)calloc(params.num_steps, 
                                                           sizeof(SimulationResult));
    if (results == NULL) {
        fprintf(stderr, "Критическая ошибка: не удалось выделить память для результатов\n");
        free_particles(particles);  // освобождаем то, что уже выделили
        return 1;
    }

    // Разбрасываем частицы вокруг начальной позиции (0.0) с разбросом 5.0
    printf("Инициализация фильтра...\n");
    init_particles(particles, params.num_particles, 0.0, 5.0);
    
    // Начальное положение объекта (истина) — начинаем с нуля
    double true_position = 0.0;
    
    // Выводим заголовок таблицы результатов
    printf("Запуск симуляции...\n\n");
    printf("%-6s %-15s %-15s %-15s\n", "Step", "True Pos", "Measurement", "Estimate");
    printf("--------------------------------------------------------------\n");

    // Повторяем для каждого шага времени
    for (int step = 0; step < params.num_steps; step++) {

        // Объект движется вперёд на скорость + небольшой случайный шум процесса
        // Это "идеальная" модель, но в реальности есть неопределённость
        true_position += params.velocity + rand_gaussian(0, sqrt(params.Q));

        // Датчик измеряет положение, но добавляет свой шум (часто большой)
        // Это то, что "видит" робот в реальном мире
        double measurement = true_position + rand_gaussian(0, sqrt(params.R));
        
        // Предсказание: сдвигаем все частицы вперёд согласно модели движения
        // Добавляем шум процесса к каждой частице (они "разлетаются")
        predict(particles, params.num_particles, params.velocity, params.Q);
        
        // Обновление весов: сравниваем каждую частицу с измерением датчика
        // Частицы, близкие к измерению, получают больший вес (они "правдоподобнее")
        update_weights(particles, params.num_particles, measurement, params.R);
        
        // Нормализация: делим все веса на их сумму, чтобы сумма стала равна 1
        // Это нужно для корректной вероятностной интерпретации
        normalize_weights(particles, params.num_particles);
        
        // Перевыборка (resample): копируем частицы с большими весами,
        // удаляем частицы с малыми весами. Так фильтр "концентрируется" на лучших гипотезах
        resample(particles, params.num_particles);
        
        // Оценка: считаем взвешенное среднее положений частиц
        // Это и есть итоговый ответ фильтра — где, по его мнению, находится объект
        double estimate = estimate_position(particles, params.num_particles);
        
        // Записываем данные этого шага в массив для последующего вывода/сохранения
        results[step].step = step;
        results[step].true_pos = true_position;      // где объект на самом деле
        results[step].measurement = measurement;     // что показал датчик
        results[step].estimate = estimate;           // что вычислил фильтр
        
        // Выводим в консоль каждый 10-й шаг + последний (чтобы не засорять экран)
        // %-6d — форматирование: число шириной 6 символов, выровнено влево
        if (step % 10 == 0 || step == params.num_steps - 1) {
            printf("%-6d %-15.4f %-15.4f %-15.4f\n", 
                   step, true_position, measurement, estimate);
        }
    }
    
    printf("\n");
    
    // Записываем все результаты в CSV-файл для построения графиков в Excel/Python
    printf("Сохранение результатов в %s...\n", params.output_file);
    if (save_results_to_csv(results, params.num_steps, params.output_file) != 0) {
        fprintf(stderr, "Предупреждение: не удалось сохранить результаты\n");
    } else {
        printf("Результаты успешно сохранены.\n");
    }
    
    // === ВЫЧИСЛЕНИЕ СТАТИСТИКИ ===
    // Считаем среднеквадратичную ошибку (RMSE) — насколько фильтр точен в среднем
    double rmse = calculate_rmse(results, params.num_steps);
    printf("\n=== Статистика ===\n");
    printf("  Среднеквадратичная ошибка (RMSE): %.6f\n", rmse);
    
    // Выводим последние 5 шагов — чтобы увидеть, как фильтр работает "на финише"
    printf("\nПоследние 5 шагов:\n");
    printf("%-6s %-15s %-15s %-15s\n", "Step", "True Pos", "Measurement", "Estimate");
    printf("--------------------------------------------------------------\n");
    for (int i = params.num_steps - 5; i < params.num_steps; i++) {
        printf("%-6d %-15.4f %-15.4f %-15.4f\n",
               results[i].step, results[i].true_pos, 
               results[i].measurement, results[i].estimate);
    }

    // Освобождаем всю динамическую память, которую выделили в начале
    // Это важно, чтобы не было утечек памяти (проверяется valgrind)
    free_particles(particles);
    free(results);
    
    // Завершаем программу успешно
    printf("\nСимуляция завершена успешно.\n");
    return 0;
}