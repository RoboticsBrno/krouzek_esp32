import sys

print(type(sys.argv[1]))
print(len(sys.argv[1]), sys.argv[1])


with open("text-utf8.txt", "r") as f:
    v = f.read()
    print(type(v))
    print(len(v), v)

with open("text-utf16.txt", "r", encoding="utf-16") as f:
    v = f.read()
    print(type(v))
    print(len(v), v)
