PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS authors (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS books (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    year INTEGER NOT NULL CHECK (year BETWEEN 1000 AND 3000),
    author_id INTEGER NOT NULL,
    FOREIGN KEY (author_id) REFERENCES authors (id)
);

CREATE INDEX IF NOT EXISTS idx_books_author ON books (author_id);

CREATE TABLE IF NOT EXISTS students (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    full_name TEXT NOT NULL,
    group_code TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS courses (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    code TEXT NOT NULL UNIQUE,
    title TEXT NOT NULL,
    credits INTEGER NOT NULL CHECK (credits > 0)
);

CREATE TABLE IF NOT EXISTS enrollments (
    student_id INTEGER NOT NULL,
    course_id INTEGER NOT NULL,
    grade INTEGER CHECK (grade IS NULL OR (grade >= 2 AND grade <= 5)),
    PRIMARY KEY (student_id, course_id),
    FOREIGN KEY (student_id) REFERENCES students (id),
    FOREIGN KEY (course_id) REFERENCES courses (id)
);

INSERT INTO authors (name) VALUES
    ('Лев Толстой'),
    ('Антон Чехов'),
    ('Александр Пушкин');

INSERT INTO books (title, year, author_id) VALUES
    ('Война и мир', 1869, 1),
    ('Анна Каренина', 1877, 1),
    ('Вишнёвый сад', 1904, 2),
    ('Палата № 6', 1892, 2),
    ('Капитанская дочка', 1836, 3);

INSERT INTO students (full_name, group_code) VALUES
    ('Иван Петров', 'ИТ-21'),
    ('Мария Сидорова', 'ИТ-21'),
    ('Пётр Козлов', 'ИТ-22');

INSERT INTO courses (code, title, credits) VALUES
    ('CS101', 'Основы программирования', 4),
    ('DB200', 'Базы данных', 5),
    ('NET150', 'Сети и протоколы', 3);

INSERT INTO enrollments (student_id, course_id, grade) VALUES
    (1, 1, 5),
    (1, 2, 4),
    (2, 1, 5),
    (2, 3, NULL),
    (3, 2, 3);
