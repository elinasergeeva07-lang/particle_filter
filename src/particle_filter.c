// Подключаем заголовочный файл с объявлениями структур и функций
#include "particle_filter.h"

// Стандартные библиотеки C
#include <stdlib.h>     // calloc, free, malloc
#include <stdio.h>      // printf, fprintf, stderr
#include <math.h>       // sqrt, exp, log, cos, M_PI
#include <string.h>     // memcpy, strerror
#include <errno.h>      // коды ошибок системы

// Определяем константу пи
#define M_PI 3.14159265358979323846

// Генерирует случайное число от 0.0 до 1.0 (равномерное распределение)
double rand_uniform(void) {
    // Делим rand() на максимальное значение + 1, чтобы получить дробь [0, 1)
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

// Генерирует случайное число из гауссова распределения (нормальное)
// mean - среднее значение (центр "колокола")
// std_dev - стандартное отклонение (ширина "колокола")
double rand_gaussian(double mean, double std_dev) {
    // Используем преобразование Бокса-Мюллера: из двух равномерных делаем одно гауссово
    
    // Получаем два случайных числа от 0 до 1
    double u1 = rand_uniform();
    double u2 = rand_uniform();
    
    // Защита: log(0) невозможен, поэтому если u1 очень близко к 0 — заменяем
    if (u1 < 1e-10) {
        u1 = 1e-10;
    }
    
    // Формула Бокса-Мюллера: превращаем равномерные числа в гауссово
    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    
    // Сдвигаем и масштабируем результат под нужные mean и std_dev
    return mean + z0 * std_dev;
}

// Вычисляет гауссову функцию правдоподобия
// x - положение частицы
// mean - измерение с датчика
// var - дисперсия шума датчика (R)
// Возвращает: насколько частица "похожа" на измерение (чем ближе, тем больше число)
double gaussian_likelihood(double x, double mean, double var) {
    // Находим разницу между положением частицы и измерением
    double diff = x - mean;
    
    // Вычисляем экспоненту: -0.5 * (разница)^2 / дисперсия
    // Чем больше разница — тем меньше результат (частица менее правдоподобна)
    double exponent = -0.5 * (diff * diff) / var;
    
    // Возвращаем exp(экспонента) — это и есть вес частицы
    return exp(exponent);
}

// Выделяет память под массив из N частиц
// Возвращает указатель на массив или NULL при ошибке
Particle* allocate_particles(int N) {
    // Проверяем: количество частиц должно быть положительным
    if (N <= 0) {
        fprintf(stderr, "Ошибка: некорректное количество частиц %d\n", N);
        return NULL;  // не выделяем память при ошибке
    }
    
    // Выделяем память и сразу обнуляем (calloc)
    Particle* particles = (Particle*)calloc(N, sizeof(Particle));
    
    // Проверяем, что память действительно выделилась
    if (particles == NULL) {
        fprintf(stderr, "Ошибка выделения памяти: %s\n", strerror(errno));
        return NULL;
    }
    
    // Возвращаем указатель на выделенный массив
    return particles;
}

// Освобождает память, выделенную под частицы
void free_particles(Particle* particles) {
    // Проверяем, что указатель не NULL (чтобы не упасть)
    if (particles != NULL) {
        free(particles);  // освобождаем память
    }
}

// Инициализирует частицы: задаёт начальные позиции и веса
// particles - массив частиц
// N - количество частиц
// mean, std_dev - параметры начального распределения (где искать объект в начале)
void init_particles(Particle* particles, int N, double mean, double std_dev) {
    // Проверяем входные параметры на корректность
    if (particles == NULL || N <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры инициализации\n");
        return;  // выходим, если что-то не так
    }
    
    // Проходим по всем частицам
    for (int i = 0; i < N; i++) {
        // Задаём случайную позицию из гауссова распределения
        particles[i].x = rand_gaussian(mean, std_dev);
        
        // Задаём одинаковый начальный вес: 1 / N
        // (все гипотезы равноправны в начале)
        particles[i].weight = 1.0 / N;
    }
}

// Шаг предсказания: сдвигаем частицы согласно модели движения
// velocity - скорость объекта (метры за шаг)
// Q - дисперсия шума процесса (насколько неточно движение)
void predict(Particle* particles, int N, double velocity, double Q) {
    // Проверяем параметры
    if (particles == NULL || N <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры предсказания\n");
        return;
    }
    
    // Стандартное отклонение = корень из дисперсии
    double std_dev = sqrt(Q);
    
    // Сдвигаем каждую частицу
    for (int i = 0; i < N; i++) {
        // Новая позиция = старая + скорость + случайный шум
        // Шум добавляет неопределённость: мы не знаем точно, как двигался объект
        particles[i].x += velocity + rand_gaussian(0, std_dev);
    }
}

// Обновляет веса частиц: сравниваем с измерением датчика
// z - измерение с датчика (зашумлённое)
// R - дисперсия шума датчика (насколько датчик врёт)
void update_weights(Particle* particles, int N, double z, double R) {
    // Проверяем параметры
    if (particles == NULL || N <= 0 || R <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры обновления весов\n");
        return;
    }
    
    // Проходим по всем частицам
    for (int i = 0; i < N; i++) {
        // Вычисляем правдоподобие: насколько позиция частицы близка к измерению
        // Частицы рядом с z получат большой вес, далёкие — маленький
        particles[i].weight = gaussian_likelihood(particles[i].x, z, R);
    }
}

// Нормализует веса: делает так, чтобы их сумма была равна 1.0
void normalize_weights(Particle* particles, int N) {
    // Проверяем параметры
    if (particles == NULL || N <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры нормализации\n");
        return;
    }
    
    // Считаем сумму всех весов
    double sum = 0.0;
    for (int i = 0; i < N; i++) {
        sum += particles[i].weight;
    }
    
    // Защита от деления на ноль: если сумма очень маленькая
    if (sum < 1e-10) {
        // Присваиваем всем частицам одинаковый вес
        double uniform_weight = 1.0 / N;
        for (int i = 0; i < N; i++) {
            particles[i].weight = uniform_weight;
        }
        return;  // выходим после обработки
    }
    
    // Делим каждый вес на сумму: теперь сумма всех весов = 1.0
    for (int i = 0; i < N; i++) {
        particles[i].weight /= sum;
    }
}

// Перевыборка (resample): копируем "удачные" частицы, удаляем "неудачные"
// После этой функции частицы с большими весами встречаются чаще
void resample(Particle* particles, int N) {
    // Проверяем параметры
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
    
    // Шаг между точками выборки: 1 / N
    double step = 1.0 / N;
    
    // Случайный сдвиг начала: чтобы выборка была случайной
    double u0 = rand_uniform() * step;
    
    // Индекс текущей частицы-источника
    int j = 0;
    
    // Накопленная сумма весов (начинаем с первой частицы)
    double cumsum = particles[0].weight;
    
    // Проходим по всем позициям в новом массиве
    for (int i = 0; i < N; i++) {
        // Вычисляем текущую точку выборки
        double u = u0 + i * step;
        
        // Ищем частицу, чей накопленный вес покрывает точку u
        // Двигаемся вперёд, пока не найдём нужную
        while (u > cumsum && j < N - 1) {
            j++;  // переходим к следующей частице
            cumsum += particles[j].weight;  // добавляем её вес к сумме
        }
        
        // Копируем позицию найденной частицы в новый массив
        temp_particles[i].x = particles[j].x;
        
        // Сбрасываем вес на равномерный (1/N) — после ресемплинга все равны
        temp_particles[i].weight = 1.0 / N;
    }
    
    // Копируем новый массив обратно в старый (заменяем частицы)
    memcpy(particles, temp_particles, N * sizeof(Particle));
    
    // Освобождаем временную память
    free_particles(temp_particles);
}

// Вычисляет оценку положения: взвешенное среднее всех частиц
// Возвращает одно число — где, по мнению фильтра, находится объект
double estimate_position(Particle* particles, int N) {
    // Проверяем параметры
    if (particles == NULL || N <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры оценки\n");
        return 0.0;
    }
    
    // Накопитель для взвешенной суммы
    double estimate = 0.0;
    
    // Складываем: позиция * вес для каждой частицы
    for (int i = 0; i < N; i++) {
        estimate += particles[i].x * particles[i].weight;
    }
    
    // Возвращаем итоговую оценку
    return estimate;
}

// Сохраняет результаты симуляции в CSV-файл
// results - массив структур с данными
// count - количество записей
// filename - имя файла для сохранения
// Возвращает: 0 при успехе, -1 при ошибке
int save_results_to_csv(SimulationResult* results, int count, const char* filename) {
    // Проверяем входные параметры
    if (results == NULL || filename == NULL || count <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры сохранения\n");
        return -1;
    }
    
    // Открываем файл для записи
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Ошибка открытия файла %s: %s\n", filename, strerror(errno));
        return -1;
    }
    
    // Пишем заголовок CSV: названия колонок
    fprintf(file, "step,true_position,measurement,estimate\n");
    
    // Пишем данные: по одной строке на каждый шаг
    for (int i = 0; i < count; i++) {
        fprintf(file, "%d,%.6f,%.6f,%.6f\n",
                results[i].step,           // номер шага
                results[i].true_pos,       // истинное положение
                results[i].measurement,    // измерение с шумом
                results[i].estimate);      // оценка фильтра
    }
    
    // Закрываем файл
    fclose(file);
    
    // Возвращаем успех
    return 0;
}

// Вычисляет среднеквадратичную ошибку (RMSE)
// Показывает, насколько в среднем оценка фильтра отличается от истины
double calculate_rmse(SimulationResult* results, int count) {
    // Проверяем параметры
    if (results == NULL || count <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры расчёта RMSE\n");
        return -1.0;
    }
    
    // Накопитель для суммы квадратов ошибок
    double sum_sq_error = 0.0;
    
    // Проходим по всем результатам
    for (int i = 0; i < count; i++) {
        // Находим ошибку: оценка минус истина
        double error = results[i].estimate - results[i].true_pos;
        
        // Добавляем квадрат ошибки к сумме
        sum_sq_error += error * error;
    }
    
    // Делим сумму на количество, берём корень — это и есть RMSE
    return sqrt(sum_sq_error / count);
}