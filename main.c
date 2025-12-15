#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <windows.h>
#include <time.h>

#define MAX_LINE 256

HANDLE mutexList, mutexOutput, mutexVariables;
HANDLE semaphoreSlots, semaphoreItems;

FILE* logFile = NULL;

typedef struct Order {
	int id;
	char customerName[50];
	int quantidade;
	float precoTotal;
	struct Order* next;
} Order;

Order* head = NULL;
Order* tail = NULL;

int countPending = 0;
int countProcessing = 0;
int countProcessed = 0;

int minWorkTime = 0;
int maxWorkTime = 0;

int finished = 0;

void pushOrder(Order* newOrder) {
	newOrder->next = NULL;

	WaitForSingleObject(semaphoreSlots, INFINITE);
	WaitForSingleObject(mutexList, INFINITE);

	if (head == NULL) {
		head = newOrder;
		tail = newOrder;
	}
	else {
		tail->next = newOrder;
		tail = newOrder;
	}

	countPending++;

	ReleaseMutex(mutexList);
	ReleaseSemaphore(semaphoreItems, 1, NULL);
}

Order* popOrder() {
	WaitForSingleObject(semaphoreItems, INFINITE);
	WaitForSingleObject(mutexList, INFINITE);

	if (head == NULL) {
		ReleaseMutex(mutexList);
		return NULL;
	}

	Order* order = head;
	head = head->next;

	if (head == NULL) {
		tail = NULL;
	}

	countPending--;

	ReleaseMutex(mutexList);
	ReleaseSemaphore(semaphoreSlots, 1, NULL);

	return order;
}

DWORD WINAPI worker(LPVOID params)
{
	int myID = *(int*)params;
	free(params);

	while (1) {
		Order* myOrder = popOrder();

		if (myOrder == NULL) {
			if (finished) {
				break;
			}
			continue;
		}

		WaitForSingleObject(mutexVariables, INFINITE);
		countProcessing++;
		ReleaseMutex(mutexVariables);

		int waitTime = minWorkTime + (rand() % (maxWorkTime - minWorkTime + 1));
		Sleep(waitTime);

		WaitForSingleObject(mutexOutput, INFINITE);
		if (logFile) {
			fprintf(logFile, "Worker %d processou a encomenda %d (Cliente: %s)\n",
				myID, myOrder->id, myOrder->customerName);
			fflush(logFile);
		}
		ReleaseMutex(mutexOutput);

		WaitForSingleObject(mutexVariables, INFINITE);
		countProcessing--;
		countProcessed++;
		ReleaseMutex(mutexVariables);

		free(myOrder);
	}
	return 0;
}

DWORD WINAPI monitor(LPVOID params)
{
	int pend, proc, concl;

	while (1) {

		WaitForSingleObject(mutexList, INFINITE);
		pend = countPending;
		ReleaseMutex(mutexList);

		WaitForSingleObject(mutexVariables, INFINITE);
		proc = countProcessing;
		concl = countProcessed;

		if (finished && pend == 0 && proc == 0) {

			printf("[MONITOR] Pendentes: %03d | A Processar: %02d | Concluidos: %04d\n",
				pend, proc, concl);

			ReleaseMutex(mutexVariables);
			break;
		}

		ReleaseMutex(mutexVariables);

		printf("[MONITOR] Pendentes: %03d | A Processar: %02d | Concluidos: %04d\n",
			pend, proc, concl);

		Sleep(1000);
	}
	return 0;
}

void mainErrorHandeling(int argc, char* argv[]) {
	if (argc != 6) {
		printf("Numero incorreto de argumentos!\n");
		printf("Sintaxe: <Filename> <NWorkers> <MaxOrders> <MinWorkTime> <MaxWorkTime>\n");
		exit(1);
	}

	if (atoi(argv[2]) <= 0) {
		printf("Erro: Numero de workers invalido (deve ser > 0).\n");
		exit(1);
	}

	if (atoi(argv[3]) <= 0) {
		printf("Erro: Numero maximo de ordens invalido (deve ser > 0).\n");
		exit(1);
	}

	if (atoi(argv[4]) < 0) {
		printf("Erro: Duracao minima invalida (nao pode ser negativa).\n");
		exit(1);
	}

	int minTime = atoi(argv[4]);
	int maxTime = atoi(argv[5]);

	if (maxTime <= 0) {
		printf("Erro: Duracao maxima invalida (deve ser > 0).\n");
		exit(1);
	}

	if (maxTime < minTime) {
		printf("Erro: Duracao maxima (%d) nao pode ser menor que a minima (%d).\n", maxTime, minTime);
		exit(1);
	}
}

int main(int argc, char* argv[])
{
	mainErrorHandeling(argc, argv);

	char* filePath = argv[1];
	int nWorkers = atoi(argv[2]);
	int maxOrders = atoi(argv[3]);
	minWorkTime = atoi(argv[4]);
	maxWorkTime = atoi(argv[5]);

	srand(time(NULL));

	logFile = fopen("output.log", "w");
	if (!logFile) {
		printf("Erro ao criar output.log\n");
		return 1;
	}

	FILE* file = fopen(filePath, "r");
	if (!file) {
		printf("Erro a abrir ficheiro: %s\n", filePath);
		return 1;
	}
	remove("output.log");

	DWORD* threadIDWorker = (DWORD*)malloc(sizeof(DWORD) * nWorkers);
	DWORD threadIDMonitor;
	HANDLE* threadHWorker = (HANDLE*)malloc(sizeof(HANDLE) * nWorkers);
	HANDLE threadHMonitor;

	mutexList = CreateMutex(NULL, FALSE, NULL);
	if (mutexList == NULL) {
		printf("Create list mutex failed!\n");
		exit(1);
	}

	mutexOutput = CreateMutex(NULL, FALSE, NULL);
	if (mutexOutput == NULL) {
		printf("Create output mutex failed!\n");
		exit(1);
	};

	mutexVariables = CreateMutex(NULL, FALSE, NULL);
	if (mutexVariables == NULL) {
		printf("Create variables mutex failed!\n");
		exit(1);
	};

	semaphoreSlots = CreateSemaphore(NULL, maxOrders, maxOrders, NULL);
	if (semaphoreSlots == NULL) {
		printf("Create read semaphore failed!\n");
		exit(1);
	};

	semaphoreItems = CreateSemaphore(NULL, 0, maxOrders, NULL);
	if (semaphoreItems == NULL) {
		printf("Create write semaphore failed!\n");
		exit(1);
	};

	threadHMonitor = CreateThread(NULL, 0, monitor, NULL, 0, &threadIDMonitor);
	if (threadHMonitor == NULL) return 0;

	for (int i = 0; i < nWorkers; i++) {

		int* pID = (int*)malloc(sizeof(int));
		*pID = i;

		threadHWorker[i] = CreateThread(NULL, 0, worker, (LPVOID)pID, 0, &threadIDWorker[i]);
		if (threadHWorker[i] == NULL) return 0;

		if (threadHWorker[i] == NULL) {
			printf("Erro ao criar thread worker %d\n", i);
			free(pID); 
			return 1;
		}
	}

	int idTemp;
	char nameTemp[50];
	int qtdTemp;
	float priceTemp;

	while (fscanf(file, "%d;%49[^;];%d;%f", &idTemp, nameTemp, &qtdTemp, &priceTemp) == 4)
	{
		Order* newOrder = (Order*)malloc(sizeof(Order));
		if (newOrder == NULL) {
			printf("Erro de memoria\n");
			break;
		}

		newOrder->id = idTemp;
		strcpy(newOrder->customerName, nameTemp);
		newOrder->quantidade = qtdTemp;
		newOrder->precoTotal = priceTemp;

		pushOrder(newOrder);
	}

	fclose(file);

	finished = 1;

	for (int i = 0; i < nWorkers; i++) {
		if (!ReleaseSemaphore(semaphoreItems, 1, NULL)) {
			Sleep(10);
			i--;
		}
	}

	for (int i = 0; i < nWorkers; i++) {
		WaitForSingleObject(threadHWorker[i], INFINITE);
	}

	WaitForSingleObject(threadHMonitor, INFINITE);

	for (int i = 0; i < nWorkers; i++) {
		CloseHandle(threadHWorker[i]);
	}

	CloseHandle(threadHMonitor);

	free(threadIDWorker);
	free(threadHWorker);

	if (logFile) fclose(logFile);

	CloseHandle(mutexList);
	CloseHandle(mutexOutput);
	CloseHandle(mutexVariables);
	CloseHandle(semaphoreSlots);
	CloseHandle(semaphoreItems);

	printf("\n\nTodos os trabalhos concluidos. Total processado: %d\n", countProcessed);

	return 0;
}