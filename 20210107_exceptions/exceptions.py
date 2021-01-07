

def open_file(path: str) -> None:
    mu.lock()
    try:
        with open(path) as f:
            print(f.read())
        print("jen kdyz neni chyba")
    finally:
        print("always")
        mu.unlock()
