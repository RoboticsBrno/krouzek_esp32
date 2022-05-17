Import("env")

flags = env.get("UPLOADERFLAGS", [])
i = 0
while i < len(flags):
    if flags[i] == "--before" or flags[i] == "--after":
        flags.pop(i)
        flags.pop(i)
    else:
        i += 1


env.Replace(UPLOADERFLAGS=flags)
