#ifndef PARTICLE_FILTER_H
#define PARTICLE_FILTER_H

#include <stdbool.h>
#include <stddef.h>

// Структура частицы
typedef struct {
    double x;      // положение частицы
    double weight; // вес частицы
} Particle;

// Структура для результатов
typedef struct {
    int step;           // номер шага
    double true_pos;    // истинное положение
    double measurement; // измерение с шумом
    double estimate;    // оценка фильтра
} SimulationResult;

// Основные функции фильтра

// Инициализация частиц
// particles - массив частиц
// N - количество
// mean, std_dev - параметры начального распределения
void init_particles(Particle* particles, int N, double mean, double std_dev);

// Шаг предсказания (движение + шум)
// velocity - скорость объекта
// Q - шум процесса
void predict(Particle* particles, int N, double velocity, double Q);

// Вычисление весов частиц
// z - измерение с датчика
// R - шум датчика
void update_weights(Particle* particles, int N, double z, double R);

// Нормализация весов (сумма = 1)
void normalize_weights(Particle* particles, int N);

// Перевыборка частиц (копируем лучшие, удаляем худшие)
void resample(Particle* particles, int N);

// Оценка положения (взвешенное среднее)
double estimate_position(Particle* particles, int N);

// Вспомогательные функции

// Случайное число из гауссова распределения
double rand_gaussian(double mean, double std_dev);

// Случайное число [0, 1)
double rand_uniform(void);

// Гауссова функция правдоподобия
double gaussian_likelihood(double x, double mean, double var);

// Выделение памяти для частиц
Particle* allocate_particles(int N);

// Освобождение памяти
void free_particles(Particle* particles);

// Сохранение результатов в CSV
int save_results_to_csv(SimulationResult* results, int count, const char* filename);

// Вычисление ошибки RMSE
double calculate_rmse(SimulationResult* results, int count);

#endif