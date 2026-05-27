// Подключаем заголовочный файл с нашими функциями
#include "particle_filter.h"

// Стандартные библиотеки
#include <stdio.h>      // printf, fprintf
#include <stdlib.h>     // calloc, free
#include <math.h>       // fabs, sqrt
#include <string.h>     // strstr
#include <assert.h>     // assert (для отладки)
#include <windows.h>    // SetConsoleOutputCP для русского текста в Windows

// Допустимая погрешность при сравнении дробных чисел
#define EPSILON 1e-6

// Коды возврата для тестов: 0 = успех, 1 = провал
#define TEST_PASSED 0
#define TEST_FAILED 1

// счётчики результатов
// Статические переменные хранят значения между вызовами функций
static int tests_run = 0;     // сколько всего тестов запустили
static int tests_passed = 0;  // сколько прошло успешно
static int tests_failed = 0;  // сколько упало с ошибкой

// Макрос для запуска тестов
// Автоматизирует вывод "Запуск теста... PASSED/FAILED" и подсчёт статистики
// #test_func превращает имя функции в строку для вывода
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

// вспомогательная функция
// Сравнивает два дробных числа с учётом погрешности
// Возвращает TEST_PASSED если числа почти равны, иначе TEST_FAILED
int assert_close(double a, double b, double eps) {
    // Если разница меньше eps — считаем числа равными
    if (fabs(a - b) < eps) return TEST_PASSED;
    
    // Если не равны — выводим подробную ошибку в stderr
    fprintf(stderr, "\n  %.6f != %.6f (diff: %.6f)\n", a, b, fabs(a-b));
    return TEST_FAILED;
}

// Тест: проверка инициализации частиц
// Проверяет, что после init_particles все веса равны 1/N
int test_init_particles(void) {
    int N = 50;  // количество частиц для теста
    
    // Выделяем память под массив частиц
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;  // если память не выделилась — провал
    
    // Инициализируем частицы: среднее=0, разброс=1
    init_particles(p, N, 0.0, 1.0);
    
    // Проверяем каждую частицу: вес должен быть 1.0 / N
    for (int i = 0; i < N; i++) {
        if (assert_close(p[i].weight, 1.0/N, EPSILON) == TEST_FAILED) {
            free_particles(p);  // освобождаем память перед выходом
            return TEST_FAILED;
        }
    }
    
    free_particles(p);  // чистим память
    return TEST_PASSED; // все веса верны — тест пройден
}

// Тест: проверка нормализации весов
// После normalize_weights сумма всех весов должна быть ровно 1.0
int test_normalize_weights(void) {
    int N = 10;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    // Задаём произвольные "неправильные" веса: 0.5, 1.0, 1.5, ...
    for (int i = 0; i < N; i++) {
        p[i].x = i;  // положение не важно для этого теста
        p[i].weight = (i + 1) * 0.5;
    }
    
    // Вызываем нормализацию — она должна привести сумму к 1.0
    normalize_weights(p, N);
    
    // Считаем сумму всех весов после нормализации
    double sum = 0.0;
    for (int i = 0; i < N; i++) sum += p[i].weight;
    
    free_particles(p);
    
    // Проверяем: сумма должна быть 1.0 (с маленькой погрешностью)
    return assert_close(sum, 1.0, 1e-5);
}

// Тест: проверка обновления весов
// Частица, ближайшая к измерению, должна получить наибольший вес
int test_update_weights(void) {
    int N = 5;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    // Создаём частицы на позициях 0, 1, 2, 3, 4
    for (int i = 0; i < N; i++) {
        p[i].x = i * 1.0;
        p[i].weight = 1.0;  // начальные веса одинаковые
    }
    
    // Измерение датчика: z = 2.0
    // Частица с x=2.0 должна стать "самой правдоподобной"
    update_weights(p, N, 2.0, 1.0);
    
    // Находим индекс частицы с максимальным весом
    int max_idx = 0;
    for (int i = 1; i < N; i++)
        if (p[i].weight > p[max_idx].weight) max_idx = i;
    
    free_particles(p);
    
    // Ожидаем, что частица с индексом 2 (x=2.0) получит max вес
    return (max_idx == 2) ? TEST_PASSED : TEST_FAILED;
}

// Тест: проверка оценки положения (взвешенное среднее)
// Если все частицы в одной точке с равными весами, оценка = эта точка
int test_estimate_position(void) {
    int N = 4;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    // Все 4 частицы в точке 5.0 с весом 0.25
    for (int i = 0; i < N; i++) {
        p[i].x = 5.0;
        p[i].weight = 0.25;
    }
    
    // Вычисляем оценку
    double est = estimate_position(p, N);
    
    free_particles(p);
    
    // Ожидается: оценка = 5.0
    return assert_close(est, 5.0, EPSILON);
}

// Тест: проверка шага предсказания (predict)
// Частицы должны сдвинуться вперёд на скорость + небольшой шум
int test_predict(void) {
    int N = 10;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    // Изначально все частицы в нуле
    for (int i = 0; i < N; i++) {
        p[i].x = 0.0;
        p[i].weight = 1.0/N;
    }
    
    // Предсказываем: скорость=1.0, шум процесса маленький (0.01)
    predict(p, N, 1.0, 0.01);
    
    // Считаем среднее положение частиц после сдвига
    double sum = 0.0;
    for (int i = 0; i < N; i++) sum += p[i].x;
    
    free_particles(p);
    
    // Среднее должно быть около 1.0 (допускаем погрешность 0.5 из-за шума)
    return assert_close(sum/N, 1.0, 0.5);
}

// Тест: проверка перевыборки (resample)
// После resample сумма весов должна остаться 1.0
int test_resample(void) {
    int N = 50;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    // Инициализируем частицы
    init_particles(p, N, 0.0, 1.0);
    
    // Делаем веса неравномерными (чтобы resample было что "перевыбирать")
    for (int i = 0; i < N; i++)
        p[i].weight = (i + 1) * 0.1;
    
    normalize_weights(p, N);  // сначала нормализуем
    resample(p, N);           // затем перевыбираем
    
    // Проверяем: сумма весов после ресемплинга всё ещё 1.0
    double sum = 0.0;
    for (int i = 0; i < N; i++) sum += p[i].weight;
    
    free_particles(p);
    return assert_close(sum, 1.0, 1e-5);
}

// Тест: проверка сохранения в CSV
// Проверяет, что файл создаётся и содержит правильный заголовок
int test_save_csv(void) {
    int count = 5;
    // Выделяем массив результатов
    SimulationResult* res = calloc(count, sizeof(SimulationResult));
    if (!res) return TEST_FAILED;
    
    // Заполняем тестовыми данными
    for (int i = 0; i < count; i++) {
        res[i].step = i;
        res[i].true_pos = i;
        res[i].measurement = i + 0.1;
        res[i].estimate = i + 0.05;
    }
    
    // Пытаемся сохранить в файл
    int ret = save_results_to_csv(res, count, "output/test.csv");
    free(res);
    
    if (ret != 0) return TEST_FAILED;  // ошибка записи
    
    // Проверяем, что файл действительно создался
    FILE* f = fopen("output/test.csv", "r");
    if (!f) return TEST_FAILED;
    
    // Читаем первую строку (заголовок)
    char buf[256];
    fgets(buf, sizeof(buf), f);
    fclose(f);
    
    // Проверяем, что в заголовке есть нужные колонки
    return (strstr(buf, "step") && strstr(buf, "true_position")) ? 
           TEST_PASSED : TEST_FAILED;
}

// Тест: проверка расчёта ошибки RMSE
// Проверяет формулу: sqrt(среднее квадратов ошибок)
int test_rmse(void) {
    int count = 3;
    SimulationResult* res = calloc(count, sizeof(SimulationResult));
    if (!res) return TEST_FAILED;
    
    // Создаём данные с известными ошибками: 1, 2, 3
    // Ожидаемый RMSE = sqrt((1² + 2² + 3²) / 3) = sqrt(14/3) ≈ 2.16
    for (int i = 0; i < count; i++) {
        res[i].true_pos = 0.0;      // истина = 0
        res[i].estimate = i + 1;    // оценка = 1, 2, 3 → ошибки 1, 2, 3
    }
    
    double rmse = calculate_rmse(res, count);
    double expected = sqrt((1.0 + 4.0 + 9.0) / 3.0);
    free(res);
    
    // Сравниваем рассчитанный RMSE с ожидаемым
    return assert_close(rmse, expected, 0.01);
}

// Тест: полный цикл фильтра (интеграционный тест)
// Проверяет, что фильтр сходится к истине после нескольких шагов
int test_full_cycle(void) {
    int N = 50, steps = 20;
    Particle* p = allocate_particles(N);
    if (!p) return TEST_FAILED;
    
    // Инициализируем фильтр
    init_particles(p, N, 0.0, 5.0);
    
    double true_pos = 0.0;  // начинаем с нуля
    
    // Симулируем 20 шагов движения
    for (int step = 0; step < steps; step++) {
        // Объект движется вперёд на 1.0
        true_pos += 1.0;
        
        // Датчик измеряет с шумом
        double z = true_pos + rand_gaussian(0, 1.0);
        
        // Запускаем полный шаг фильтра
        predict(p, N, 1.0, 0.1);
        update_weights(p, N, z, 1.0);
        normalize_weights(p, N);
        resample(p, N);
        
        // Получаем оценку
        double est = estimate_position(p, N);
        
        // На последних 5 шагах фильтр должен уже "сходиться"
        // Проверяем: ошибка не больше 3.0 (допускаем большой разброс)
        if (step >= steps - 5 && fabs(est - true_pos) > 3.0) {
            free_particles(p);
            return TEST_FAILED;
        }
    }
    
    free_particles(p);
    return TEST_PASSED;
}

// Тест: проверка выделения/освобождения памяти
// Проверяет обработку ошибок и корректную работу allocate/free
int test_allocation(void) {
    // Проверяем: выделение 0 частиц должно вернуть NULL
    Particle* p1 = allocate_particles(0);
    if (p1 != NULL) { free_particles(p1); return TEST_FAILED; }
    
    // Проверяем: выделение отрицательного числа должно вернуть NULL
    Particle* p2 = allocate_particles(-5);
    if (p2 != NULL) { free_particles(p2); return TEST_FAILED; }
    
    // Проверяем: нормальное выделение работает
    Particle* p3 = allocate_particles(100);
    if (!p3) return TEST_FAILED;
    
    // Проверяем: можно записывать и читать из выделенной памяти
    p3[0].x = 42.0;
    p3[99].weight = 0.5;
    
    if (p3[0].x != 42.0 || p3[99].weight != 0.5) {
        free_particles(p3);
        return TEST_FAILED;
    }
    
    free_particles(p3);  // освобождаем
    return TEST_PASSED;
}

// Точка входа: запускает все тесты и выводит статистику
int main(void) {
    // Включаем поддержку русского текста в Windows-консоли
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif

    // Заголовок вывода
    printf("=== Тесты фильтра частиц ===\n\n");
    
    // Фиксированный seed для воспроизводимости тестов
    // (чтобы случайные числа были одинаковыми при каждом запуске)
    srand(42);
    
    // ЗАПУСК ВСЕХ ТЕСТОВ
    // Каждый RUN_TEST вызывает функцию и обновляет счётчики
    RUN_TEST(test_allocation);        // тест памяти
    RUN_TEST(test_init_particles);    // тест инициализации
    RUN_TEST(test_normalize_weights); // тест нормализации
    RUN_TEST(test_update_weights);    // тест обновления весов
    RUN_TEST(test_estimate_position); // тест оценки
    RUN_TEST(test_predict);           // тест предсказания
    RUN_TEST(test_resample);          // тест перевыборки
    RUN_TEST(test_save_csv);          // тест сохранения в файл
    RUN_TEST(test_rmse);              // тест расчёта ошибки
    RUN_TEST(test_full_cycle);        // тест полного цикла
    
    printf("\n=== Результаты ===\n");
    printf("Всего:  %d\n", tests_run);      // сколько тестов запустили
    printf("Passed: %d\n", tests_passed);   // сколько прошло
    printf("Failed: %d\n", tests_failed);   // сколько упало
    // Считаем процент успешных тестов
    printf("Процент: %.1f%%\n", (double)tests_passed/tests_run*100);
    
    // Возвращаем код: 0 = все тесты прошли, 1 = были провалы
    // (нужно для автоматических систем проверки)
    return (tests_failed == 0) ? 0 : 1;
}