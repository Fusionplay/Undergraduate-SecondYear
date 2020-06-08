#pragma once
#include "common.h"

#define hashsize 8000
#define lensize 125 //100 0000 / 8000 = 125
#define sortsize 120000
#define setsize 120000
#define bucsize 10240  //ELFhash的桶数


//class MyDatabase;
int ELFhash(char *str);



class Singlebase  //用散列来实现！
{
	//散列函数：hash(x) = x / lensize

public:
	chainnode* dat[hashsize];   //散列数组
	int valnum;  //每行中属性的数量

	Singlebase() { valnum = 0; memset(dat, NULL, hashsize * sizeof(chainnode*)); }
	//~Singlebase();

	void output(chainnode* out);

	//单点指令
	void insert(int id, char* name, int *start);  //插入一行数据
	void del(int id);  //删除某个id的数据
	void set(int id, int key, int val);  //设置某行数据的值
	void add(int id, int key, int val);  //增加某行的某个数据的值
	void find_id(int id);  //按ID查找
	void find_name(char *name);  //按名字查找
	void find_keyreange(int key, string op, int val);  //按key查找:范围


	//区间指令
	void del_ran(int idl, int idr);  //删除某个id区间的数据
	void set_ran(int idl, int idr, int key, int val);  //设置某个范围的数据的值
	void add_ran(int idl, int idr, int key, int val);  //增加某个范围的数据的值
	void find_idran(int idl, int idr, int key, string op, int val);  //按key查找:在一定的ID范围内
	void sum_idran(int idl, int idr, int key);         //区间和

	//混合专有指令
	void find_kthkey(int key, int k, char *mode);
	void find_kthkey_ran(int idl, int idr, int key, int k, char *mode);
	void find_kthkeylist(int key, int k, char *mode);
	void find_kthkey_ran_list(int idl, int idr, int key, int k, char *mode);
};



class setbase
{
public:
	chainnode* dat[bucsize];   //字符串散列数组
	chainset* listhead;  //链表表头
	int valnum;
	char name[30]; //set name

	setbase() { valnum = 0; memset(dat, NULL, bucsize * sizeof(chainnode*)); listhead = NULL; }
	inline bool name_exist(int namehash, char *name);
	//inline void output(chainnode* out);
};


void Union(setbase *table1, setbase *table2);
void inter(setbase *table1, setbase *table2);