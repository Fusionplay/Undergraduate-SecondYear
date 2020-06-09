# coding: utf8

import sys


class Point:
    def __init__(self, x, y):
        # coordinate of the point
        self.x = x
        self.y = y
        self.cost = sys.maxsize