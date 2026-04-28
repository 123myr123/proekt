from __future__ import annotations

import os
import sqlite3
from pathlib import Path


def get_db_path() -> str:
    return os.environ.get("DB_PATH", "/data/teach.db")


def connect_db(path: str) -> sqlite3.Connection:
    p = Path(path)
    if not p.parent.exists():
        raise FileNotFoundError(f"Каталог для БД не существует: {p.parent}")
    conn = sqlite3.connect(path)
    conn.row_factory = sqlite3.Row
    return conn


def print_section(title: str) -> None:
    print()
    print("=" * 60)
    print(title)
    print("=" * 60)


def run_select_all_authors(conn: sqlite3.Connection) -> None:
    print_section("1) SELECT: все строки таблицы authors")
    cur = conn.execute("SELECT id, name FROM authors ORDER BY id")
    rows = cur.fetchall()
    for row in rows:
        print(dict(row))


def run_join_books(conn: sqlite3.Connection) -> None:
    print_section("2) INNER JOIN: книги с именем автора")
    sql = """
    SELECT b.id, b.title, b.year, a.name AS author
    FROM books AS b
    INNER JOIN authors AS a ON a.id = b.author_id
    ORDER BY b.year
    """
    cur = conn.execute(sql)
    for row in cur.fetchall():
        print(dict(row))


def run_group_by(conn: sqlite3.Connection) -> None:
    print_section("3) GROUP BY + COUNT: сколько книг у каждого автора")
    sql = """
    SELECT a.name, COUNT(b.id) AS book_count
    FROM authors a
    LEFT JOIN books b ON b.author_id = a.id
    GROUP BY a.id, a.name
    ORDER BY book_count DESC, a.name
    """
    cur = conn.execute(sql)
    for row in cur.fetchall():
        print(dict(row))


def run_three_way_join(conn: sqlite3.Connection) -> None:
    print_section("4) JOIN трёх таблиц: студенты, курсы, оценки")
    sql = """
    SELECT s.full_name, c.code, c.title, e.grade
    FROM enrollments e
    JOIN students s ON s.id = e.student_id
    JOIN courses c ON c.id = e.course_id
    ORDER BY s.full_name, c.code
    """
    cur = conn.execute(sql)
    for row in cur.fetchall():
        print(dict(row))


def run_parameterized_insert(conn: sqlite3.Connection) -> None:
    print_section("5) Параметризованный INSERT + повторный SELECT")
    conn.execute(
        "INSERT INTO authors (name) VALUES (?)",
        ("Фёдор Достоевский",),
    )
    conn.commit()
    cur = conn.execute(
        "SELECT id, name FROM authors WHERE name = ?",
        ("Фёдор Достоевский",),
    )
    row = cur.fetchone()
    print("Добавлена строка:", dict(row) if row else None)


def run_transaction_rollback_demo(conn: sqlite3.Connection) -> None:
    print_section("6) Транзакция: INSERT откатывается (демонстрация)")
    try:
        conn.execute("BEGIN")
        conn.execute(
            "INSERT INTO books (title, year, author_id) VALUES (?, ?, ?)",
            ("Преступление и наказание", 1866, 4),
        )
        conn.rollback()
    except sqlite3.Error as e:
        conn.rollback()
        print("Ошибка SQL:", e)
    cur = conn.execute("SELECT COUNT(*) AS n FROM books WHERE title LIKE '%Преступление%'")
    print("Строк после ROLLBACK (должно быть 0):", cur.fetchone()["n"])


def main() -> None:
    path = get_db_path()
    print(f"Файл базы данных: {path}")
    conn = connect_db(path)
    try:
        run_select_all_authors(conn)
        run_join_books(conn)
        run_group_by(conn)
        run_three_way_join(conn)
        run_parameterized_insert(conn)
        run_transaction_rollback_demo(conn)
    finally:
        conn.close()
    print()
    print("Демонстрация завершена. Для ручного SQL: sqlite3", path)
    print("Примеры запросов: /app/sql/demo_queries.sql")
    print()


if __name__ == "__main__":
    main()
