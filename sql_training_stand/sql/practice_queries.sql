-- 1) Базовый SELECT с сортировкой
SELECT id, full_name, email
FROM students
ORDER BY full_name ASC;

-- 2) Фильтрация строк
SELECT id, title, level, price_usd
FROM courses
WHERE level = 'advanced'
ORDER BY price_usd DESC;

-- 3) JOIN: вывести заказы с именем студента и названием курса
SELECT o.id, s.full_name, c.title, o.total_usd, o.ordered_at
FROM orders o
JOIN students s ON s.id = o.student_id
JOIN courses c ON c.id = o.course_id
ORDER BY o.ordered_at DESC;

-- 4) GROUP BY: выручка по каждому курсу
SELECT c.title, COUNT(o.id) AS orders_count, ROUND(SUM(o.total_usd), 2) AS revenue_usd
FROM courses c
LEFT JOIN orders o ON o.course_id = c.id
GROUP BY c.id
ORDER BY revenue_usd DESC;

-- 5) HAVING: только курсы с 2+ заказами
SELECT c.title, COUNT(o.id) AS orders_count
FROM courses c
LEFT JOIN orders o ON o.course_id = c.id
GROUP BY c.id
HAVING COUNT(o.id) >= 2
ORDER BY orders_count DESC, c.title ASC;

-- 6) Пример транзакции (безопасный откат)
BEGIN TRANSACTION;
INSERT INTO orders (student_id, course_id, total_usd)
VALUES (1, 3, 70.00);
SELECT COUNT(*) AS total_orders_inside_tx FROM orders;
ROLLBACK;

-- 7) Идея параметризованного запроса:
-- SELECT * FROM students WHERE email = ?;
