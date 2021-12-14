import sqlite3
import requests

print(sqlite3.sqlite_version)





def get_users():
    res = requests.get("https://reqres.in/api/users")
    return res.json()["data"]


def insert_users():
    users = get_users()

    #conn = sqlite3.connect("databaze.sqlite")
    #try:
    #    ...
    #    conn.execute("....")
    #    conn.commit() # !!!
    #except Exception:
    #    conn.rollback()
    #    raise
    #finally:
    #    conn.close()

    with sqlite3.connect("databaze.sqlite") as conn:

        conn.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='users';")
        conn.execute("""
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER NOT NULL,
                email TEXT NOT NULL,
                first_name TEXT NOT NULL,
                last_name TEXT NOT NULL,
                PRIMARY KEY(id)
            );
        """)


        for u in users:
            # conn.execute("INSERT INTO users (id, email, first_name, last_name) VALUES (?, ?, ?, ?)",
            #   [ u["id"], u["email"], u["first_name"], u["last_name"] ])
            conn.execute("INSERT INTO users (id, email, first_name, last_name) VALUES (:id, :email, :first_name, :last_name)", u)







def select_users():
    with sqlite3.connect("databaze.sqlite") as conn:
        cur = conn.cursor()
        cur.execute("SELECT id,email,first_name FROM users ORDER BY id;")

        for row in cur:
            print(row, type(row))
        
        for row in cur:
            print(row, type(row))

        cur.close()


#insert_users()
select_users()




























