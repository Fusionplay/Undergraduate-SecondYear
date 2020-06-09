# coding: utf8

import sys
import time
import numpy as np
from matplotlib.patches import Rectangle

import point
import command
import random_map

path = []


class AStar:
    def __init__(self, themap):
        self.map = themap
        self.open_set = []
        self.close_set = []


# all 2 methods below : start point :(0, 0);  end point : (size - 1, size -1)
# using Euclidean distance
    def BaseCost(self, p): #cost from current point to the start point: p
        # p is the current point b
        # x_dis = p.x
        # y_dis = p.y
        # Distance to start point
        # return x_dis + y_dis + (np.sqrt(2) - 2) * min(x_dis, y_dis)
        return p.x + p.y

    def HeuristicCost(self, p):
        x_dis = self.map.size - 1 - p.x
        y_dis = self.map.size - 1 - p.y
        # Distance to end point
        # return x_dis + y_dis + (np.sqrt(2) - 2) * min(x_dis, y_dis)
        return x_dis + y_dis

    def TotalCost(self, p):
        # f(n)
        return self.BaseCost(p) + self.HeuristicCost(p)

    def IsValidPoint(self, x, y):
        # if point is obstacle or out of the range of map then invalid
        if x < 0 or y < 0:
            return False
        if x >= self.map.size or y >= self.map.size:
            return False
        return not self.map.IsObstacle(x, y)

    def IsInPointList(self, p, point_list):
        for point in point_list:
            if point.x == p.x and point.y == p.y:
                return True
        return False

    def IsInOpenList(self, p):
        return self.IsInPointList(p, self.open_set)

    def IsInCloseList(self, p):
        return self.IsInPointList(p, self.close_set)

    def IsStartPoint(self, p):
        return p.x == 0 and p.y == 0

    def IsEndPoint(self, p):
        return p.x == self.map.size-1 and p.y == self.map.size-1

    def SaveImage(self, plt):
        millis = int(round(time.time() * 1000))
        filename = './' + str(millis) + '.png'
        plt.savefig(filename)


    def ProcessPoint(self, x, y, parent):
        if not self.IsValidPoint(x, y):
            return # Do nothing for invalid point
        p = point.Point(x, y)
        if self.IsInCloseList(p):
            return # Do nothing for visited point
        # print('Process Point [', p.x, ',', p.y, ']', ', cost: ', p.cost)
        if not self.IsInOpenList(p):
            p.parent = parent
            p.cost = self.TotalCost(p)
            self.open_set.append(p)

    def SelectPointInOpenList(self):
        # select the point with the highest priority(least cost)
        index = 0
        selected_index = -1
        min_cost = sys.maxsize
        for p in self.open_set:
            cost = self.TotalCost(p)
            if cost < min_cost:
                min_cost = cost
                selected_index = index
            index += 1
        return selected_index


    def BuildPath(self, p, ax, plt, start_time):
        # build a path based on parental relationship
        # p is the last point of the path

        # the path list is the final output of this algorithm

        #path = []
        while True:
            path.insert(0, p) # Insert first
            if self.IsStartPoint(p):
                break
            else:
                p = p.parent

        # 这些输出不是必要的，因为Path已经得到了

        plot_time_st = time.time()
        for i in range(len(path)):
            rec = Rectangle((path[i].x, path[i].y), 1, 1, color='g')
            ax.add_patch(rec)
            plt.draw()
            if i == len(path) - 1:
                self.SaveImage(plt)
        plot_time_end = time.time()

        getins = command.Motion()
        instrs = getins.create_instr(path) # 获取指令


        end_time = time.time()
        print('===== Algorithm finish in', end_time - start_time, ' seconds')
        print('Plotting time: ', plot_time_end - plot_time_st, ' seconds')
        # print(path[0].x, path[0].y)
        return instrs


    def RunAndSaveImage(self, ax, plt):
        start_time = time.time()

        start_point = point.Point(0, 0)  #setting the start point to (0, 0)
        start_point.cost = 0
        self.open_set.append(start_point)

        while True:
            index = self.SelectPointInOpenList()
            if index < 0:
                print('No path found, algorithm failed!!!')
                return

            p = self.open_set[index]
            rec = Rectangle((p.x, p.y), 1, 1, color='c')
            ax.add_patch(rec)
            # self.SaveImage(plt)

            if self.IsEndPoint(p): # if is endpoint now then just build path
                return self.BuildPath(p, ax, plt, start_time)

            del self.open_set[index] # delete the current point in open_set
            self.close_set.append(p)

            # Process all neighbors
            x = p.x
            y = p.y
            # self.ProcessPoint(x-1, y+1, p)

            # self.ProcessPoint(x-1, y-1, p)
            # self.ProcessPoint(x, y-1, p) # down
            # self.ProcessPoint(x+1, y-1, p)
            self.ProcessPoint(x + 1, y, p) # right
            # self.ProcessPoint(x+1, y+1, p)
            self.ProcessPoint(x, y + 1, p) # up
            self.ProcessPoint(x - 1, y, p)  # left




