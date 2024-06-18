g = 0
def main():
    global g
    # for _ in range(2):
    #     sys_fork()
    #     sys_write(f'g = {g}\n')
    #     g += 1
    sys_fork()
    sys_write(f'g = {g}\n')
    g += 1
    sys_fork()
    sys_write(f'g = {g}\n')
    g += 1


main()