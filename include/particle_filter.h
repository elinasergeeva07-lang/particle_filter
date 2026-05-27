#ifndef PARTICLE_FILTER_H
#define PARTICLE_FILTER_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Структура частицы
 * 
 * Представляет одну гипотезу о положении объекта в одномерном пространстве.
 */
typedef struct {
    double x;      ///< Положение частицы
    double weight; ///< Вес частицы (правдоподобие)
} Particle;

/**
 * @brief Структура для хранения результатов симуляции
 */
typedef struct {
    int step;           ///< Номер шага
    double true_pos;    ///< Истинное положение
    double measurement; ///< Измерение с шумом
    double estimate;    ///< Оценка фильтра
} SimulationResult;

/* ==========================================================================
   Основные функции фильтра частиц (требования из задания)
   ========================================================================== */

/**
 * @brief Инициализация частиц из априорного распределения
 * 
 * @param particles Массив частиц для инициализации
 * @param N Количество частиц
 * @param mean Среднее значение начального распределения
 * @param std_dev Стандартное отклонение начального распределения
 */
void init_particles(Particle* particles, int N, double mean, double std_dev);

/**
 * @brief Шаг предсказания (добавление шума процесса)
 * 
 * @param particles Массив частиц
 * @param N Количество частиц
 * @param velocity Скорость движения объекта
 * @param Q Дисперсия шума процесса
 */
void predict(Particle* particles, int N, double velocity, double Q);

/**
 * @brief Вычисление весов частиц по правдоподобию
 * 
 * Использует гауссову функцию правдоподобия:
 * w = exp(-0.5 * (z - x)^2 / R)
 * 
 * @param particles Массив частиц
 * @param N Количество частиц
 * @param z Измерение с датчика
 * @param R Дисперсия шума датчика
 */
void update_weights(Particle* particles, int N, double z, double R);

/**
 * @brief Нормализация весов частиц
 * 
 * Приводит сумму весов к единице.
 * 
 * @param particles Массив частиц
 * @param N Количество частиц
 */
void normalize_weights(Particle* particles, int N);

/**
 * @brief Систематическая перевыборка частиц
 * 
 * Заменяет частицы с малыми весами на копии частиц с большими весами.
 * Использует систематический ресемплинг.
 * 
 * @param particles Массив частиц
 * @param N Количество частиц
 */
void resample(Particle* particles, int N);

/**
 * @brief Оценка положения объекта (взвешенное среднее)
 * 
 * @param particles Массив частиц
 * @param N Количество частиц
 * @return double Оценка положения
 */
double estimate_position(Particle* particles, int N);

/* ==========================================================================
   Вспомогательные функции
   ========================================================================== */

/**
 * @brief Генерация случайного числа из гауссова распределения
 * 
 * Использует преобразование Бокса-Мюллера.
 * 
 * @param mean Среднее значение
 * @param std_dev Стандартное отклонение
 * @return double Случайное число
 */
double rand_gaussian(double mean, double std_dev);

/**
 * @brief Генерация случайного числа из равномерного распределения [0, 1)
 * 
 * @return double Случайное число
 */
double rand_uniform(void);

/**
 * @brief Вычисление гауссовой функции правдоподобия
 * 
 * @param x Положение
 * @param mean Среднее значение (измерение)
 * @param var Дисперсия
 * @return double Значение функции правдоподобия
 */
double gaussian_likelihood(double x, double mean, double var);

/**
 * @brief Выделение памяти под массив частиц
 * 
 * @param N Количество частиц
 * @return Particle* Указатель на выделенную память или NULL при ошибке
 */
Particle* allocate_particles(int N);

/**
 * @brief Освобождение памяти массива частиц
 * 
 * @param particles Массив частиц
 */
void free_particles(Particle* particles);

/**
 * @brief Сохранение результатов симуляции в CSV файл
 * 
 * @param results Массив результатов
 * @param count Количество записей
 * @param filename Имя файла
 * @return int 0 при успехе, -1 при ошибке
 */
int save_results_to_csv(SimulationResult* results, int count, const char* filename);

/**
 * @brief Вычисление среднеквадратичной ошибки (RMSE)
 * 
 * @param results Массив результатов
 * @param count Количество записей
 * @return double RMSE
 */
double calculate_rmse(SimulationResult* results, int count);

#endif /* PARTICLE_FILTER_H */