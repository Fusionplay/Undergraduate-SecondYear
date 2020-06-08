#include "database.h"

extern FILE *singleout;   //单点
extern FILE *section1out; //区间
extern FILE *section2out; //区间2
extern FILE *setout;      //集合
extern int TEST_TYPE;


int ELFhash(char *str)
{
	unsigned int hash = 0;
	unsigned int x = 0;
	while (*str)
	{
		hash = (hash << 4) + *str;  
		if ((x = hash & 0xF0000000) != 0) 
		{
			hash ^= (x >> 24); 
			hash &= ~x; 
		}
		str++;  
	}

	return (hash & 0x7fffffff) % bucsize;    //6 
}


inline bool setbase::name_exist(int namehash, char *name)
{
	chainnode *tr = dat[namehash];
	while (tr)
	{
		if (strcmp(tr->row.name, name) == 0)
			return true;

		tr = tr->link;
	}
	
	return false;
}


inline bool larger(chainnode *left, chainnode *right, int key)  //upordown = 0
{
	if (left->row.values[key] > right->row.values[key])
		return true;

	else if (left->row.values[key] == right->row.values[key] && left->row.id > right->row.id)
		return true;

	return false;
}


inline bool largeroreq(chainnode *left, chainnode *right, int key) //upordown = 1
{
	if (left->row.values[key] > right->row.values[key])
		return true;

	else if (left->row.values[key] == right->row.values[key] && left->row.id <= right->row.id) //ID大的是小的
		return true;

	return false;
}


inline bool lower(chainnode *left, chainnode *right, int key) //upordown = 1
{
	if (left->row.values[key] < right->row.values[key])
		return true;

	else if (left->row.values[key] == right->row.values[key] && left->row.id > right->row.id) //ID大的是小的
		return true;

	return false;
}


inline bool loweroreq(chainnode *left, chainnode *right, int key)  //upordown = 0
{
	if (left->row.values[key] < right->row.values[key])
		return true;

	else if (left->row.values[key] == right->row.values[key] && left->row.id <= right->row.id)
		return true;

	return false;
}


void sift_down(chainnode **heap, int start, int end, int key, bool upordown)
{
	//upordown==1: ASC, 第k大，最大堆； ==0：第k小，最小堆

	int i = start;
	int j = 2 * i + 1;

	chainnode *temp = heap[i];

	if (upordown)
	{
		while (j <= end)
		{
			if (j < end && lower(heap[j], heap[j + 1], key))
				j++;

			if (largeroreq(temp, heap[j], key))
				break;

			else
			{
				heap[i] = heap[j];
				i = j;
				j = 2 * j + 1;
			}
		}
	}


	else
	{
		while (j <= end)
		{
			if (j < end && larger(heap[j], heap[j + 1], key))
				j++;

			if (loweroreq(temp, heap[j], key))
				break;

			else
			{
				heap[i] = heap[j];
				i = j;
				j = 2 * j + 1;
			}
		}
	}
	

	heap[i] = temp;
}


  

inline void Singlebase::output(chainnode *out)
{
	switch (TEST_TYPE)
	{
	case 0:
		fprintf(singleout, "%d %s", out->row.id, out->row.name);
		for (int i = 0; i < valnum; i++)
			fprintf(singleout, " %d", out->row.values[i]);

		fputc('\n', singleout);

		break;


	case 1:
		fprintf(section1out, "%d %s", out->row.id, out->row.name);
		for (int i = 0; i < valnum; i++)
			fprintf(section1out, " %d", out->row.values[i]);

		fputc('\n', section1out);

		break;


	case 2:
		fprintf(section2out, "%d %s", out->row.id, out->row.name);
		for (int i = 0; i < valnum; i++)
			fprintf(section2out, " %d", out->row.values[i]);

		fputc('\n', section2out);

		break;
	}
}


void Singlebase::insert(int id, char *name, int *start)
{
	int IDhash = id / lensize;


	if (dat[IDhash] == NULL) //该位置之前没有 现在插入
	{
		dat[IDhash] = new chainnode;
		dat[IDhash]->row.id = id;  //设置ID
		strcpy(dat[IDhash]->row.name, name);

		int *st = dat[IDhash]->row.values;
		for (int i = 0; i < valnum; i++)
			st[i] = start[i];
	}


	else //该位置之前有数据
	{
		chainnode *tr = dat[IDhash];

		chainnode *pre = NULL;
		while (tr != NULL && tr->row.id <= id)
		{
			pre = tr;
			tr = tr->link;
		}

		if (pre != NULL)
		{
			pre->link = new chainnode;
			pre = pre->link;

			pre->row.id = id;
			strcpy(pre->row.name, name);

			int *st = pre->row.values;
			for (int i = 0; i < valnum; i++)
				st[i] = start[i];

			pre->link = tr;
		}

		else
		{
			dat[IDhash] = new chainnode;
			dat[IDhash]->row.id = id;  //设置ID
			strcpy(dat[IDhash]->row.name, name);
			int *st = dat[IDhash]->row.values;
			for (int i = 0; i < valnum; i++)
				st[i] = start[i];

			dat[IDhash]->link = tr;
		}
	}
}


void Singlebase::del(int id)
{
	int IDhash = id / lensize;
	chainnode *p;
	chainnode *tr;
	tr = dat[IDhash];
	if (tr->row.id == id) //首个就是
	{
		if (tr->link != NULL)
			dat[IDhash] = dat[IDhash]->link;
		else
			dat[IDhash] = NULL;

		delete tr;
	}
	else
	{
		while (tr->link->row.id != id)
			tr = tr->link;

		p = tr->link;
		tr->link = p->link;
		delete p;
	}
}


void Singlebase::set(int id, int key, int val)
{
	int IDhash = id / lensize;
	chainnode *tr = dat[IDhash];

	while (tr->row.id != id)
		tr = tr->link;

	tr->row.values[key] = val;
}


void Singlebase::add(int id, int key, int val)
{
	int IDhash = id / lensize;
	chainnode *tr = dat[IDhash];

	while (tr->row.id != id)
		tr = tr->link;

	tr->row.values[key] += val;
}


void Singlebase::find_id(int id)
{
	int IDhash = id / lensize;
	chainnode *tr = dat[IDhash];

	while (tr->row.id != id)
		tr = tr->link;

	output(tr);
}


void Singlebase::find_name(char *name)
{
	for (int i = 0; i < hashsize; i++)
	{
		chainnode *tr = dat[i];
		while (tr != NULL)
		{
			if (strcmp(tr->row.name, name) == 0)
				output(tr);

			tr = tr->link;
		}
	}
}


void Singlebase::find_keyreange(int key, string op, int val)
{
	if (op == ">")
	{
		for (int i = 0; i < hashsize; i++)
		{
			chainnode *tr = dat[i];
			while (tr != NULL)
			{
				if (tr->row.values[key] > val)
					output(tr);

				tr = tr->link;
			}
		}
	}

	
	else if (op == "<")
	{
		for (int i = 0; i < hashsize; i++)
		{
			chainnode *tr = dat[i];
			while (tr != NULL)
			{
				if (tr->row.values[key] < val)
					output(tr);

				tr = tr->link;
			}
		}
	}


	else if (op == "<=")
	{
		for (int i = 0; i < hashsize; i++)
		{
			chainnode *tr = dat[i];
			while (tr != NULL)
			{
				if (tr->row.values[key] <= val)
					output(tr);

				tr = tr->link;
			}
		}
	}


	else if (op == ">=")
	{
		for (int i = 0; i < hashsize; i++)
		{
			chainnode *tr = dat[i];
			while (tr != NULL)
			{
				if (tr->row.values[key] >= val)
					output(tr);

				tr = tr->link;
			}
		}
	}


	else if (op == "=")
	{
		for (int i = 0; i < hashsize; i++)
		{
			chainnode *tr = dat[i];
			while (tr != NULL)
			{
				if (tr->row.values[key] == val)
					output(tr);

				tr = tr->link;
			}
		}
	}


	else if (op == "!=")
	{
		for (int i = 0; i < hashsize; i++)
		{
			chainnode *tr = dat[i];
			while (tr != NULL)
			{
				if (tr->row.values[key] != val)
					output(tr);

				tr = tr->link;
			}
		}
	}
}


/*===============================================================range===================================================*/


void Singlebase::del_ran(int idl, int idr)
{
	int IDhashl = idl / lensize;
	int IDhashr = idr / lensize;

	chainnode *pre;
	chainnode *tr;
	chainnode *deltmp;
	int flag;

	
	for (int i = IDhashl; i <= IDhashr; i++)
	{
		if (dat[i])
		{
			flag = 1;

			pre = tr = dat[i];
			while (tr && tr->row.id < idl)
			{
				pre = tr;
				tr = tr->link;
			}

			if (tr == dat[i])
				flag = 0;

			while (tr != NULL && tr->row.id <= idr)
			{
				deltmp = tr;
				tr = tr->link;
				delete deltmp;
			}

			if (flag == 0)
				dat[i] = tr;
			else
				pre->link = tr;
		}
	}
}


void Singlebase::set_ran(int idl, int idr, int key, int val)
{
	int IDhashl = idl / lensize;
	int IDhashr = idr / lensize;
	chainnode *tr;


	for (int i = IDhashl; i <= IDhashr; i++)
	{
		if (dat[i])
		{
			tr = dat[i];
			while (tr && tr->row.id < idl)
				tr = tr->link;

			while (tr != NULL && tr->row.id <= idr)
			{
				tr->row.values[key] = val;
				tr = tr->link;
			}
		}
	}
}


void Singlebase::add_ran(int idl, int idr, int key, int val)
{
	int IDhashl = idl / lensize;
	int IDhashr = idr / lensize;
	chainnode *tr;


	for (int i = IDhashl; i <= IDhashr; i++)
	{
		if (dat[i])
		{
			tr = dat[i];
			while (tr && tr->row.id < idl)
				tr = tr->link;

			while (tr != NULL && tr->row.id <= idr)
			{
				tr->row.values[key] += val;
				tr = tr->link;
			}
		}
	}
}


void Singlebase::find_idran(int idl, int idr, int key, string op, int val)
{
	int IDhashl = idl / lensize;
	int IDhashr = idr / lensize;


	if (op == ">")
	{
		for (int i = IDhashl; i <= IDhashr; i++)
		{
			if (dat[i])
			{
				chainnode *tr = dat[i];
				while (tr && tr->row.id < idl)
					tr = tr->link;

				while (tr != NULL && tr->row.id <= idr)
				{
					if (tr->row.values[key] > val)
						output(tr);

					tr = tr->link;
				}
			}
		}
	}


	else if (op == "<")
	{
		for (int i = IDhashl; i <= IDhashr; i++)
		{
			if (dat[i])
			{
				chainnode *tr = dat[i];
				while (tr && tr->row.id < idl)
					tr = tr->link;

				while (tr != NULL && tr->row.id <= idr)
				{
					if (tr->row.values[key] < val)
						output(tr);

					tr = tr->link;
				}
			}
		}
	}


	else if (op == "<=")
	{
		for (int i = IDhashl; i <= IDhashr; i++)
		{
			if (dat[i])
			{
				chainnode *tr = dat[i];
				while (tr && tr->row.id < idl)
					tr = tr->link;

				while (tr != NULL && tr->row.id <= idr)
				{
					if (tr->row.values[key] <= val)
						output(tr);

					tr = tr->link;
				}
			}
		}
	}


	else if (op == ">=")
	{
		for (int i = IDhashl; i <= IDhashr; i++)
		{
			if (dat[i])
			{
				chainnode *tr = dat[i];
				while (tr && tr->row.id < idl)
					tr = tr->link;

				while (tr != NULL && tr->row.id <= idr)
				{
					if (tr->row.values[key] >= val)
						output(tr);

					tr = tr->link;
				}
			}
		}
	}


	else if (op == "=")
	{
		for (int i = IDhashl; i <= IDhashr; i++)
		{
			if (dat[i])
			{
				chainnode *tr = dat[i];
				while (tr && tr->row.id < idl)
					tr = tr->link;

				while (tr != NULL && tr->row.id <= idr)
				{
					if (tr->row.values[key] == val)
						output(tr);

					tr = tr->link;
				}
			}
		}
	}


	else if (op == "!=")
	{
		for (int i = IDhashl; i <= IDhashr; i++)
		{
			if (dat[i])
			{
				chainnode *tr = dat[i];
				while (tr && tr->row.id < idl)
					tr = tr->link;

				while (tr != NULL && tr->row.id <= idr)
				{
					if (tr->row.values[key] != val)
						output(tr);

					tr = tr->link;
				}
			}
		}
	}
}


void Singlebase::sum_idran(int idl, int idr, int key)
{
	int IDhashl = idl / lensize;
	int IDhashr = idr / lensize;
	chainnode *tr;
	int sum = 0;

	for (int i = IDhashl; i <= IDhashr; i++)
	{
		if (dat[i])
		{
			tr = dat[i];
			while (tr && tr->row.id < idl)
				tr = tr->link;

			while (tr != NULL && tr->row.id <= idr)
			{
				sum += tr->row.values[key];
				tr = tr->link;
			}
		}
	}

	if (TEST_TYPE == 1)
		fprintf(section1out, "%d\n", sum);
	else if (TEST_TYPE == 2)
		fprintf(section2out, "%d\n", sum);
}



/*===================================================mixed====================================================*/



void Singlebase::find_kthkey(int key, int k, char *mode)
{
	chainnode **mer = new chainnode*[sortsize];
	int ind = 0;
	bool mod;
	if (strcmp(mode, "ASC") == 0)
		mod = true;
	else
		mod = false;

	 
	for (int i = 0; i < hashsize; i++)  //复制到数组里
	{
		chainnode *tr = dat[i];
		while (tr != NULL)
		{
			mer[ind] = tr;
			tr = tr->link;
			ind++;
		}
	}


	for (int i = (ind - 2) / 2; i >= 0; i--)
		sift_down(mer, i, ind - 1, key, mod);


	for (int i = 1; i <= k; i++)
	{
		chainnode *temp = mer[0];
		mer[0] = mer[ind - i];
		mer[ind - i] = temp;

		sift_down(mer, 0, ind - i - 1, key, mod);
	}

	output(mer[ind - k]);

	delete[] mer;
}


void Singlebase::find_kthkey_ran(int idl, int idr, int key, int k, char *mode)
{
	chainnode **mer = new chainnode*[idr - idl + 2];
	int ind = 0;
	bool mod;
	if (strcmp(mode, "ASC") == 0)
		mod = true;
	else
		mod = false;

	int IDhashl = idl / lensize;
	int IDhashr = idr / lensize;


	for (int i = IDhashl; i <= IDhashr; i++)  //复制到数组里
	{
		if (dat[i])
		{
			chainnode *tr = dat[i];
			while (tr && tr->row.id < idl)
				tr = tr->link;

			while (tr && tr->row.id <= idr)
			{
				mer[ind] = tr;
				tr = tr->link;
				ind++;
			}
		}
	}


	for (int i = (ind - 2) / 2; i >= 0; i--)
		sift_down(mer, i, ind - 1, key, mod);


	for (int i = 1; i <= k; i++)
	{
		chainnode *temp = mer[0];
		mer[0] = mer[ind - i];
		mer[ind - i] = temp;

		sift_down(mer, 0, ind - i - 1, key, mod);
	}

	output(mer[ind - k]);

	delete[] mer;
}


void Singlebase::find_kthkeylist(int key, int k, char *mode)
{
	chainnode **mer = new chainnode*[sortsize];
	int ind = 0;
	bool mod;
	if (strcmp(mode, "ASC") == 0)
		mod = true;
	else
		mod = false;


	for (int i = 0; i < hashsize; i++)  //复制到数组里
	{
		chainnode *tr = dat[i];
		while (tr != NULL)
		{
			mer[ind] = tr;
			tr = tr->link;
			ind++;
		}
	}


	for (int i = (ind - 2) / 2; i >= 0; i--)
		sift_down(mer, i, ind - 1, key, mod);


	for (int i = 1; i <= k; i++)
	{
		chainnode *temp = mer[0];
		mer[0] = mer[ind - i];
		mer[ind - i] = temp;

		sift_down(mer, 0, ind - i - 1, key, mod);
	}

	for (int i = ind - 1; i >= ind - k; i--)
		output(mer[i]);

	delete[] mer;
}


void Singlebase::find_kthkey_ran_list(int idl, int idr, int key, int k, char * mode)
{
	chainnode **mer = new chainnode*[idr - idl + 2];
	int ind = 0;
	bool mod;
	if (strcmp(mode, "ASC") == 0)
		mod = true;
	else
		mod = false;

	int IDhashl = idl / lensize;
	int IDhashr = idr / lensize;


	for (int i = IDhashl; i <= IDhashr; i++)  //复制到数组里
	{
		if (dat[i])
		{
			chainnode *tr = dat[i];
			while (tr && tr->row.id < idl)
				tr = tr->link;

			while (tr && tr->row.id <= idr)
			{
				mer[ind] = tr;
				tr = tr->link;
				ind++;
			}
		}
	}


	for (int i = (ind - 2) / 2; i >= 0; i--)
		sift_down(mer, i, ind - 1, key, mod);


	for (int i = 1; i <= k; i++)
	{
		chainnode *temp = mer[0];
		mer[0] = mer[ind - i];
		mer[ind - i] = temp;

		sift_down(mer, 0, ind - i - 1, key, mod);
	}
	

	for (int i = ind - 1; i >= ind - k; i--)
		output(mer[i]);

	delete[] mer;
}


void Union(setbase *table1, setbase *table2)
{
	chainset *tr = table1->listhead;
	while (tr)  //先输出table1
	{
		fprintf(setout, "%s\n", tr->row->name); //output the name only
		tr = tr->link;
	}


	tr = table2->listhead;
	while (tr)  //再检查table2
	{
		if (!table1->name_exist(ELFhash(tr->row->name), tr->row->name)) //不存在才输出
			fprintf(setout, "%s\n", tr->row->name); //output the name only

		tr = tr->link;
	}
}


void inter(setbase *table1, setbase *table2)
{
	chainset *tr = table1->listhead;
	while (tr)
	{
		if (table2->name_exist(ELFhash(tr->row->name), tr->row->name))
			fprintf(setout, "%s\n", tr->row->name); //output the name only

		tr = tr->link;
	}
}


//Singlebase::~Singlebase()
//{
//	if (dat)
//	{
//		for (int i = 0; i < LISTLEN; i++)
//		{
//			delete[] dat[i].values;
//		}
//
//		delete[] dat;
//	}
//}