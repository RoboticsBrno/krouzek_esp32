import sqlite3
import requests

def get_users():
    res = requests.get("https://reqres.in/api/users")
    return res.json()["data"]


def insert_users():
    users = get_users()

    with sqlite3.connect("databaze2.sqlite") as conn:
        # conn.execute() pusti jen jeden prikaz
        conn.executescript("""
            CREATE TABLE users (
                user_id INTEGER NOT NULL,
                email TEXT NOT NULL,
                first_name TEXT NOT NULL,
                last_name TEXT NOT NULL,
                PRIMARY KEY(user_id)
            );
            CREATE TABLE products (
                product_id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL
            );
            CREATE TABLE user_products (
                user_id INTEGER NOT NULL,
                product_id INTEGER NOT NULL,
                PRIMARY KEY(user_id, product_id)
            )
        """)

        conn.executemany("INSERT INTO users (user_id, email, first_name, last_name) VALUES (:id, :email, :first_name, :last_name)",
            users)

def add_product(user_id, product_name):
    with sqlite3.connect("databaze2.sqlite") as conn:
        cur = conn.cursor()
        cur.execute("BEGIN;")
        try:
            cur.execute("INSERT INTO products (name) VALUES (?)", (product_name, ))
            product_id = cur.lastrowid
            print(f"Added product with id {product_id}")
            cur.execute("INSERT INTO user_products (user_id, product_id) VALUES (?, ?)", (user_id, product_id))
        except Exception:
            cur.execute("ROLLBACK;")
            raise
        cur.execute("COMMIT;")
        cur.close()

def select_users():
    with sqlite3.connect("databaze2.sqlite") as conn:
        cur_users = conn.cursor()

        cur_users.execute("SELECT * FROM users LEFT OUTER JOIN user_products USING(user_id)" +
            "LEFT OUTER JOIN products USING(product_id) ORDER BY user_id;")
        for row in cur_users:
            print(row, type(row))

print(sqlite3.sqlite_version)
#insert_users()

add_product(1, "test product")
select_users()
