#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <windows.h>

#define MAX_LINE 256

HANDLE mutexRead, mutexWrite;
HANDLE semaphoreRead, semaphoreWrite;

typedef struct Order {
	int id;
	char customerName[50];
	int quantidade;
	float precoTotal;
	struct Order* next;
} Order;

Order* list = NULL;

DWORD WINAPI worker(LPVOID params)
{
	return 0;
}

DWORD WINAPI monitor(LPVOID params)
{
	return 0;
}

void mainErrorHandeling(int argc, char* argv[]) {
	if (argc != 6) {
		printf("Incorret number of arguments!\n Syntax: <Filename> <NWorkers> <MaxOrders> <MinWorkTime> <MaxWorkTime>\n");
		exit(1);
	}

	if (atoi(argv[2]) == 0 && argv[2][0] != '0') {
		printf("Invalid number of workers\n");
		exit(1);
	}

	if (atoi(argv[3]) == 0 && argv[3][0] != '0') {
		printf("Invalid number of max orders\n");
		exit(1);
	}

	if (atoi(argv[4]) == 0 && argv[4][0] != '0') {
		printf("Invalid minimum worker duration\n");
		exit(1);
	}

	if (atoi(argv[5]) == 0 && argv[4][0] != '0') {
		printf("Invalid maximum worker duration\n");
		exit(1);
	}
}

int main(int argc, char* argv[])
{
	mainErrorHandeling(argc, argv);

	char* filePath = argv[1];
	int nWorkers = atoi(argv[2]);
	int maxOrders = atoi(argv[3]);
	int minWorkTime = atoi(argv[4]);
	int maxWorkTime = atoi(argv[5]);

	FILE* file = fopen(filePath, "r");
	if (!file) {
		printf("Erro a abrir ficheiro: %s\n", filePath);
		return 1;
	}

	DWORD* threadIDWorker = (DWORD*)malloc(sizeof(DWORD) * nWorkers);
	DWORD threadIDMonitor;
	HANDLE* threadHWorker = (HANDLE*)malloc(sizeof(HANDLE) * nWorkers);
	HANDLE threadHMonitor;

	mutexRead = CreateMutex(NULL, FALSE, NULL);
	if (mutexRead == NULL) {
		printf("Create read mutex failed!\n");
		exit(1);
	}

	mutexWrite = CreateMutex(NULL, FALSE, NULL);
	if (mutexWrite == NULL) {
		printf("Create write mutex failed!\n");
		exit(1);
	};

	semaphoreRead = CreateSemaphore(NULL, maxOrders, maxOrders, NULL);
	if (semaphoreRead == NULL) {
		printf("Create read semaphore failed!\n");
		exit(1);
	};

	semaphoreWrite = CreateSemaphore(NULL, 0, maxOrders, NULL);
	if (semaphoreWrite == NULL) {
		printf("Create write semaphore failed!\n");
		exit(1);
	};

	threadHMonitor = CreateThread(NULL, 0, monitor, (LPVOID)list, 0, &threadIDMonitor);
	if (threadHMonitor == NULL) return 0;

	for (int i = 0; i < nWorkers; i++) {
		threadHWorker[i] = CreateThread(NULL, 0, worker, (LPVOID)list, 0, &threadIDWorker[i]);
		if (threadHWorker[i] == NULL) return 0;
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

		WaitForSingleObject(semaphoreRead, INFINITE);
		WaitForSingleObject(mutexWrite, INFINITE);

		newOrder->next = list;
		list = newOrder;

		ReleaseMutex(mutexWrite);
		ReleaseSemaphore(semaphoreWrite, 1, NULL);
	}

	fclose(file);

	for (int i = 0; i < nWorkers; i++) {
		WaitForSingleObject(threadHWorker[i], INFINITE);
	}

	WaitForSingleObject(threadHMonitor, INFINITE);

	for (int i = 0; i < nWorkers; i++) {
		CloseHandle(threadHWorker[i]);
	}

	CloseHandle(threadHMonitor);

	return 0;
}