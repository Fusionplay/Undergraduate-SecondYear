# coding: utf8

#移动指令
move_time = '1500'
turn_time = '1500'
ahead = 'w ' + move_time
back = 'x ' + move_time
turn_left = 'a ' + turn_time
turn_right = 'd ' + turn_time

#抓取指令
#
take = 'take'


class Motion:
    def __init__(self):
        # x y是当前的坐标
        # self.x = 0
        # self.y = 0
        self.direction = 0      # 当前的朝向：默认是向上
        self.instr = []         # 最终的指令数组
        # 0:up 1:right 2:down 3:left


    # 根据path来创建移动指令, 并写入文件instr.txt中
    def create_instr(self, path):
        # file = open('E:/Python/files/path_managing/command.txt', 'w')

        # path是算法生成的路径数组。路径以坐标的形式表示
        for i in range(1, len(path)):
            # up
            if self.direction == 0:
                if path[i].x - path[i - 1].x == 1:
                    # file.write('右转 + 前进\n')
                    self.instr.append(turn_right)
                    self.instr.append(ahead)
                    self.direction = 1

                elif path[i].x - path[i - 1].x == -1:
                    # file.write('左转 + 前进\n')
                    self.instr.append(turn_left)
                    self.instr.append(ahead)
                    self.direction = 3

                elif path[i].y - path[i - 1].y == 1:
                    # file.write('前进!\n')
                    self.instr.append(ahead)
                    # path[i].direction = 0


            # right
            elif self.direction == 1:
                if path[i].x - path[i - 1].x == 1:
                    # file.write('前进!\n')
                    self.instr.append(ahead)
                    # path[i].direction = 1

                elif path[i].y - path[i - 1].y == 1:
                    # file.write('左转 + 前进\n')
                    self.instr.append(turn_left)
                    self.instr.append(ahead)
                    self.direction = 0

                elif path[i].y - path[i - 1].y == -1:
                    # file.write('右转 + 前进\n')
                    self.instr.append(turn_right)
                    self.instr.append(ahead)
                    self.direction = 2


            # down
            elif self.direction == 2:
                if path[i].x - path[i - 1].x == 1:
                    file.write('左转命令' + ' ' + '前进命令' + '\n')
                    path[i].direction = 1

                elif path[i].x - path[i - 1].x == -1:
                    file.write('右转命令' + ' ' + '前进命令' + '\n')
                    path[i].direction = 3

                elif path[i].y - path[i - 1].y == -1:
                    file.write('前进命令' + '\n')
                    path[i].direction = 2


            # left
            elif self.direction == 3:
                if path[i].x - path[i - 1].x == -1:
                    # file.write('前进命令' + '\n')
                    self.instr.append(ahead)
                    # path[i].direction = 3

                elif path[i].y - path[i - 1].y == 1:
                    # file.write('右转命令' + ' ' + '前进命令' + '\n')
                    self.instr.append(turn_right)
                    self.instr.append(ahead)
                    self.direction = 0

                elif path[i].y - path[i - 1].y == -1:
                    # file.write('左转命令'+' '+'前进命令' + '\n')
                    self.instr.append(turn_left)
                    self.instr.append(ahead)
                    self.direction = 2


        #合并路径


        #生成逆向的路径
        back_instr = self.instr[::-1]
        for i in range(len(back_instr)):
            if back_instr[i][0] == 'd':
                back_instr[i] = 'a' + back_instr[i][1:]
            elif back_instr[i][0] == 'a':
                back_instr[i] = 'd' + back_instr[i][1:]

        self.instr.append(take)
        self.instr.append(turn_left)
        self.instr.append(turn_left)
        self.instr += back_instr

        #把指令写入instr.txt中
        with open('instr.txt', 'w') as f:
            for ins in self.instr:
                f.write(ins + '\n')

            # f.write(take + '\n')
            # f.write(turn_left + '\n')
            # f.write(turn_left + '\n')
            #
            # for ins in back_instr:
            #     f.write(ins + '\n')


        return self.instr