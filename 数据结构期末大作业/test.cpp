#include "common.h"
#include "test.h"

extern FILE *singleout;   //����
extern FILE *section1out; //����
extern FILE *section2out; //����2
extern FILE *setout;      //����
int cnt = 0;


void Timer::tic(char t[])
{
    strcpy(test_name, t);
    cout << test_name << " test begin." << endl;
    start = clock();

}


void Timer::toc()
{
    end = clock();
    used_time = double(end - start) / CLOCKS_PER_SEC;
    cout << test_name << " test done. Use time: " << used_time << endl;
}





/*ʱ������� ========================================================================================================================*/






Tester::Tester(const char *test_file, const char *result_file)
{
    fin.open(test_file);      //���ļ�����������������ָ���ļ���
    //fout.open(result_file);   //����Ľ���ļ�

	switch (cnt)
	{
	case 0:
		singleout = fopen(result_file, "w");
		break;

	case 1:
		section1out = fopen(result_file, "w");
		break;

	case 2:
		section2out = fopen(result_file, "w");
		break;

	case 3:
		setout = fopen(result_file, "w");
		break;
	}

	cnt++;
    //assert((fin && fout) || (INFO("open file failed!"), 0));
	//assert((fin) || (INFO("open file failed!"), 0));
}


Tester::~Tester()
{
    fin.close();
    //fout.close();
}



//���Ե�ִ�У�����������������������������������������������������������������������������������������������������������
void Tester::exec()   //���Ե�ִ��
{
    Command command;
    Timer timer;
    timer.tic(test_name);

    while (fin >> command)  //ͨ�����غ����������ָ��
        helper(command);  //������ִ��ָ��
    
    timer.toc();
}








SingleTester::SingleTester(const char *test_file, const char *result_file): Tester(test_file, result_file)
{
    strcpy(test_name, "Single");   
}


SectionTester::SectionTester(const char *test_file, const char *result_file): Tester(test_file, result_file)
{
    strcpy(test_name, "Section");
}


SetTester::SetTester(const char *test_file, const char *result_file): Tester(test_file, result_file)
{
    strcpy(test_name, "Set");
}