# coding: utf8

import math
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
import random_map
import a_star
import keyboard_control


def get_ob_list():
    # 还需要做整数化处理：直接在这里做
    # [(左下边界，右上边界)]
    # 8.6晚：仍假设摄像头坐标原点为网格的中心, 网格大小为30 * 30， 中心坐标值为14，15

    thelist = [
        [-7, -14, -4, -4],
        [-11, 3, -5, 15],
        [9, 1, 15, 10],
    ]

    maxlimit = random_map.MAP_SIZE - 1
    middle = random_map.MAP_SIZE // 2 if random_map.MAP_SIZE % 2 else random_map.MAP_SIZE / 2 - 1

    for obs in thelist:
        # 坐标的范围是：x, y都是(0, 99)
        # 设摄像头坐标原点为 (49, 49)
        # 左下的x y都下取整，右上的都上取整
        obs[0] = int(math.floor(obs[0]) + middle)
        obs[1] = int(math.floor(obs[1]) + middle)
        obs[2] = int(math.ceil(obs[2]) + middle)
        obs[3] = int(math.ceil(obs[3]) + middle)

        if obs[0] < 0:
            obs[0] = 0
        if obs[1] < 0:
            obs[1] = 0
        if obs[2] > maxlimit:
            obs[2] = maxlimit
        if obs[3] > maxlimit:
            obs[3] = maxlimit


    return thelist


# ==================================================================================================================


plt.figure(figsize=(5, 5))

ob_list = get_ob_list()
themap = random_map.RandomMap(ob_list)

ax = plt.gca()
ax.set_xlim([0, themap.size])
ax.set_ylim([0, themap.size])

for i in range(themap.size):
    for j in range(themap.size):
        if themap.IsObstacle(i, j):
            rec = Rectangle((i, j), width=1, height=1, color='gray')
            ax.add_patch(rec)
        else:
            rec = Rectangle((i, j), width=1, height=1, edgecolor='gray', facecolor='w')
            ax.add_patch(rec)

rec = Rectangle((0, 0), width = 1, height = 1, facecolor='b')
ax.add_patch(rec)

rec = Rectangle((themap.size - 1, themap.size - 1), width = 1, height = 1, facecolor='r')
ax.add_patch(rec)
plt.axis('equal')
plt.axis('off')
plt.tight_layout()


# 路径规划及指令生成算法开始生效
# 目前还没有规划出倒序的指令
a_star = a_star.AStar(themap)

instr = a_star.RunAndSaveImage(ax, plt)
keyboard_control.KeysTyping.send_command(instr)


