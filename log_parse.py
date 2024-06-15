
with open('log.txt', 'r') as file:
    lines = file.readlines()
lines[-1] += '\n'


last_append = 0
for i in range(len(lines)):
    ix = i
    if lines[i].startswith('STACK_APPEND'):
        line = lines[i][len('STACK_APPEND('):]
        last_append = i
    elif lines[i].startswith('STACK_EDIT'):
        line = lines[i][len('STACK_EDIT('):]
        ix = last_append
    else:
        continue
    if line.startswith('UNDO'):
        undo = True
        line = line[5:]
    else:
        assert line.startswith('REDO')
        undo = False
        line = line[5:]
    parts = line.split(':', 1)
    parts = parts[1].split(' - ')
    word = parts[1].split(':', 1)[1][1:-1]
    parts[1] = parts[1].split(':', 1)[0]
    start = (int(parts[0].strip().split(',')[0][1:]), int(parts[0].strip().split(',')[1][:-1]))
    end = (int(parts[1].strip().split(',')[0][1:]), int(parts[1].strip().split(',')[1][:-1]))
    print(f'{undo}, {start}, {end}, {word}')
    lines[ix] = lines[i].replace('\n', '')



