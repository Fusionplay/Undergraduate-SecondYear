# coding: utf8

import point

MAP_SIZE = 30


class RandomMap:
    def __init__(self, thelist, size=MAP_SIZE):
        #8.6晚：初步：1.5m * 1.5m, 30 * 30

        self.size = size
        self.obstacle_point = []
        # self.obstacle = size // 4 # number of obstacles
        self.GenerateObstacle(thelist)


    def IsObstacle(self, i, j):
        for p in self.obstacle_point:
            if i == p.x and j == p.y:
                return True
        return False


    def GenerateObstacle(self, thelist):

        for obs in thelist:
            print(obs[0], obs[1], obs[2], obs[3], '\n')
            for i in range(obs[0], obs[2]+1):
                for j in range(obs[1], obs[3]+1):
                    self.obstacle_point.append(point.Point(i, j))



        #    self.obstacle_point = []
        # self.obstacle_point.append(point.Point(self.size // 2, self.size // 2))      # (50, 50)
        # self.obstacle_point.append(point.Point(self.size // 2, self.size // 2 - 1))  # (50, 49)
        #
        #
        # # Generate an obstacle in the middle
        # for i in range(self.size // 2 - 4, self.size // 2):  # (i = 46 -> 49)
        #     self.obstacle_point.append(point.Point(i, self.size - i))      # (46, 54)
        #     self.obstacle_point.append(point.Point(i, self.size - i - 1))  # (46, 53)
        #     self.obstacle_point.append(point.Point(self.size - i, i))      # (54, 46)
        #     self.obstacle_point.append(point.Point(self.size - i, i - 1))  # (54, 45)
        #
        # for i in range(self.obstacle-1):
        #     x = np.random.randint(0, self.size)
        #     y = np.random.randint(0, self.size)
        #     self.obstacle_point.append(point.Point(x, y))
        #
        #     if (np.random.rand() > 0.5): # Random boolean
        #         for l in range(self.size//4):
        #             self.obstacle_point.append(point.Point(x, y+l))
        #             pass
        #     else:
        #         for l in range(self.size//4):
        #             self.obstacle_point.append(point.Point(x+l, y))
        #             pass
        #
        #
        # for i in range(5):
        #     if not self.IsObstacle(10, i):
        #         self.obstacle_point.append(point.Point(10, i))
        #
        #
        # for i in range(5):
        #     if not self.IsObstacle(10, i):
        #         self.obstacle_point.append(point.Point(10, i))
