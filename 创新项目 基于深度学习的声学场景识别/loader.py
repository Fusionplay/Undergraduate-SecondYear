import os
import numpy as np
from keras.utils import to_categorical
import librosa


# 本文件使用librosa库来提取指定目录下的音频文件的声学特征。特征可以人为指定为mfcc或chroma_cqt或两者都有

class FeaturesLoader:
    def __init__(self, data_dir, labels, n_classes=10, method='both', n_features=20):
        """Initialization"""
        self.data_dir = data_dir #数据目录
        self.n_classes = n_classes  # 类别的数目
        self.file_names = self.__get_file_names() #返回数据集目录下的所有wav文件列表

        self.labels = labels
        self.method = method  #使用的特征提取方法：mfcc, chroma_cqt, 或者都有
        self.n_features = n_features #特征数量

    def __get_file_names(self):
        """
        从数据目录中获取各个文件的名称, 返回数据集目录下的所有wav文件列表
        :return:
        """
        file_names = os.listdir(self.data_dir)
        # file_names = list(filter(lambda x: x.endswith('.wav'), file_names))
        file_names = list(filter(lambda x: x.endswith('.wav'), file_names))
        # 返回数据集目录下的所有wav文件列表

        return file_names

    def get_data(self):
        """获取数据"""
        list_x = []
        list_y = []
        print('Processing ' + str(len(self.file_names)) + ' audio files')  # 显示出处理的wav文件的数量
        for counter, file_name in enumerate(self.file_names):
            if counter % 100 == 0:
                print('Processed ' + str(counter) + ' audio files out of ' + str(len(self.file_names)))
            # It will convert to mono : 自动转换成转换成单声道
            data, fs = librosa.load(os.path.join(self.data_dir, file_name), sr=None)

            # 特征提取
            if self.method == 'mfcc':
                # x_i=librosa.feature.
                x_i = librosa.feature.mfcc(y=data, sr=fs, n_mfcc=self.n_features)
            elif self.method == 'chroma_cqt':
                x_i = librosa.feature.chroma_cqt(y=data, sr=fs, n_chroma=self.n_features)
            elif self.method == 'both':
                mfcc = librosa.feature.mfcc(y=data, sr=fs, n_mfcc=self.n_features)
                chroma = librosa.feature.chroma_cqt(y=data, sr=fs, n_chroma=self.n_features)
                x_i = np.concatenate((mfcc, chroma), axis=1) # 2种特征拼接起来
                #x_i是2维的: (n_features, t)
            else:
                raise Exception('Method not recognized')

            # class_name = file_name.split('-')[-1].split('.')[0]
            # print(file_name)
            ##
            strin = '-'
            f1 = file_name
            f2 = file_name
            class_name = f1.split('-')[0] + strin + f2.split('-')[1]
            # class_name=file_name[:3]
            print(class_name)

            y_i = self.labels.index(class_name)

            list_x.append(x_i)
            list_y.append(y_i)

        x = np.stack(list_x, axis=0) #拼成一个大的序列x, 维度为(训练样本数, n_features, t)
        x = x[:, :, :, np.newaxis] #尾部插入一个新维度，现在维度变为：(训练样本数, n_features, t, 1)
        y = np.asarray(list_y)

        return x, to_categorical(y, num_classes=self.n_classes) # y转为one-hot


class DemoLoader:
    def __init__(self, data_dir, labels, n_classes=10, method='both', n_features=20):
        """Initialization"""
        self.data_dir = data_dir
        self.n_classes = n_classes
        self.file_names = self.__get_file_names()
        # self.file_names=data_dir
        print('init file name :', self.file_names)
        self.labels = labels
        self.method = method
        self.n_features = n_features

    def __get_file_names(self):
        file_names = self.data_dir
        file_names = file_names.split('/')[-1]

        return file_names

    def get_data(self):
        list_x = []
        list_y = []
        print('filename:', self.file_names)
        data, fs = librosa.load(self.file_names, sr=None)

        if self.method == 'mfcc':
            x_i = librosa.feature.mfcc(y=data, sr=fs, n_mfcc=self.n_features)
        elif self.method == 'chroma_cqt':
            x_i = librosa.feature.chroma_cqt(y=data, sr=fs, n_chroma=self.n_features)
        elif self.method == 'both':
            mfcc = librosa.feature.mfcc(y=data, sr=fs, n_mfcc=self.n_features)
            chroma = librosa.feature.chroma_cqt(y=data, sr=fs, n_chroma=self.n_features)
            x_i = np.concatenate((mfcc, chroma), axis=1)
        else:
            raise Exception('Method not recognized')

        # class_name = file_name.split('-')[-1].split('.')[0]
        # print(file_name)
        ##
        strin = '-'
        f1 = self.file_names
        f2 = self.file_names
        class_name = f1.split('-')[0] + strin + f2.split('-')[1]
        # class_name=file_name[:3]
        print(class_name)

        y_i = self.labels.index(class_name)

        list_x.append(x_i)
        list_y.append(y_i)
        #
        x = np.stack(list_x, axis=0)
        x = x[:, :, :, np.newaxis]
        y = np.asarray(list_y)

        return x, to_categorical(y, num_classes=self.n_classes)
