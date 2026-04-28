.headers on
.mode column

SELECT '--- Все авторы ---' AS info;
SELECT id, name FROM authors ORDER BY id;

SELECT '--- Книги с именем автора (INNER JOIN) ---' AS info;
SELECT b.id, b.title, b.year, a.name AS author
FROM books b
INNER JOIN authors a ON a.id = b.author_id
ORDER BY b.year;

SELECT '--- Число книг по авторам (GROUP BY) ---' AS info;
SELECT a.name, COUNT(b.id) AS book_count
FROM authors a
LEFT JOIN books b ON b.author_id = a.id
GROUP BY a.id, a.name
ORDER BY book_count DESC;

SELECT '--- Студенты и курсы (JOIN трёх таблиц) ---' AS info;
SELECT s.full_name, c.code, c.title, e.grade
FROM enrollments e
JOIN students s ON s.id = e.student_id
JOIN courses c ON c.id = e.course_id
ORDER BY s.full_name, c.code;
