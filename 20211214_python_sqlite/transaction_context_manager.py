import sqlite3

class SqliteTransaction:
    def __init__(self, connection):
        self.cursor = connection.cursor()

    def __enter__(self):
        self.cursor.execute("BEGIN;")
        return self.cursor

    def __exit__(self, exc_type, exc_value, traceback):
        if exc_type is None:
            self.cursor.execute("COMMIT;")
        else:
            self.cursor.execute("ROLLBACK;")


def add_product(user_id, product_name):
    with sqlite3.connect("databaze2.sqlite") as conn:
        with SqliteTransaction(conn) as cur:
            cur.execute("INSERT INTO products (name) VALUES (?)", (product_name, ))
            product_id = cur.lastrowid
            print(f"Added product with id {product_id}")
            cur.execute("INSERT INTO user_products (user_id, product_id) VALUES (?, ?)", (user_id, product_id))


def select_users():
    with sqlite3.connect("databaze2.sqlite") as conn:
        conn.cursor().close()
        cur_users = conn.cursor()

        cur_users.execute("SELECT * FROM users JOIN user_products USING(user_id) JOIN products USING(product_id) ORDER BY user_id;")
        for row in cur_users:
            print(row, type(row))

try:
    add_product(1, "test product")
except Exception as e:
    print(e)

select_users()
