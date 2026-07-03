#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// ====================== 全局常量定义 ======================
#define HASH_TABLE_SIZE 512    // 哈希表桶大小
#define MAX_NAME_LEN 20
#define MAX_COLLEGE_LEN 30
#define MAX_DATE_LEN 11
#define MAX_TERM_LEN 7
#define FILE_NAME "record.dat" // 二进制持久化文件

// ====================== 1.选课记录结构体（文档标准字段） ======================
typedef struct Record {
    char stuId[13];        // 12位学号
    char stuName[MAX_NAME_LEN];
    char college[MAX_COLLEGE_LEN];
    char courseId[9];      // 8位课程号
    char courseName[40];
    float credit;          // 学分
    char term[MAX_TERM_LEN];// 6位学期 2024-02
    char selectDate[MAX_DATE_LEN]; // YYYY-MM-DD
    int score;             // 0~100成绩
} Record;

// ====================== 2.双向链表节点定义 ======================
typedef struct ListNode {
    Record data;
    struct ListNode *prev;
    struct ListNode *next;
} ListNode;

typedef struct DoubleList {
    ListNode *head;
    ListNode *tail;
    int size;
} DoubleList;

// ====================== 3.哈希表节点定义（拉链法） ======================
typedef struct HashNode {
    Record data;
    struct HashNode *next;
} HashNode;

typedef struct HashTable {
    HashNode *bucket[HASH_TABLE_SIZE];
    int size;
} HashTable;

// ====================== 4.全局双存储结构（同时维护两份数据，保证一致性） ======================
DoubleList g_list;
HashTable g_hash;

// ====================== 函数声明 ======================
// 工具函数
void ClearInputBuf();
int DateCompare(const char *d1, const char *d2);
void GenerateRandomRecord(Record *rec, int idSeq);
void BatchGenTestData(int num);
void PrintRecord(Record *r);
void SaveToFile();
void LoadFromFile();

// 双向链表ADT
void ListInit(DoubleList *L);
int ListInsert(DoubleList *L, Record *r);
int ListDeleteByKey(DoubleList *L, char *stuId, char *courseId);
ListNode* ListSearchStuId(DoubleList *L, char *stuId);
ListNode* ListSearchCourseName(DoubleList *L, char *name);
int ListModifyScore(DoubleList *L, char *stuId, char *cid, int newScore);
void ListTraverse(DoubleList *L, void (*func)(Record*));
void ListClear(DoubleList *L);

// 哈希表ADT
unsigned int HashFunc(char *stuId, char *courseId);
void HashInit(HashTable *ht);
int HashInsert(HashTable *ht, Record *r);
int HashDeleteByKey(HashTable *ht, char *stuId, char *courseId);
HashNode* HashSearch(HashTable *ht, char *stuId, char *courseId);
int HashModifyScore(HashTable *ht, char *stuId, char *cid, int newScore);
void HashTraverse(HashTable *ht, void (*func)(Record*));
void HashClear(HashTable *ht);

// 业务核心功能
void Func_InsertRecord();
void Func_DeleteSingle();
void Func_ModifyScore();
void Func_Search();
void Func_FilterMulti();
void Func_SortMultiKey();
void Func_StatAnalysis();
void Func_CleanExpired();
void Func_PerformanceTest();
void ShowMenu();

// ====================== 工具函数实现 ======================
void ClearInputBuf() {
    while(getchar() != '\n');
}

// 日期比较：d1<d2返回-1，相等0，d1>d2返回1
int DateCompare(const char *d1, const char *d2) {
    int y1,m1,d1_,y2,m2,d2_;
    sscanf(d1,"%d-%d-%d",&y1,&m1,&d1_);
    sscanf(d2,"%d-%d-%d",&y2,&m2,&d2_);
    if(y1 != y2) return y1 - y2;
    if(m1 != m2) return m1 - m2;
    return d1_ - d2_;
}

// 生成单条合法随机选课记录
void GenerateRandomRecord(Record *rec, int idSeq) {
    memset(rec,0,sizeof(Record));
    // 学号12位 B20250302xxxx
    sprintf(rec->stuId,"B20250302%04d",idSeq%9999);
    // 随机姓名
    char namePool[10][4] = {"张","李","王","刘","陈","杨","黄","赵","周","吴"};
    char surPool[10][4] = {"伟","芳","静","强","敏","磊","婷","杰","娟","浩"};
    strcat(rec->stuName, namePool[rand()%10]);
    strcat(rec->stuName, surPool[rand()%10]);
    // 学院
    char collegePool[5][30] = {"计算机科学与工程学院","机械工程学院","外国语学院","经济管理学院","艺术学院"};
    strcpy(rec->college, collegePool[rand()%5]);
    // 课程号 CSxxxxxx
    sprintf(rec->courseId,"CS%06d",rand()%999999);
    // 课程名
    char coursePool[6][40] = {"数据结构与算法","高等数学","大学英语","计算机网络","操作系统","Python程序设计"};
    strcpy(rec->courseName, coursePool[rand()%6]);
    // 学分 1.0~4.0
    rec->credit = 1.0 + (rand()%31)*0.1;
    // 学期 2023-01 ~ 2026-02
    int year = 2023 + rand()%4;
    int term = rand()%2 + 1;
    sprintf(rec->term,"%d-%02d",year,term);
    // 选课日期 2020-01-01 ~ 2026-06-01
    int yd = 2020 + rand()%7;
    int md = rand()%12 + 1;
    int dd = rand()%28 + 1;
    sprintf(rec->selectDate,"%04d-%02d-%02d",yd,md,dd);
    // 成绩 0~100
    rec->score = rand()%101;
}

// 批量生成N条测试数据并插入双结构
void BatchGenTestData(int num) {
    Record tmp;
    printf("正在批量生成 %d 条测试记录...\n",num);
    for(int i=0;i<num;i++){
        GenerateRandomRecord(&tmp, i);
        ListInsert(&g_list, &tmp);
        HashInsert(&g_hash, &tmp);
    }
    SaveToFile();
    printf("生成完成，已同步存入链表、哈希表并持久化到文件\n");
}

void PrintRecord(Record *r) {
    printf("====================================================\n");
    printf("学号：%s  姓名：%s  学院：%s\n", r->stuId, r->stuName, r->college);
    printf("课程号：%s  课程名：%s  学分：%.1f\n", r->courseId, r->courseName, r->credit);
    printf("学期：%s  选课日期：%s  成绩：%d\n", r->term, r->selectDate, r->score);
    printf("====================================================\n");
}

// 二进制持久化保存
void SaveToFile() {
    FILE *fp = fopen(FILE_NAME,"wb");
    if(!fp) { printf("文件打开失败，无法保存！\n"); return; }
    fwrite(&g_list.size, sizeof(int), 1, fp);
    ListNode *p = g_list.head;
    while(p) {
        fwrite(&p->data, sizeof(Record),1,fp);
        p = p->next;
    }
    fclose(fp);
}

// 从文件加载数据到双结构
void LoadFromFile() {
    FILE *fp = fopen(FILE_NAME,"rb");
    if(!fp) { printf("无历史数据文件，新建空白库\n"); return; }
    int total;
    fread(&total, sizeof(int),1,fp);
    Record tmp;
    ListClear(&g_list); HashClear(&g_hash);
    for(int i=0;i<total;i++){
        fread(&tmp, sizeof(Record),1,fp);
        ListInsert(&g_list, &tmp);
        HashInsert(&g_hash, &tmp);
    }
    fclose(fp);
    printf("成功加载 %d 条历史选课记录\n", total);
}

// ====================== 双向链表ADT实现 ======================
void ListInit(DoubleList *L) {
    L->head = L->tail = NULL;
    L->size = 0;
}

int ListInsert(DoubleList *L, Record *r) {
    ListNode *newNode = (ListNode*)malloc(sizeof(ListNode));
    if(!newNode) return -1;
    memcpy(&newNode->data, r, sizeof(Record));
    newNode->prev = newNode->next = NULL;
    if(L->size == 0) {
        L->head = L->tail = newNode;
    } else {
        newNode->prev = L->tail;
        L->tail->next = newNode;
        L->tail = newNode;
    }
    L->size++;
    return 0;
}

// 根据学号+课程号删除唯一记录
int ListDeleteByKey(DoubleList *L, char *stuId, char *courseId) {
    ListNode *p = L->head;
    while(p) {
        if(strcmp(p->data.stuId, stuId)==0 && strcmp(p->data.courseId, courseId)==0) {
            // 头节点
            if(p->prev == NULL) L->head = p->next;
            else p->prev->next = p->next;
            // 尾节点
            if(p->next == NULL) L->tail = p->prev;
            else p->next->prev = p->prev;
            free(p);
            L->size--;
            return 0;
        }
        p = p->next;
    }
    return -1;
}

ListNode* ListSearchStuId(DoubleList *L, char *stuId) {
    ListNode *p = L->head;
    while(p) {
        if(strcmp(p->data.stuId, stuId)==0) return p;
        p = p->next;
    }
    return NULL;
}

ListNode* ListSearchCourseName(DoubleList *L, char *name) {
    ListNode *p = L->head;
    while(p) {
        if(strstr(p->data.courseName, name)) return p;
        p = p->next;
    }
    return NULL;
}

int ListModifyScore(DoubleList *L, char *stuId, char *cid, int newScore) {
    ListNode *p = L->head;
    while(p) {
        if(strcmp(p->data.stuId,stuId)==0 && strcmp(p->data.courseId,cid)==0) {
            if(newScore<0 || newScore>100) return -2;
            p->data.score = newScore;
            return 0;
        }
        p = p->next;
    }
    return -1;
}

void ListTraverse(DoubleList *L, void (*func)(Record*)) {
    ListNode *p = L->head;
    while(p) {
        func(&p->data);
        p = p->next;
    }
}

void ListClear(DoubleList *L) {
    ListNode *p = L->head, *q;
    while(p) {
        q = p->next;
        free(p);
        p = q;
    }
    L->head = L->tail = NULL;
    L->size = 0;
}

// ====================== 哈希表ADT实现（拉链法） ======================
unsigned int HashFunc(char *stuId, char *courseId) {
    unsigned int hash = 0;
    char buf[30];
    strcpy(buf, stuId); strcat(buf, courseId);
    for(int i=0;buf[i];i++) hash = hash*31 + buf[i];
    return hash % HASH_TABLE_SIZE;
}

void HashInit(HashTable *ht) {
    for(int i=0;i<HASH_TABLE_SIZE;i++) ht->bucket[i] = NULL;
    ht->size = 0;
}

int HashInsert(HashTable *ht, Record *r) {
    unsigned int idx = HashFunc(r->stuId, r->courseId);
    HashNode *newNode = (HashNode*)malloc(sizeof(HashNode));
    if(!newNode) return -1;
    memcpy(&newNode->data, r, sizeof(Record));
    newNode->next = ht->bucket[idx];
    ht->bucket[idx] = newNode;
    ht->size++;
    return 0;
}

int HashDeleteByKey(HashTable *ht, char *stuId, char *courseId) {
    unsigned int idx = HashFunc(stuId, courseId);
    HashNode *p = ht->bucket[idx], *pre = NULL;
    while(p) {
        if(strcmp(p->data.stuId,stuId)==0 && strcmp(p->data.courseId,courseId)==0) {
            if(pre == NULL) ht->bucket[idx] = p->next;
            else pre->next = p->next;
            free(p);
            ht->size--;
            return 0;
        }
        pre = p;
        p = p->next;
    }
    return -1;
}

HashNode* HashSearch(HashTable *ht, char *stuId, char *courseId) {
    unsigned int idx = HashFunc(stuId, courseId);
    HashNode *p = ht->bucket[idx];
    while(p) {
        if(strcmp(p->data.stuId,stuId)==0 && strcmp(p->data.courseId,courseId)==0)
            return p;
        p = p->next;
    }
    return NULL;
}

int HashModifyScore(HashTable *ht, char *stuId, char *cid, int newScore) {
    HashNode *res = HashSearch(ht, stuId, cid);
    if(!res) return -1;
    if(newScore<0 || newScore>100) return -2;
    res->data.score = newScore;
    return 0;
}

void HashTraverse(HashTable *ht, void (*func)(Record*)) {
    for(int i=0;i<HASH_TABLE_SIZE;i++) {
        HashNode *p = ht->bucket[i];
        while(p) {
            func(&p->data);
            p = p->next;
        }
    }
}

void HashClear(HashTable *ht) {
    for(int i=0;i<HASH_TABLE_SIZE;i++) {
        HashNode *p = ht->bucket[i], *q;
        while(p) {
            q = p->next;
            free(p);
            p = q;
        }
        ht->bucket[i] = NULL;
    }
    ht->size = 0;
}

// ====================== 业务功能实现 ======================
// 1.插入单条记录
void Func_InsertRecord() {
    Record rec;
    printf("===新增选课记录===\n");
    printf("输入12位学号："); scanf("%s",rec.stuId); ClearInputBuf();
    printf("输入学生姓名："); scanf("%s",rec.stuName); ClearInputBuf();
    printf("输入学院："); scanf("%s",rec.college); ClearInputBuf();
    printf("输入8位课程号："); scanf("%s",rec.courseId); ClearInputBuf();
    printf("输入课程名称："); scanf("%[^\n]",rec.courseName); ClearInputBuf();
    printf("输入学分："); scanf("%f",&rec.credit); ClearInputBuf();
    printf("输入学期(2024-02)："); scanf("%s",rec.term); ClearInputBuf();
    printf("输入选课日期(YYYY-MM-DD)："); scanf("%s",rec.selectDate); ClearInputBuf();
    printf("输入成绩(0-100)："); scanf("%d",&rec.score); ClearInputBuf();
    if(rec.score<0 || rec.score>100) { printf("成绩非法，插入失败！\n"); return; }
    // 双结构同步插入
    ListInsert(&g_list, &rec);
    HashInsert(&g_hash, &rec);
    SaveToFile();
    printf("插入成功！\n");
}

// 2.按学号+课程号删除单条
void Func_DeleteSingle() {
    char sid[13], cid[9];
    printf("===删除单条选课记录===\n");
    printf("输入待删学号："); scanf("%s",sid); ClearInputBuf();
    printf("输入待删课程号："); scanf("%s",cid); ClearInputBuf();
    int ret1 = ListDeleteByKey(&g_list, sid, cid);
    int ret2 = HashDeleteByKey(&g_hash, sid, cid);
    if(ret1==0 && ret2==0) {
        SaveToFile();
        printf("删除成功，数据已同步更新\n");
    } else {
        printf("未找到匹配记录，删除失败\n");
    }
}

//3.修改成绩
void Func_ModifyScore() {
    char sid[13], cid[9];
    int newSc;
    printf("===修改选课成绩===\n");
    printf("学号："); scanf("%s",sid);
    printf("课程号："); scanf("%s",cid);
    printf("新成绩(0-100)："); scanf("%d",&newSc);
    int r1 = ListModifyScore(&g_list, sid, cid, newSc);
    int r2 = HashModifyScore(&g_hash, sid, cid, newSc);
    if(r1==0 && r2==0) {
        SaveToFile();
        printf("成绩修改完成\n");
    } else if(r1==-2) {
        printf("成绩超出0-100范围\n");
    } else {
        printf("无匹配记录\n");
    }
}

//4.查找：精确学号 / 模糊课程名
void Func_Search() {
    int op;
    printf("===检索功能===\n1.按学号精确查找 2.按课程名模糊查找\n请选择：");
    scanf("%d",&op); ClearInputBuf();
    if(op == 1) {
        char sid[13];
        printf("输入学号："); scanf("%s",sid);
        ListNode *res = ListSearchStuId(&g_list, sid);
        if(res) PrintRecord(&res->data);
        else printf("未找到该学生选课记录\n");
    } else if(op == 2) {
        char key[40];
        printf("输入课程关键字："); scanf("%s",key);
        ListNode *p = g_list.head;
        int cnt=0;
        while(p) {
            if(strstr(p->data.courseName, key)) {
                PrintRecord(&p->data); cnt++;
            }
            p = p->next;
        }
        if(cnt==0) printf("无匹配课程\n");
    }
}

//5.多条件筛选：课程名、学期、成绩区间、学院
void Func_FilterMulti() {
    char cName[40] = "";
    char term[7] = "";
    char college[30] = "";
    int sMin = 0, sMax = 100;
    printf("===多条件筛选（留空代表不限制）===\n");
    printf("课程名关键字："); scanf("%s",cName); ClearInputBuf();
    printf("选课学期(2024-02)："); scanf("%s",term); ClearInputBuf();
    printf("学院名称："); scanf("%s",college); ClearInputBuf();
    printf("成绩下限："); scanf("%d",&sMin); ClearInputBuf();
    printf("成绩上限："); scanf("%d",&sMax); ClearInputBuf();
    ListNode *p = g_list.head;
    int match = 0;
    while(p) {
        Record r = p->data;
        int ok = 1;
        if(strlen(cName) && !strstr(r.courseName, cName)) ok=0;
        if(strlen(term) && strcmp(r.term, term)!=0) ok=0;
        if(strlen(college) && strcmp(r.college, college)!=0) ok=0;
        if(r.score < sMin || r.score > sMax) ok=0;
        if(ok) { PrintRecord(&r); match++; }
        p = p->next;
    }
    printf("筛选完成，共匹配 %d 条记录\n", match);
}

//6.多关键字排序（成绩降序，学号升序）
void Func_SortMultiKey() {
    if(g_list.size <= 1) { printf("数据不足无需排序\n"); return; }
    Record arr[10000];
    int n = g_list.size;
    ListNode *p = g_list.head;
    for(int i=0;i<n;i++) { arr[i] = p->data; p = p->next; }
    //冒泡排序：成绩降序，学号升序
    for(int i=0;i<n-1;i++){
        for(int j=0;j<n-1-i;j++){
            if(arr[j].score < arr[j+1].score ||
               (arr[j].score == arr[j+1].score && strcmp(arr[j].stuId, arr[j+1].stuId)>0)){
                Record t = arr[j]; arr[j] = arr[j+1]; arr[j+1] = t;
            }
        }
    }
    printf("====多关键字排序结果（成绩降序，学号升序）====\n");
    for(int i=0;i<n;i++) PrintRecord(&arr[i]);
}

//7.数据统计分析（实现4项：课程选课人数、学生总学分、学院分布、成绩分段）
void Func_StatAnalysis() {
    printf("====数据统计分析====\n");
    // 7.1 各学院选课人数统计
    char colNames[5][30] = {"计算机科学与工程学院","机械工程学院","外国语学院","经济管理学院","艺术学院"};
    int colCnt[5] = {0};
    ListNode *p = g_list.head;
    while(p) {
        for(int i=0;i<5;i++){
            if(strcmp(p->data.college, colNames[i])==0) colCnt[i]++;
        }
        p = p->next;
    }
    printf("【各学院选课人数】\n");
    for(int i=0;i<5;i++) printf("%s：%d人\n", colNames[i], colCnt[i]);

    //7.2 成绩分段统计
    int grade[5] = {0}; //优秀90-100，良好80-89，中等70-79，及格60-69，不及格<60
    p = g_list.head;
    while(p) {
        int s = p->data.score;
        if(s>=90) grade[0]++;
        else if(s>=80) grade[1]++;
        else if(s>=70) grade[2]++;
        else if(s>=60) grade[3]++;
        else grade[4]++;
        p = p->next;
    }
    printf("\n【成绩分段统计】\n优秀(90-100)：%d 良好(80-89)：%d 中等(70-79)：%d 及格(60-69)：%d 不及格：%d\n",
           grade[0],grade[1],grade[2],grade[3],grade[4]);

    //7.3 每门课程选课人数
    typedef struct Cnt { char cid[9]; char cname[40]; int num; } Cnt;
    Cnt courseCnt[1000]; int cTotal=0;
    p = g_list.head;
    while(p) {
        int find = 0;
        for(int i=0;i<cTotal;i++){
            if(strcmp(courseCnt[i].cid, p->data.courseId)==0){
                courseCnt[i].num++; find=1; break;
            }
        }
        if(!find){
            strcpy(courseCnt[cTotal].cid, p->data.courseId);
            strcpy(courseCnt[cTotal].cname, p->data.courseName);
            courseCnt[cTotal].num = 1;
            cTotal++;
        }
        p = p->next;
    }
    printf("\n【各课程选课人数】\n");
    for(int i=0;i<cTotal;i++) printf("%s-%s：%d人选课\n", courseCnt[i].cid, courseCnt[i].cname, courseCnt[i].num);

    //7.4 学生总学分统计（取前10名展示）
    typedef struct StuCredit { char sid[13]; char name[20]; float sum; } StuCredit;
    StuCredit stuCr[2000]; int stuTotal=0;
    p = g_list.head;
    while(p) {
        int find=0;
        for(int i=0;i<stuTotal;i++){
            if(strcmp(stuCr[i].sid, p->data.stuId)==0){
                stuCr[i].sum += p->data.credit; find=1; break;
            }
        }
        if(!find){
            strcpy(stuCr[stuTotal].sid, p->data.stuId);
            strcpy(stuCr[stuTotal].name, p->data.stuName);
            stuCr[stuTotal].sum = p->data.credit;
            stuTotal++;
        }
        p = p->next;
    }
    printf("\n【学生累计总学分（前10）】\n");
    int show = stuTotal>10 ? 10 : stuTotal;
    for(int i=0;i<show;i++) printf("%s %s：%.1f学分\n", stuCr[i].sid, stuCr[i].name, stuCr[i].sum);
}

//8.批量清理过期记录：基准2026-09-01，删除早于2023-09-01的记录
void Func_CleanExpired() {
    char deadline[] = "2023-09-01";
    int delCount = 0;
    ListNode *p = g_list.head, *next;
    //先统计数量
    while(p) {
        next = p->next;
        if(DateCompare(p->data.selectDate, deadline) < 0) delCount++;
        p = next;
    }
    if(delCount == 0) { printf("无过期选课记录，无需清理\n"); return; }
    printf("检测到 %d 条过期记录（选课日期早于2023-09-01），确认删除？1确认/0取消：", delCount);
    int opt; scanf("%d",&opt);
    if(opt != 1) { printf("已取消清理操作\n"); return; }
    //遍历删除，同步双结构
    p = g_list.head;
    while(p) {
        next = p->next;
        if(DateCompare(p->data.selectDate, deadline) < 0) {
            char sid[13], cid[9];
            strcpy(sid, p->data.stuId);
            strcpy(cid, p->data.courseId);
            ListDeleteByKey(&g_list, sid, cid);
            HashDeleteByKey(&g_hash, sid, cid);
        }
        p = next;
    }
    SaveToFile();
    printf("清理完成，成功删除 %d 条过期数据\n", delCount);
}

//9.双结构性能对比测试（100/1000/10000条）
void Func_PerformanceTest() {
    int scale;
    printf("性能测试：输入数据规模(100/1000/10000)：");
    scanf("%d",&scale); ClearInputBuf();
    // 清空原有数据
    ListClear(&g_list); HashClear(&g_hash);
    BatchGenTestData(scale);
    clock_t st, ed;
    double timeList, timeHash;
    Record target;
    GenerateRandomRecord(&target, scale/2);

    // 查找耗时测试
    st = clock();
    ListSearchStuId(&g_list, target.stuId);
    ed = clock();
    timeList = (double)(ed-st)/CLOCKS_PER_SEC * 1000;

    st = clock();
    HashSearch(&g_hash, target.stuId, target.courseId);
    ed = clock();
    timeHash = (double)(ed-st)/CLOCKS_PER_SEC * 1000;

    printf("====性能测试结果（%d条数据）====\n", scale);
    printf("双向链表单次查找耗时：%.3f ms\n", timeList);
    printf("哈希表单次查找耗时：%.3f ms\n", timeHash);
    printf("链表size：%d 哈希表size：%d\n", g_list.size, g_hash.size);
    printf("结论：哈希表随机查找性能远优于链表，数据量越大差距越明显\n");
    SaveToFile();
}

// 菜单界面
void ShowMenu() {
    printf("\n================校园选课记录检索与大数据分析系统================\n");
    printf("1.新增选课记录        2.删除单条记录        3.修改成绩\n");
    printf("4.信息检索查询        5.多条件筛选          6.多关键字排序\n");
    printf("7.数据统计分析        8.批量清理过期数据    9.性能对比测试\n");
    printf("10.批量生成测试数据(输入100/1000/10000)    0.退出系统\n");
    printf("================================================================\n");
    printf("请输入功能编号：");
}

// ====================== 主函数 ======================
int main() {
    srand((unsigned)time(NULL));
    ListInit(&g_list);
    HashInit(&g_hash);
    LoadFromFile();
    int op;
    while(1) {
        ShowMenu();
        scanf("%d",&op); ClearInputBuf();
        switch(op) {
            case 1: Func_InsertRecord(); break;
            case 2: Func_DeleteSingle(); break;
            case 3: Func_ModifyScore(); break;
            case 4: Func_Search(); break;
            case 5: Func_FilterMulti(); break;
            case 6: Func_SortMultiKey(); break;
            case 7: Func_StatAnalysis(); break;
            case 8: Func_CleanExpired(); break;
            case 9: Func_PerformanceTest(); break;
            case 10: {
                int num; printf("生成数据条数："); scanf("%d",&num);
                BatchGenTestData(num); break;
            }
            case 0: ListClear(&g_list); HashClear(&g_hash);
                    printf("数据已保存，系统退出！\n"); return 0;
            default: printf("输入错误，请重新选择\n");
        }
    }
    return 0;
}