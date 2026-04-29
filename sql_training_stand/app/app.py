import os
import sqlite3
from flask import Flask, jsonify, render_template

DB_PATH = os.environ.get("DB_PATH", "/workspace/data/training.db")

app = Flask(__name__)


def query_all(sql: str, params=()):
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    rows = conn.execute(sql, params).fetchall()
    conn.close()
    return [dict(r) for r in rows]


@app.route("/")
def index():
    recent_orders = query_all(
        """
        SELECT o.id, s.full_name AS student, c.title AS course, o.total_usd, o.ordered_at
        FROM orders o
        JOIN students s ON s.id = o.student_id
        JOIN courses c ON c.id = o.course_id
        ORDER BY o.ordered_at DESC
        LIMIT 12
        """
    )
    best_courses = query_all(
        """
        SELECT c.title, COUNT(o.id) AS orders_count, ROUND(SUM(o.total_usd), 2) AS revenue_usd
        FROM courses c
        LEFT JOIN orders o ON o.course_id = c.id
        GROUP BY c.id
        ORDER BY revenue_usd DESC, orders_count DESC
        LIMIT 5
        """
    )
    return render_template("index.html", recent_orders=recent_orders, best_courses=best_courses)


@app.route("/api/summary")
def api_summary():
    totals = query_all(
        """
        SELECT
          (SELECT COUNT(*) FROM students) AS students_count,
          (SELECT COUNT(*) FROM courses) AS courses_count,
          (SELECT COUNT(*) FROM orders) AS orders_count,
          (SELECT ROUND(COALESCE(SUM(total_usd), 0), 2) FROM orders) AS total_revenue_usd
        """
    )[0]
    return jsonify(totals)


@app.route("/api/recent-orders")
def api_recent_orders():
    rows = query_all(
        """
        SELECT o.id, s.full_name AS student, c.title AS course, o.total_usd, o.ordered_at
        FROM orders o
        JOIN students s ON s.id = o.student_id
        JOIN courses c ON c.id = o.course_id
        ORDER BY o.ordered_at DESC
        LIMIT 30
        """
    )
    return jsonify(rows)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=int(os.environ.get("PORT", "8090")))
