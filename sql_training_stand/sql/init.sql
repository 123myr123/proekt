PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS students (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  full_name TEXT NOT NULL,
  email TEXT NOT NULL UNIQUE,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS courses (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  title TEXT NOT NULL,
  level TEXT NOT NULL CHECK (level IN ('beginner', 'intermediate', 'advanced')),
  price_usd REAL NOT NULL CHECK (price_usd >= 0)
);

CREATE TABLE IF NOT EXISTS orders (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  student_id INTEGER NOT NULL,
  course_id INTEGER NOT NULL,
  total_usd REAL NOT NULL CHECK (total_usd >= 0),
  ordered_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (student_id) REFERENCES students(id),
  FOREIGN KEY (course_id) REFERENCES courses(id)
);

CREATE INDEX IF NOT EXISTS idx_orders_student_id ON orders(student_id);
CREATE INDEX IF NOT EXISTS idx_orders_course_id ON orders(course_id);
CREATE INDEX IF NOT EXISTS idx_orders_ordered_at ON orders(ordered_at);

INSERT INTO students (full_name, email) VALUES
('Alice Brown', 'alice@example.com'),
('Bob Smith', 'bob@example.com'),
('Carol Davis', 'carol@example.com'),
('Daniel Green', 'daniel@example.com'),
('Eva White', 'eva@example.com');

INSERT INTO courses (title, level, price_usd) VALUES
('SQL Basics', 'beginner', 40.00),
('SQL Joins in Practice', 'intermediate', 55.00),
('Transactions and Isolation', 'advanced', 70.00),
('Data Modeling 101', 'intermediate', 50.00),
('Analytics with SQL', 'advanced', 80.00);

INSERT INTO orders (student_id, course_id, total_usd, ordered_at) VALUES
(1, 1, 40.00, '2026-04-01 10:10:00'),
(2, 2, 55.00, '2026-04-02 11:20:00'),
(1, 4, 50.00, '2026-04-03 13:00:00'),
(3, 2, 55.00, '2026-04-04 09:30:00'),
(4, 3, 70.00, '2026-04-05 15:05:00'),
(5, 5, 80.00, '2026-04-05 16:40:00'),
(3, 1, 40.00, '2026-04-06 12:00:00'),
(2, 5, 80.00, '2026-04-06 14:00:00'),
(5, 2, 55.00, '2026-04-07 09:15:00'),
(4, 4, 50.00, '2026-04-08 17:45:00');
